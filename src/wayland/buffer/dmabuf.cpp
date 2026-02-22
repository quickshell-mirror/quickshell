#include "dmabuf.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>
#include <GLES3/gl32.h>
#include <fcntl.h>
#include <gbm.h>
#include <libdrm/drm_fourcc.h>
#include <private/qquickwindow_p.h>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qopenglcontext.h>
#include <qopenglcontext_platform.h>
#include <qpair.h>
#include <qquickwindow.h>
#include <qscopedpointer.h>
#include <qsgrendererinterface.h>
#include <qsgtexture_platform.h>
#include <qvulkanfunctions.h>
#include <qvulkaninstance.h>
#include <qwayland-linux-dmabuf-v1.h>
#include <qwaylandclientextension.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <qtypes.h>
#include <vulkan/vulkan_core.h>
#include <wayland-client-protocol.h>
#include <wayland-linux-dmabuf-v1-client-protocol.h>
#include <wayland-util.h>
#include <xf86drm.h>

#include "../../core/logcat.hpp"
#include "../../core/stacklist.hpp"
#include "manager.hpp"
#include "manager_p.hpp"

namespace qs::wayland::buffer::dmabuf {

namespace {

QS_LOGGING_CATEGORY(logDmabuf, "quickshell.wayland.buffer.dmabuf", QtWarningMsg);

LinuxDmabufManager* MANAGER = nullptr; // NOLINT

VkFormat drmFormatToVkFormat(uint32_t drmFormat) {
	// NOLINTBEGIN(bugprone-branch-clone): XRGB/ARGB intentionally map to the same VK format
	switch (drmFormat) {
	case DRM_FORMAT_ARGB8888: return VK_FORMAT_B8G8R8A8_UNORM;
	case DRM_FORMAT_XRGB8888: return VK_FORMAT_B8G8R8A8_UNORM;
	case DRM_FORMAT_ABGR8888: return VK_FORMAT_R8G8B8A8_UNORM;
	case DRM_FORMAT_XBGR8888: return VK_FORMAT_R8G8B8A8_UNORM;
	case DRM_FORMAT_ARGB2101010: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
	case DRM_FORMAT_XRGB2101010: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
	case DRM_FORMAT_ABGR2101010: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
	case DRM_FORMAT_XBGR2101010: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
	case DRM_FORMAT_ABGR16161616F: return VK_FORMAT_R16G16B16A16_SFLOAT;
	case DRM_FORMAT_RGB565: return VK_FORMAT_R5G6B5_UNORM_PACK16;
	case DRM_FORMAT_BGR565: return VK_FORMAT_B5G6R5_UNORM_PACK16;
	default: return VK_FORMAT_UNDEFINED;
	}
	// NOLINTEND(bugprone-branch-clone)
}

} // namespace

QDebug& operator<<(QDebug& debug, const FourCCStr& fourcc) {
	debug << fourcc.cStr();
	return debug;
}

QDebug& operator<<(QDebug& debug, const FourCCModStr& fourcc) {
	debug << fourcc.cStr();
	return debug;
}

QDebug& operator<<(QDebug& debug, const WlDmaBuffer* buffer) {
	auto saver = QDebugStateSaver(debug);
	debug.nospace();

	if (buffer) {
		debug << "WlDmaBuffer(" << static_cast<const void*>(buffer) << ", size=" << buffer->width << 'x'
		      << buffer->height << ", format=" << FourCCStr(buffer->format) << ", modifier=`"
		      << FourCCModStr(buffer->modifier) << "`)";
	} else {
		debug << "WlDmaBuffer(0x0)";
	}

	return debug;
}

GbmDeviceHandle::~GbmDeviceHandle() {
	if (this->device) {
		MANAGER->unrefGbmDevice(this->device);
	}
}

// This will definitely backfire later
void LinuxDmabufFormatSelection::ensureSorted() {
	if (this->sorted) return;
	auto beginIter = this->formats.begin();

	auto xrgbIter = std::ranges::find_if(this->formats, [](const auto& format) {
		return format.first == DRM_FORMAT_XRGB8888;
	});

	if (xrgbIter != this->formats.end()) {
		std::swap(*beginIter, *xrgbIter);
		++beginIter;
	}

	auto argbIter = std::ranges::find_if(this->formats, [](const auto& format) {
		return format.first == DRM_FORMAT_ARGB8888;
	});

	if (argbIter != this->formats.end()) std::swap(*beginIter, *argbIter);

	this->sorted = true;
}

LinuxDmabufFeedback::LinuxDmabufFeedback(::zwp_linux_dmabuf_feedback_v1* feedback)
    : zwp_linux_dmabuf_feedback_v1(feedback) {}

LinuxDmabufFeedback::~LinuxDmabufFeedback() { this->destroy(); }

void LinuxDmabufFeedback::zwp_linux_dmabuf_feedback_v1_format_table(int32_t fd, uint32_t size) {
	this->formatTableSize = size;

	this->formatTable = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);

