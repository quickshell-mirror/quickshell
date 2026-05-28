find_package(PkgConfig REQUIRED)
pkg_check_modules(dmabuf-deps REQUIRED IMPORTED_TARGET libdrm gbm egl)

find_package(VulkanHeaders REQUIRED)

qt_add_library(quickshell-wayland-buffer STATIC
	manager.cpp
	dmabuf.cpp
	shm.cpp
)

wl_proto(wlp-linux-dmabuf linux-dmabuf-v1 "${WAYLAND_PROTOCOLS}/stable/linux-dmabuf")

target_link_libraries(quickshell-wayland-buffer PRIVATE
	Qt::Quick Qt::QuickPrivate Qt::GuiPrivate Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	PkgConfig::dmabuf-deps
	wlp-linux-dmabuf
	Vulkan::Headers
)

qs_pch(quickshell-wayland-buffer SET large)
