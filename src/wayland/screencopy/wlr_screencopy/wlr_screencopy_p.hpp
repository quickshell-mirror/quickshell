#pragma once

#include <private/qwayland-wayland.h>
#include <private/qwaylandscreen_p.h>
#include <qcontainerfwd.h>
#include <qtclasshelpermacros.h>
#include <qtypes.h>
#include <qwayland-wlr-screencopy-unstable-v1.h>

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

	class OutputTransformQuery: public QtWayland::wl_output {
	public:
		OutputTransformQuery(WlrScreencopyContext* context);
		~OutputTransformQuery() override;
		Q_DISABLE_COPY_MOVE(OutputTransformQuery);

		qint32 transform = -1;
		void setScreen(QtWaylandClient::QWaylandScreen* screen);

	protected:
		void output_geometry(
		    qint32 x,
		    qint32 y,
		    qint32 width,
		    qint32 height,
		    qint32 subpixel,
		    const QString& make,
		    const QString& model,
		    qint32 transform
		) override;

	private:
		WlrScreencopyContext* context;
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