	if (this->formatTable == MAP_FAILED) {
		this->formatTable = nullptr;
		qCFatal(logDmabuf) << "Failed to mmap format table.";
	}

	qCDebug(logDmabuf) << "Got format table";
}

void LinuxDmabufFeedback::zwp_linux_dmabuf_feedback_v1_main_device(wl_array* device) {
	if (device->size != sizeof(dev_t)) {
		qCFatal(logDmabuf) << "The size of dev_t used by the compositor and quickshell is mismatched. "
		                      "Try recompiling both.";
	}

	this->mainDevice = *reinterpret_cast<dev_t*>(device->data);
	qCDebug(logDmabuf) << "Got main device id" << this->mainDevice;
}

void LinuxDmabufFeedback::zwp_linux_dmabuf_feedback_v1_tranche_target_device(wl_array* device) {
	if (device->size != sizeof(dev_t)) {
		qCFatal(logDmabuf) << "The size of dev_t used by the compositor and quickshell is mismatched. "
		                      "Try recompiling both.";
	}

	auto& tranche = this->tranches.emplaceBack();
	tranche.device = *reinterpret_cast<dev_t*>(device->data);
	qCDebug(logDmabuf) << "Got target device id" << this->mainDevice;
}

void LinuxDmabufFeedback::zwp_linux_dmabuf_feedback_v1_tranche_flags(uint32_t flags) {
	this->tranches.back().flags = flags;
	qCDebug(logDmabuf) << "Got target device flags" << flags;
}

void LinuxDmabufFeedback::zwp_linux_dmabuf_feedback_v1_tranche_formats(wl_array* indices) {
	struct FormatTableEntry {
		uint32_t format;
		uint32_t padding;
		uint64_t modifier;
	};

	static_assert(sizeof(FormatTableEntry) == 16, "Format table entry was not packed to 16 bytes.");

	if (this->formatTable == nullptr) {
		qCFatal(logDmabuf) << "Received tranche formats before format table.";
	}

	auto& tranche = this->tranches.back();

	auto* table = reinterpret_cast<FormatTableEntry*>(this->formatTable);
	auto* indexTable = reinterpret_cast<uint16_t*>(indices->data);
	auto indexTableLength = indices->size / sizeof(uint16_t);

	uint32_t lastFormat = 0;
	LinuxDmabufModifiers* modifiers = nullptr;

	for (uint16_t ti = 0; ti != indexTableLength; ++ti) {
		auto i = indexTable[ti];      // NOLINT
		const auto& entry = table[i]; // NOLINT

		// Compositors usually send a single format's modifiers as a block.
		if (!modifiers || entry.format != lastFormat) {
			lastFormat = entry.format;

			auto modifiersIter = std::ranges::find_if(tranche.formats.formats, [&](const auto& pair) {
				return pair.first == entry.format;
			});

			if (modifiersIter == tranche.formats.formats.end()) {
				tranche.formats.formats.push(qMakePair(entry.format, LinuxDmabufModifiers()));
				modifiers = &(--tranche.formats.formats.end())->second;
			} else {
				modifiers = &modifiersIter->second;
			}
		}

		if (entry.modifier == DRM_FORMAT_MOD_INVALID) {
			modifiers->implicit = true;
		} else {
			modifiers->modifiers.push(entry.modifier);
		}
	}
}

void LinuxDmabufFeedback::zwp_linux_dmabuf_feedback_v1_tranche_done() {
	qCDebug(logDmabuf) << "Got tranche end.";
}

