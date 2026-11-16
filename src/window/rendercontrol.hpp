#pragma once

#include <qpoint.h>
#include <qquickrendercontrol.h>
#include <qtmetamacros.h>
#include <qwindow.h>

namespace qs::window {

// Platform render loop, created by the registered factory.
class WindowRenderControl: public QQuickRenderControl {
	Q_OBJECT;

public:
	static void setEnabled(bool enabled);
	static void setFactory(WindowRenderControl* (*factory)());
	static WindowRenderControl* create();

	QWindow* renderWindow(QPoint* offset) override;

	// Commit surface state even without a repaint; see WaylandRenderControl::commitWithoutFrame().
	virtual void requestCommit() = 0;
};

} // namespace qs::window
