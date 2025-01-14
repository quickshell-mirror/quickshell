#pragma once

#include <qtclasshelpermacros.h>
#include <qwayland-hyprland-toplevel-export-v1.h>

#include "../../toplevel_management/handle.hpp"
#include "../manager.hpp"

namespace qs::wayland::screencopy::hyprland {

class HyprlandScreencopyManager;

class HyprlandScreencopyContext
    : public ScreencopyContext
    , public QtWayland::hyprland_toplevel_export_frame_v1 {
public:
	explicit HyprlandScreencopyContext(
	    HyprlandScreencopyManager* manager,
	    toplevel_management::impl::ToplevelHandle* handle,
	    bool paintCursors
	);

	~HyprlandScreencopyContext() override;
	Q_DISABLE_COPY_MOVE(HyprlandScreencopyContext);

	void captureFrame() override;

protected:
	// clang-format off
	void hyprland_toplevel_export_frame_v1_buffer(uint32_t format, uint32_t width, uint32_t height, uint32_t stride) override;
	void hyprland_toplevel_export_frame_v1_linux_dmabuf(uint32_t format, uint32_t width, uint32_t height) override;
	void hyprland_toplevel_export_frame_v1_flags(uint32_t flags) override;
	void hyprland_toplevel_export_frame_v1_buffer_done() override;
	void hyprland_toplevel_export_frame_v1_ready(uint32_t tvSecHi, uint32_t tvSecLo, uint32_t tvNsec) override;
	void hyprland_toplevel_export_frame_v1_failed() override;
	// clang-format on

private slots:
	void onToplevelDestroyed();

private:
	HyprlandScreencopyManager* manager;
	buffer::WlBufferRequest request;
	bool copiedFirstFrame = false;

	toplevel_management::impl::ToplevelHandle* handle;
	bool paintCursors;
};

} // namespace qs::wayland::screencopy::hyprland