void LinuxDmabufFeedback::zwp_linux_dmabuf_feedback_v1_done() {
	qCDebug(logDmabuf) << "Got feedback done.";

	if (this->formatTable) {
		munmap(this->formatTable, this->formatTableSize);
		this->formatTable = nullptr;
	}

	if (logDmabuf().isDebugEnabled()) {
		qCDebug(logDmabuf) << "Dmabuf tranches:";

		for (auto& tranche: this->tranches) {
			qCDebug(logDmabuf) << "  Tranche on device" << tranche.device;

			// will be sorted on first use otherwise
			tranche.formats.ensureSorted();

			for (auto& [format, modifiers]: tranche.formats.formats) {
				qCDebug(logDmabuf) << "    Format" << FourCCStr(format);

				if (modifiers.implicit) {
					qCDebug(logDmabuf) << "      Implicit Modifier";
				}

				for (const auto& modifier: modifiers.modifiers) {
					qCDebug(logDmabuf) << "      Explicit Modifier" << FourCCModStr(modifier);
				}
			}
		}
	}

	// Copy tranches to the manager. If the compositor ever updates
	// our tranches, we'll start from a clean slate.
	MANAGER->tranches = this->tranches;
	this->tranches.clear();

	MANAGER->feedbackDone();
}

LinuxDmabufManager::LinuxDmabufManager(WlBufferManagerPrivate* manager)
    : QWaylandClientExtensionTemplate(5)
    , manager(manager) {
	MANAGER = this;
	this->initialize();

	if (this->isActive()) {
		qCDebug(logDmabuf) << "Requesting default dmabuf feedback...";
		new LinuxDmabufFeedback(this->get_default_feedback());
	}
}

void LinuxDmabufManager::feedbackDone() { this->manager->dmabufReady(); }

GbmDeviceHandle LinuxDmabufManager::getGbmDevice(dev_t handle) {
	struct DrmFree {
		static void cleanup(drmDevice* d) { drmFreeDevice(&d); }
	};

	std::string renderNodeStorage;
	std::string* renderNode = nullptr;

	auto sharedDevice = std::ranges::find_if(this->gbmDevices, [&](const SharedGbmDevice& d) {
		return d.handle == handle;
	});

	if (sharedDevice != this->gbmDevices.end()) {
		renderNode = &sharedDevice->renderNode;
	} else {
		drmDevice* drmDevPtr = nullptr;
		if (auto error = drmGetDeviceFromDevId(handle, 0, &drmDevPtr); error != 0) {
			qCWarning(logDmabuf) << "Failed to get drm device information from handle:"
			                     << qt_error_string(error);
			return nullptr;
		}

		auto drmDev = QScopedPointer<drmDevice, DrmFree>(drmDevPtr);

		if (!(drmDev->available_nodes & (1 << DRM_NODE_RENDER))) {
			qCDebug(logDmabuf) << "Cannot create GBM device: DRM device does not have render node.";
			return nullptr;
		}

		renderNodeStorage = drmDev->nodes[DRM_NODE_RENDER]; // NOLINT
		renderNode = &renderNodeStorage;
		sharedDevice = std::ranges::find_if(this->gbmDevices, [&](const SharedGbmDevice& d) {
			return d.renderNode == renderNodeStorage;
		});
	}

	if (sharedDevice != this->gbmDevices.end()) {
		qCDebug(logDmabuf) << "Used existing GBM device on render node" << *renderNode;
		++sharedDevice->refcount;
		return sharedDevice->device;
	} else {
		auto fd = open(renderNode->c_str(), O_RDWR | O_CLOEXEC);
		if (fd < 0) {
			qCDebug(logDmabuf) << "Could not open render node" << *renderNode << ":"
			                   << qt_error_string(fd);
			return nullptr;
		}

		auto* device = gbm_create_device(fd);

		if (!device) {
			qCDebug(logDmabuf) << "Failed to create GBM device from render node" << *renderNode;
			close(fd);
			return nullptr;
		}

		qCDebug(logDmabuf) << "Created GBM device on render node" << *renderNode;

		this->gbmDevices.push_back({
		    .handle = handle,
		    .renderNode = std::move(renderNodeStorage),
		    .device = device,
		    .refcount = 1,
		});

		return device;
	}
}

void LinuxDmabufManager::unrefGbmDevice(gbm_device* device) {
	auto iter = std::ranges::find_if(this->gbmDevices, [device](const SharedGbmDevice& d) {
		return d.device == device;
	});
	if (iter == this->gbmDevices.end()) return;

	qCDebug(logDmabuf) << "Lost reference to GBM device" << device;

	if (--iter->refcount == 0) {
		auto fd = gbm_device_get_fd(iter->device);
		gbm_device_destroy(iter->device);
		close(fd);

		this->gbmDevices.erase(iter);
		qCDebug(logDmabuf) << "Destroyed GBM device" << device;
	}
}

