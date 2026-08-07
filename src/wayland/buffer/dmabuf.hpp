#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <EGL/egl.h>
#include <gbm.h>
#include <qcontainerfwd.h>
#include <qhash.h>
#include <qlist.h>
#include <qquickwindow.h>
#include <qsgtexture.h>
#include <qsize.h>
#include <qtclasshelpermacros.h>
#include <qtypes.h>
#include <qvulkanfunctions.h>
#include <qwayland-linux-dmabuf-v1.h>
#include <qwaylandclientextension.h>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>
#include <wayland-linux-dmabuf-v1-client-protocol.h>
#include <wayland-util.h>
#include <xf86drm.h>

#include "manager.hpp"
#include "qsg.hpp"

namespace qs::wayland::buffer {
class WlBufferManagerPrivate;
}

namespace qs::wayland::buffer::dmabuf {

class LinuxDmabufManager;
class FourCCStr {
public:
	explicit FourCCStr(uint32_t code)
	    : chars(
	          {static_cast<char>(code >> 0 & 0xff),
	           static_cast<char>(code >> 8 & 0xff),
	           static_cast<char>(code >> 16 & 0xff),
	           static_cast<char>(code >> 24 & 0xff),
	           '\0'}
	      ) {
		for (auto i = 3; i != 0; i--) {
			if (this->chars[i] == ' ') this->chars[i] = '\0';
			else break;
		}
	}

	[[nodiscard]] const char* cStr() const { return this->chars.data(); }

private:
	std::array<char, 5> chars {};
};

class FourCCModStr {
public:
	explicit FourCCModStr(uint64_t code): drmStr(drmGetFormatModifierName(code)) {}
	~FourCCModStr() {
		if (this->drmStr) drmFree(this->drmStr);
	}

	Q_DISABLE_COPY_MOVE(FourCCModStr);

	[[nodiscard]] const char* cStr() const { return this->drmStr; }

private:
	char* drmStr;
};

QDebug& operator<<(QDebug& debug, const FourCCStr& fourcc);
QDebug& operator<<(QDebug& debug, const FourCCModStr& fourcc);

struct GbmDevice {
	GbmDevice(dev_t handle, std::string renderNode, gbm_device* device)
	    : handle(handle)
	    , renderNode(std::move(renderNode))
	    , device(device) {}
	~GbmDevice();
	Q_DISABLE_COPY_MOVE(GbmDevice);

	dev_t handle = 0;
	std::string renderNode;
	gbm_device* device = nullptr;
};

class WlDmaBufferQSGTexture: public WlBufferQSGTexture {
public:
	~WlDmaBufferQSGTexture() override;
	Q_DISABLE_COPY_MOVE(WlDmaBufferQSGTexture);

	[[nodiscard]] QSGTexture* texture() const override { return this->qsgTexture; }

private:
	WlDmaBufferQSGTexture(EGLImage eglImage, GLuint glTexture, QSGTexture* qsgTexture)
	    : eglImage(eglImage)
	    , glTexture(glTexture)
	    , qsgTexture(qsgTexture) {}

	EGLImage eglImage;
	GLuint glTexture;
	QSGTexture* qsgTexture;

	friend class WlDmaBuffer;
};

class WlDmaBufferVulkanQSGTexture: public WlBufferQSGTexture {
public:
	~WlDmaBufferVulkanQSGTexture() override;
	Q_DISABLE_COPY_MOVE(WlDmaBufferVulkanQSGTexture);

	[[nodiscard]] QSGTexture* texture() const override { return this->qsgTexture; }

private:
	WlDmaBufferVulkanQSGTexture(
	    QVulkanDeviceFunctions* devFuncs,
	    VkDevice device,
	    VkImage image,
	    VkDeviceMemory memory,
	    QSGTexture* qsgTexture
	)
	    : devFuncs(devFuncs)
	    , device(device)
	    , image(image)
	    , memory(memory)
	    , qsgTexture(qsgTexture) {}

	QVulkanDeviceFunctions* devFuncs = nullptr;
	VkDevice device = VK_NULL_HANDLE;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	QSGTexture* qsgTexture = nullptr;

	friend class WlDmaBuffer;
};

class WlDmaBuffer: public WlBuffer {
public:
	~WlDmaBuffer() override;
	Q_DISABLE_COPY(WlDmaBuffer);
	WlDmaBuffer(WlDmaBuffer&& other) noexcept;
	WlDmaBuffer& operator=(WlDmaBuffer&& other) noexcept;

