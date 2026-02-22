find_package(PkgConfig REQUIRED)
pkg_check_modules(dmabuf-deps REQUIRED IMPORTED_TARGET libdrm gbm egl)

find_path(VULKAN_INCLUDE_DIR vulkan/vulkan.h)
if (NOT VULKAN_INCLUDE_DIR)
	message(FATAL_ERROR "Vulkan headers are required but were not found.")
endif()

qt_add_library(quickshell-wayland-buffer STATIC
	manager.cpp
	dmabuf.cpp
	shm.cpp
)

wl_proto(wlp-linux-dmabuf linux-dmabuf-v1 "${WAYLAND_PROTOCOLS}/stable/linux-dmabuf")

target_link_libraries(quickshell-wayland-buffer PRIVATE
	Qt::Quick Qt::QuickPrivate Qt::WaylandClient Qt::WaylandClientPrivate wayland-client
	PkgConfig::dmabuf-deps
	wlp-linux-dmabuf
)

target_include_directories(quickshell-wayland-buffer PRIVATE ${VULKAN_INCLUDE_DIR})

qs_pch(quickshell-wayland-buffer SET large)