GbmDeviceHandle LinuxDmabufManager::dupHandle(const GbmDeviceHandle& handle) {
	if (!handle) return GbmDeviceHandle();

	auto iter = std::ranges::find_if(this->gbmDevices, [&handle](const SharedGbmDevice& d) {
		return d.device == *handle;
	});
	if (iter == this->gbmDevices.end()) return GbmDeviceHandle();

	qCDebug(logDmabuf) << "Duplicated GBM device handle" << *handle;
	++iter->refcount;
	return GbmDeviceHandle(*handle);
}

WlBuffer* LinuxDmabufManager::createDmabuf(const WlBufferRequest& request) {
	for (auto& tranche: this->tranches) {
		if (request.dmabuf.device != 0 && tranche.device != request.dmabuf.device) {
			continue;
		}

		LinuxDmabufFormatSelection formats;
		for (const auto& format: request.dmabuf.formats) {
			if (!format.modifiers.isEmpty()) {
				formats.formats.push(
				    qMakePair(format.format, LinuxDmabufModifiers {.modifiers = format.modifiers})
				);
			} else {
				for (const auto& trancheFormat: tranche.formats.formats) {
					if (trancheFormat.first == format.format) {
						formats.formats.push(trancheFormat);
					}
				}
			}
		}

		if (formats.formats.isEmpty()) continue;
		formats.ensureSorted();

		auto gbmDevice = this->getGbmDevice(tranche.device);

		if (!gbmDevice) {
			qCWarning(logDmabuf) << "Hit unusable tranche device while trying to create dmabuf.";
			continue;
		}

		for (const auto& [format, modifiers]: formats.formats) {
			if (auto* buf =
			        this->createDmabuf(gbmDevice, format, modifiers, request.width, request.height))
			{
				return buf;
			}
		}
	}

	qCWarning(logDmabuf) << "Unable to create dmabuf for request: No matching formats.";
	return nullptr;
}

WlBuffer* LinuxDmabufManager::createDmabuf(
    GbmDeviceHandle& device,
    uint32_t format,
    const LinuxDmabufModifiers& modifiers,
    uint32_t width,
    uint32_t height
) {
	auto buffer = std::unique_ptr<WlDmaBuffer>(new WlDmaBuffer());
	auto& bo = buffer->bo;

	const uint32_t flags = GBM_BO_USE_RENDERING;

	if (modifiers.modifiers.isEmpty()) {
		if (!modifiers.implicit) {
			qCritical(
			    logDmabuf
			) << "Failed to create gbm_bo: format supports no implicit OR explicit modifiers.";
			return nullptr;
		}

		qCDebug(logDmabuf) << "Creating gbm_bo without modifiers...";
		bo = gbm_bo_create(*device, width, height, format, flags);
	} else {
		qCDebug(logDmabuf) << "Creating gbm_bo with modifiers...";

		STACKLIST_VLA_VIEW(uint64_t, modifiers.modifiers, modifiersData);

		bo = gbm_bo_create_with_modifiers2(
		    *device,
		    width,
		    height,
		    format,
		    modifiersData,
		    modifiers.modifiers.length(),
		    flags
		);
	}

	if (!bo) {
		qCritical(logDmabuf) << "Failed to create gbm_bo.";
		return nullptr;
	}

	buffer->planeCount = gbm_bo_get_plane_count(bo);
	buffer->planes = new WlDmaBuffer::Plane[buffer->planeCount]();
	buffer->modifier = gbm_bo_get_modifier(bo);

	auto params = QtWayland::zwp_linux_buffer_params_v1(this->create_params());

	for (auto i = 0; i < buffer->planeCount; ++i) {
		auto& plane = buffer->planes[i]; // NOLINT
		plane.fd = gbm_bo_get_fd_for_plane(bo, i);

		if (plane.fd < 0) {
			qCritical(logDmabuf) << "Failed to get gbm_bo fd for plane" << i << qt_error_string(plane.fd);
			params.destroy();
			gbm_bo_destroy(bo);
			return nullptr;
		}

		plane.stride = gbm_bo_get_stride_for_plane(bo, i);
		plane.offset = gbm_bo_get_offset(bo, i);

		params.add(
		    plane.fd,
		    i,
		    plane.offset,
		    plane.stride,
		    buffer->modifier >> 32,
		    buffer->modifier & 0xffffffff
		);
	}

	buffer->mBuffer =
	    params.create_immed(static_cast<int32_t>(width), static_cast<int32_t>(height), format, 0);
	params.destroy();

	buffer->device = this->dupHandle(device);
	buffer->width = width;
	buffer->height = height;
	buffer->format = format;

	qCDebug(logDmabuf) << "Created dmabuf" << buffer.get();
	return buffer.release();
}