	[[nodiscard]] wl_buffer* buffer() const override { return this->mBuffer; }

	[[nodiscard]] QSize size() const override {
		return QSize(static_cast<int>(this->width), static_cast<int>(this->height));
	}

	[[nodiscard]] bool isCompatible(const WlBufferRequest& request) const override;
	[[nodiscard]] WlBufferQSGTexture* createQsgTexture(QQuickWindow* window) const override;

private:
	WlDmaBuffer() noexcept = default;

	struct Plane {
		int fd = 0;
		uint32_t offset = 0;
		uint32_t stride = 0;
	};

	std::shared_ptr<GbmDevice> device;
	gbm_bo* bo = nullptr;
	wl_buffer* mBuffer = nullptr;
	int planeCount = 0;
	Plane* planes = nullptr;
	uint32_t format = 0;
	uint64_t modifier = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	bool usedImplicitModifier = false;

	friend class LinuxDmabufManager;
	friend QDebug& operator<<(QDebug& debug, const WlDmaBuffer* buffer);

	[[nodiscard]] WlBufferQSGTexture* createQsgTextureGl(QQuickWindow* window) const;
	[[nodiscard]] WlBufferQSGTexture* createQsgTextureVulkan(QQuickWindow* window) const;
};

QDebug& operator<<(QDebug& debug, const WlDmaBuffer* buffer);

struct LinuxDmabufModifiers {
	StackList<uint64_t, 10> modifiers;
	bool implicit = false;
};

struct LinuxDmabufFormatSelection {
	bool sorted = false;
	StackList<QPair<uint32_t, LinuxDmabufModifiers>, 2> formats;
	void ensureSorted();
};

struct LinuxDmabufTranche {
	dev_t device = 0;
	uint32_t flags = 0;
	LinuxDmabufFormatSelection formats;
};

class LinuxDmabufFeedback: public QtWayland::zwp_linux_dmabuf_feedback_v1 {
public:
	explicit LinuxDmabufFeedback(::zwp_linux_dmabuf_feedback_v1* feedback);
	~LinuxDmabufFeedback() override;
	Q_DISABLE_COPY_MOVE(LinuxDmabufFeedback);

protected:
	void zwp_linux_dmabuf_feedback_v1_main_device(wl_array* device) override;
	void zwp_linux_dmabuf_feedback_v1_format_table(int32_t fd, uint32_t size) override;
	void zwp_linux_dmabuf_feedback_v1_tranche_target_device(wl_array* device) override;
	void zwp_linux_dmabuf_feedback_v1_tranche_flags(uint32_t flags) override;
	void zwp_linux_dmabuf_feedback_v1_tranche_formats(wl_array* indices) override;
	void zwp_linux_dmabuf_feedback_v1_tranche_done() override;
	void zwp_linux_dmabuf_feedback_v1_done() override;

private:
	dev_t mainDevice = 0;
	QList<LinuxDmabufTranche> tranches;
	void* formatTable = nullptr;
	uint32_t formatTableSize = 0;
};

class LinuxDmabufManager
    : public QWaylandClientExtensionTemplate<LinuxDmabufManager>
    , public QtWayland::zwp_linux_dmabuf_v1 {
public:
	explicit LinuxDmabufManager(WlBufferManagerPrivate* manager);

	[[nodiscard]] WlBuffer* createDmabuf(const WlBufferRequest& request);

	[[nodiscard]] WlBuffer* createDmabuf(
	    const std::shared_ptr<GbmDevice>& device,
	    uint32_t format,
	    const LinuxDmabufModifiers& modifiers,
	    uint32_t width,
	    uint32_t height
	);

	bool initRenderFormats(QQuickWindow* window);

private:
	bool initRenderFormatsGl(QQuickWindow* window);
	bool initRenderFormatsVk(QQuickWindow* window);

	void feedbackDone();

	std::shared_ptr<GbmDevice> getGbmDevice(dev_t handle);

	LinuxDmabufFormatSelection renderFormats;
	std::string renderNode;
	QList<LinuxDmabufTranche> tranches;
	QList<std::weak_ptr<GbmDevice>> gbmDevices;
	WlBufferManagerPrivate* manager;

	friend class LinuxDmabufFeedback;
	friend struct GbmDevice;
};

} // namespace qs::wayland::buffer::dmabuf
