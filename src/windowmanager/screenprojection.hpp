#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qscreen.h>
#include <qtmetamacros.h>

#include "windowset.hpp"

namespace qs::wm {

///! WindowsetProjection covering one specific screen.
/// A ScreenProjection is a special type of @@WindowsetProjection which aggregates
/// all windowsets across all projections covering a specific screen.
///
/// When used with @@Windowset.setProjection(), an arbitrary projection on the screen
/// will be picked. Usually there is only one.
///
/// Use @@WindowManager.screenProjection() to get a ScreenProjection for a given screen.
class ScreenProjection: public WindowsetProjection {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

public:
	ScreenProjection(QScreen* screen, QObject* parent);

	[[nodiscard]] QScreen* screen() const;

private:
	QScreen* mScreen;
};

} // namespace qs::wm