WlDmaBuffer::WlDmaBuffer(WlDmaBuffer&& other) noexcept
    : device(std::move(other.device))
    , bo(other.bo)
    , mBuffer(other.mBuffer)
    , planes(other.planes) {
	other.mBuffer = nullptr;
	other.bo = nullptr;
	other.planeCount = 0;
}

WlDmaBuffer& WlDmaBuffer::operator=(WlDmaBuffer&& other) noexcept {
	this->~WlDmaBuffer();
	new (this) WlDmaBuffer(std::move(other));
	return *this;
}

WlDmaBuffer::~WlDmaBuffer() {
	if (this->mBuffer) {
		wl_buffer_destroy(this->mBuffer);
	}

	if (this->bo) {
		gbm_bo_destroy(this->bo);
		qCDebug(logDmabuf) << "Destroyed" << this << "freeing bo" << this->bo;
	}

	for (auto i = 0; i < this->planeCount; ++i) {
		const auto& plane = this->planes[i]; // NOLINT
		if (plane.fd) close(plane.fd);
	}

	delete[] this->planes;
}

bool WlDmaBuffer::isCompatible(const WlBufferRequest& request) const {
	if (request.width != this->width || request.height != this->height) return false;

	auto matchingFormat = std::ranges::find_if(request.dmabuf.formats, [this](const auto& format) {
		return format.format == this->format
		    && (format.modifiers.isEmpty()
		        || std::ranges::find(format.modifiers, this->modifier) != format.modifiers.end());
	});

	return matchingFormat != request.dmabuf.formats.end();
}

WlBufferQSGTexture* WlDmaBuffer::createQsgTexture(QQuickWindow* window) const {
	auto* ri = window->rendererInterface();
	if (ri && ri->graphicsApi() == QSGRendererInterface::Vulkan) {
		return this->createQsgTextureVulkan(window);
	}

	return this->createQsgTextureGl(window);
}

