#pragma once

#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

#include "../buffer/manager.hpp"

namespace qs::wayland::screencopy {

class ScreencopyContext: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] buffer::WlBufferSwapchain& swapchain() { return this->mSwapchain; }
	virtual void captureFrame() = 0;

signals:
	void frameCaptured();
	void stopped();

protected:
	ScreencopyContext() = default;

	buffer::WlBufferSwapchain mSwapchain;
};

class ScreencopyManager {
public:
	static ScreencopyContext* createContext(QObject* object, bool paintCursors);
};

} // namespace qs::wayland::screencopy
