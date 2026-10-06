#pragma once

#include <private/qwayland-wayland.h>
#include <private/qwaylandscreen_p.h>
#include <qcontainerfwd.h>
#include <qtclasshelpermacros.h>
#include <qtypes.h>
#include <qwayland-wlr-screencopy-unstable-v1.h>
#include <wayland-client-protocol.h>

#include "../manager.hpp"

namespace qs::wayland::screencopy::wlr {

class WlrScreencopyManager;

class WlrScreencopyContext
    : public ScreencopyContext
    , public QtWayland::zwlr_screencopy_frame_v1 {
public:
	explicit WlrScreencopyContext(
	    WlrScreencopyManager* manager,
	    QScreen* screen,
	    bool paintCursors,
	    QRect region
	);
	~WlrScreencopyContext() override;
	Q_DISABLE_COPY_MOVE(WlrScreencopyContext);

	void captureFrame() override;
	void updateTransform(bool previouslyUnset);

protected:
	// clang-format off
	void zwlr_screencopy_frame_v1_buffer(uint32_t format, uint32_t width, uint32_t height, uint32_t stride) override;
	void zwlr_screencopy_frame_v1_linux_dmabuf(uint32_t format, uint32_t width, uint32_t height) override;
	void zwlr_screencopy_frame_v1_flags(uint32_t flags) override;
	void zwlr_screencopy_frame_v1_buffer_done() override;
	void zwlr_screencopy_frame_v1_ready(uint32_t tvSecHi, uint32_t tvSecLo, uint32_t tvNsec) override;
	void zwlr_screencopy_frame_v1_failed() override;
	// clang-format on

private slots:
	void onScreenDestroyed();

private:
	void submitFrame();

	// Learns the output transform from wl_output.geometry on a private bind.
	// QtWayland does not retain it: QWaylandScreen consumes mTransform in
	// updateOutputProperties() and QScreen::orientation() drops flipped variants.
	//
	// The proxy deliberately does not use Qt's generated QtWayland::wl_output.
	// Compositors send wl_surface.enter for every wl_output resource a client
	// holds, and QWaylandScreen::fromWlOutput() decides whether an output is one
	// of its own screens purely by the proxy's listener - so a proxy carrying
	// Qt's listener ends up in QWaylandSurface::m_screens as a fake QWaylandScreen
	// and is dereferenced after this object is gone. (quickshell#1094)
	class OutputTransformQuery {
	public:
		explicit OutputTransformQuery(WlrScreencopyContext* context);
		~OutputTransformQuery();
		Q_DISABLE_COPY_MOVE(OutputTransformQuery);

		qint32 transform = -1;
		void setScreen(QtWaylandClient::QWaylandScreen* screen);

	private:
		void release();

		// clang-format off
		static void onGeometry(void* data, ::wl_output* output, int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight, int32_t subpixel, const char* make, const char* model, int32_t transform);
		static void onMode(void* data, ::wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
		static void onDone(void* data, ::wl_output* output);
		static void onScale(void* data, ::wl_output* output, int32_t factor);
		static void onName(void* data, ::wl_output* output, const char* name);
		static void onDescription(void* data, ::wl_output* output, const char* description);
		// clang-format on

		static const wl_output_listener LISTENER;

		WlrScreencopyContext* context;
		::wl_output* output = nullptr;
	};

	WlrScreencopyManager* manager;
	buffer::WlBufferRequest request;
	bool copiedFirstFrame = false;
	OutputTransformQuery transform {this};
	bool yInvert = false;

	QtWaylandClient::QWaylandScreen* screen;
	bool paintCursors;
	QRect region;
};

} // namespace qs::wayland::screencopy::wlr