WlBufferQSGTexture* WlDmaBuffer::createQsgTextureGl(QQuickWindow* window) const {
	static auto* glEGLImageTargetTexture2DOES = []() {
		auto* fn = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
		    eglGetProcAddress("glEGLImageTargetTexture2DOES")
		);

		if (!fn) {
			qCFatal(logDmabuf) << "Failed to create QSG texture from WlDmaBuffer: "
			                      "glEGLImageTargetTexture2DOES is missing.";
		}

		return fn;
	}();

	auto* context = QOpenGLContext::currentContext();
	if (!context) {
		qCFatal(logDmabuf) << "Failed to create QSG texture from WlDmaBuffer: No GL context.";
	}

	auto* qEglContext = context->nativeInterface<QNativeInterface::QEGLContext>();
	if (!qEglContext) {
		qCFatal(logDmabuf) << "Failed to create QSG texture from WlDmaBuffer: No EGL context.";
	}

	auto* display = qEglContext->display();

	// Ref https://github.com/hyprwm/hyprlock/blob/da1d076d849fc0f298c1d287bddd04802bf7d0f9/src/renderer/Screencopy.cpp#L194
	struct AttribNameSet {
		EGLAttrib fd;
		EGLAttrib offset;
		EGLAttrib pitch;
		EGLAttrib modlo;
		EGLAttrib modhi;
	};

	static auto attribNames = std::array<AttribNameSet, 4> {
	    AttribNameSet {
	        .fd = EGL_DMA_BUF_PLANE0_FD_EXT,
	        .offset = EGL_DMA_BUF_PLANE0_OFFSET_EXT,
	        .pitch = EGL_DMA_BUF_PLANE0_PITCH_EXT,
	        .modlo = EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
	        .modhi = EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT
	    },
	    AttribNameSet {
	        .fd = EGL_DMA_BUF_PLANE1_FD_EXT,
	        .offset = EGL_DMA_BUF_PLANE1_OFFSET_EXT,
	        .pitch = EGL_DMA_BUF_PLANE1_PITCH_EXT,
	        .modlo = EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
	        .modhi = EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT
	    },
	    AttribNameSet {
	        .fd = EGL_DMA_BUF_PLANE2_FD_EXT,
	        .offset = EGL_DMA_BUF_PLANE2_OFFSET_EXT,
	        .pitch = EGL_DMA_BUF_PLANE2_PITCH_EXT,
	        .modlo = EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
	        .modhi = EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT
	    },
	    AttribNameSet {
	        .fd = EGL_DMA_BUF_PLANE3_FD_EXT,
	        .offset = EGL_DMA_BUF_PLANE3_OFFSET_EXT,
	        .pitch = EGL_DMA_BUF_PLANE3_PITCH_EXT,
	        .modlo = EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT,
	        .modhi = EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT
	    }
	};

	// clang-format off
	auto attribs = std::vector<EGLAttrib> {
		EGL_WIDTH, this->width,
		EGL_HEIGHT, this->height,
		EGL_LINUX_DRM_FOURCC_EXT, this->format,
	};
	// clang-format on

	if (this->planeCount > 4) {
		qFatal(logDmabuf) << "Could not create EGL attrib array with more than 4 planes. Count:"
		                  << this->planeCount;
	}

	for (auto i = 0; i != this->planeCount; i++) {
		const auto& names = attribNames[i];
		const auto& plane = this->planes[i]; // NOLINT

		// clang-format off
		attribs.insert(attribs.end(), {
		    names.fd, plane.fd,
		    names.offset, plane.offset,
		    names.pitch, plane.stride,
		});
		// clang-format on

		// clang-format off
		if (this->modifier != DRM_FORMAT_MOD_INVALID) {
			attribs.insert(attribs.end(), {
			    names.modlo, static_cast<EGLAttrib>(this->modifier & 0xFFFFFFFF),
			    names.modhi, static_cast<EGLAttrib>(this->modifier >> 32),
			});
		}
		// clang-format on
	}

	attribs.emplace_back(EGL_NONE);

	auto* eglImage =
	    eglCreateImage(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attribs.data());

	if (eglImage == EGL_NO_IMAGE) {
		qFatal() << "Failed to create egl image" << eglGetError();
		return nullptr;
	}

	window->beginExternalCommands();
	GLuint glTexture = 0;
	glGenTextures(1, &glTexture);

	glBindTexture(GL_TEXTURE_2D, glTexture);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage);
	glBindTexture(GL_TEXTURE_2D, 0);
	window->endExternalCommands();

	auto* qsgTexture = QNativeInterface::QSGOpenGLTexture::fromNative(
	    glTexture,
	    window,
	    QSize(static_cast<int>(this->width), static_cast<int>(this->height))
	);

	auto* tex = new WlDmaBufferQSGTexture(eglImage, glTexture, qsgTexture);
	qCDebug(logDmabuf) << "Created WlDmaBufferQSGTexture" << tex << "from" << this;
	return tex;
}

