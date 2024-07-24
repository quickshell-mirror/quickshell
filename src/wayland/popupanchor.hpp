#pragma once

#include <qwindow.h>

#include "../core/popupanchor.hpp"

class WaylandPopupPositioner: public PopupPositioner {
public:
	void reposition(PopupAnchor* anchor, QWindow* window, bool onlyIfDirty = true) override;
	[[nodiscard]] bool shouldRepositionOnMove() const override;

private:
	static void setFlags(PopupAnchor* anchor, QWindow* window);
};

void installPopupPositioner();