WlBufferQSGTexture* WlDmaBuffer::createQsgTextureVulkan(QQuickWindow* window) const {
	auto* ri = window->rendererInterface();
	auto* vkInst = window->vulkanInstance();

	if (!vkInst) {
		qCWarning(logDmabuf) << "Failed to create Vulkan QSG texture: no QVulkanInstance.";
		return nullptr;
	}

	auto* vkDevicePtr =
	    static_cast<VkDevice*>(ri->getResource(window, QSGRendererInterface::DeviceResource));
	auto* vkPhysDevicePtr = static_cast<VkPhysicalDevice*>(
	    ri->getResource(window, QSGRendererInterface::PhysicalDeviceResource)
	);

	if (!vkDevicePtr || !vkPhysDevicePtr) {
		qCWarning(logDmabuf) << "Failed to create Vulkan QSG texture: could not get Vulkan device.";
		return nullptr;
	}

	VkDevice device = *vkDevicePtr;
	VkPhysicalDevice physDevice = *vkPhysDevicePtr;

	auto* devFuncs = vkInst->deviceFunctions(device);
	auto* instFuncs = vkInst->functions();

	if (!devFuncs || !instFuncs) {
		qCWarning(logDmabuf) << "Failed to create Vulkan QSG texture: "
		                        "could not get Vulkan functions.";
		return nullptr;
	}

	auto getMemoryFdPropertiesKHR = reinterpret_cast<PFN_vkGetMemoryFdPropertiesKHR>(
	    instFuncs->vkGetDeviceProcAddr(device, "vkGetMemoryFdPropertiesKHR")
	);

	if (!getMemoryFdPropertiesKHR) {
		qCWarning(logDmabuf) << "Failed to create Vulkan QSG texture: "
		                        "vkGetMemoryFdPropertiesKHR not available. "
		                        "Missing VK_KHR_external_memory_fd extension.";
		return nullptr;
	}

	const VkFormat vkFormat = drmFormatToVkFormat(this->format);
	if (vkFormat == VK_FORMAT_UNDEFINED) {
		qCWarning(logDmabuf) << "Failed to create Vulkan QSG texture: unsupported DRM format"
		                     << FourCCStr(this->format);
		return nullptr;
	}

	if (this->planeCount > 4) {
		qCWarning(logDmabuf) << "Failed to create Vulkan QSG texture: too many planes"
		                     << this->planeCount;
		return nullptr;
	}

	std::array<VkSubresourceLayout, 4> planeLayouts = {};
	for (int i = 0; i < this->planeCount; ++i) {
		planeLayouts[i].offset = this->planes[i].offset;   // NOLINT
		planeLayouts[i].rowPitch = this->planes[i].stride; // NOLINT
		planeLayouts[i].size = 0;
		planeLayouts[i].arrayPitch = 0;
		planeLayouts[i].depthPitch = 0;
	}

	const bool useModifier = this->modifier != DRM_FORMAT_MOD_INVALID;

	VkExternalMemoryImageCreateInfo externalInfo = {};
	externalInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
	externalInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

	VkImageDrmFormatModifierExplicitCreateInfoEXT modifierInfo = {};
	modifierInfo.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
	modifierInfo.drmFormatModifier = this->modifier;
	modifierInfo.drmFormatModifierPlaneCount = static_cast<uint32_t>(this->planeCount);
	modifierInfo.pPlaneLayouts = planeLayouts.data();

	if (useModifier) {
		externalInfo.pNext = &modifierInfo;
	}

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.pNext = &externalInfo;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = vkFormat;
	imageInfo.extent = {.width = this->width, .height = this->height, .depth = 1};
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = useModifier ? VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT : VK_IMAGE_TILING_LINEAR;
	imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImage image = VK_NULL_HANDLE;
	VkResult result = devFuncs->vkCreateImage(device, &imageInfo, nullptr, &image);
	if (result != VK_SUCCESS) {
		qCWarning(logDmabuf) << "Failed to create VkImage for DMA-BUF import, result:" << result;
		return nullptr;
	}

	VkDeviceMemory memory = VK_NULL_HANDLE;

	// dup() is required because vkAllocateMemory with VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT
	// takes ownership of the fd on succcess. Without dup, WlDmaBuffer would double-close.
	const int dupFd = dup(this->planes[0].fd); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	if (dupFd < 0) {
		qCWarning(logDmabuf) << "Failed to dup() fd for DMA-BUF import";
		goto cleanup_fail; // NOLINT
	}

	{
		VkMemoryRequirements memReqs = {};
		devFuncs->vkGetImageMemoryRequirements(device, image, &memReqs);

		VkMemoryFdPropertiesKHR fdProps = {};
		fdProps.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR;

		result = getMemoryFdPropertiesKHR(
		    device,
		    VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
		    dupFd,
		    &fdProps
		);

		if (result != VK_SUCCESS) {
			close(dupFd);
			qCWarning(logDmabuf) << "vkGetMemoryFdPropertiesKHR failed, result:" << result;
			goto cleanup_fail; // NOLINT
		}

		const uint32_t memTypeBits = memReqs.memoryTypeBits & fdProps.memoryTypeBits;

		VkPhysicalDeviceMemoryProperties memProps = {};
		instFuncs->vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);

		uint32_t memTypeIndex = UINT32_MAX;
		for (uint32_t j = 0; j < memProps.memoryTypeCount; ++j) {
			if (memTypeBits & (1u << j)) {
				memTypeIndex = j;
				break;
			}
		}

		if (memTypeIndex == UINT32_MAX) {
			close(dupFd);
			qCWarning(logDmabuf) << "No compatible memory type for DMA-BUF import";
			goto cleanup_fail; // NOLINT
		}

		VkImportMemoryFdInfoKHR importInfo = {};
		importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
		importInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
		importInfo.fd = dupFd;

		VkMemoryDedicatedAllocateInfo dedicatedInfo = {};
		dedicatedInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
		dedicatedInfo.image = image;
		dedicatedInfo.pNext = &importInfo;

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = &dedicatedInfo;
		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = memTypeIndex;

		result = devFuncs->vkAllocateMemory(device, &allocInfo, nullptr, &memory);
		if (result != VK_SUCCESS) {
			close(dupFd);
			qCWarning(logDmabuf) << "vkAllocateMemory failed, result:" << result;
			goto cleanup_fail; // NOLINT
		}

		result = devFuncs->vkBindImageMemory(device, image, memory, 0);
		if (result != VK_SUCCESS) {
			qCWarning(logDmabuf) << "vkBindImageMemory failed, result:" << result;
			goto cleanup_fail; // NOLINT
		}
	}

	{
		// acquire the DMA-BUF from the foreign (compositor) queue and transition
		// to shader-read layout. oldLayout must be GENERAL (not UNDEFINED) to
		// preserve the DMA-BUF contents written by the external producer. Hopefully.
		window->beginExternalCommands();

		auto* cmdBufPtr = static_cast<VkCommandBuffer*>(
		    ri->getResource(window, QSGRendererInterface::CommandListResource)
		);

		if (cmdBufPtr && *cmdBufPtr) {
			VkCommandBuffer cmdBuf = *cmdBufPtr;

			// find the graphics queue family index for the ownrship transfer.
			uint32_t graphicsQueueFamily = 0;
			uint32_t queueFamilyCount = 0;
			instFuncs->vkGetPhysicalDeviceQueueFamilyProperties(
			    physDevice, &queueFamilyCount, nullptr
			);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			instFuncs->vkGetPhysicalDeviceQueueFamilyProperties(
			    physDevice, &queueFamilyCount, queueFamilies.data()
			);
			for (uint32_t i = 0; i < queueFamilyCount; ++i) {
				if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					graphicsQueueFamily = i;
					break;
				}
			}

			VkImageMemoryBarrier barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_FOREIGN_EXT;
			barrier.dstQueueFamilyIndex = graphicsQueueFamily;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			devFuncs->vkCmdPipelineBarrier(
			    cmdBuf,
			    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			    0,
			    0,
			    nullptr,
			    0,
			    nullptr,
			    1,
			    &barrier
			);
		}

		window->endExternalCommands();

		auto* qsgTexture = QQuickWindowPrivate::get(window)->createTextureFromNativeTexture(
		    reinterpret_cast<quint64>(image),
		    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		    static_cast<uint>(vkFormat),
		    QSize(static_cast<int>(this->width), static_cast<int>(this->height)),
		    {}
		);

		auto* tex = new WlDmaBufferVulkanQSGTexture(
		    devFuncs,
		    device,
		    image,
		    memory,
		    qsgTexture
		);
		qCDebug(logDmabuf) << "Created WlDmaBufferVulkanQSGTexture" << tex << "from" << this;
		return tex;
	}

cleanup_fail:
	if (image != VK_NULL_HANDLE) {
		devFuncs->vkDestroyImage(device, image, nullptr);
	}
	if (memory != VK_NULL_HANDLE) {
		devFuncs->vkFreeMemory(device, memory, nullptr);
	}
	return nullptr;
}

WlDmaBufferVulkanQSGTexture::~WlDmaBufferVulkanQSGTexture() {
	delete this->qsgTexture;

	if (this->image != VK_NULL_HANDLE) {
		this->devFuncs->vkDestroyImage(this->device, this->image, nullptr);
	}

	if (this->memory != VK_NULL_HANDLE) {
		this->devFuncs->vkFreeMemory(this->device, this->memory, nullptr);
	}

	qCDebug(logDmabuf) << "WlDmaBufferVulkanQSGTexture" << this << "destroyed.";
}

WlDmaBufferQSGTexture::~WlDmaBufferQSGTexture() {
	auto* context = QOpenGLContext::currentContext();
	auto* display = context->nativeInterface<QNativeInterface::QEGLContext>()->display();

	if (this->glTexture) glDeleteTextures(1, &this->glTexture);
	if (this->eglImage) eglDestroyImage(display, this->eglImage);
	delete this->qsgTexture;

	qCDebug(logDmabuf) << "WlDmaBufferQSGTexture" << this << "destroyed.";
}

} // namespace qs::wayland::buffer::dmabuf
