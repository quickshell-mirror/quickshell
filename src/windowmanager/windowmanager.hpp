#pragma once

#include <functional>

#include <qhash.h>
#include <qlist.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qscreen.h>
#include <qtmetamacros.h>

#include "../core/qmlscreen.hpp"
#include "screenprojection.hpp"
#include "windowset.hpp"

namespace qs::wm {

class WindowManager: public QObject {
	Q_OBJECT;

public:
	static void setProvider(std::function<WindowManager*()> provider);
	static WindowManager* instance();

	Q_INVOKABLE ScreenProjection* screenProjection(QuickshellScreenInfo* screen);

	[[nodiscard]] QBindable<QList<Windowset*>> bindableWindowsets() const {
		return &this->bWindowsets;
	}

	[[nodiscard]] QBindable<QList<WindowsetProjection*>> bindableWindowsetProjections() const {
		return &this->bWindowsetProjections;
	}

signals:
	void windowsetsChanged();
	void windowsetProjectionsChanged();

public:
	Q_OBJECT_BINDABLE_PROPERTY(
	    WindowManager,
	    QList<Windowset*>,
	    bWindowsets,
	    &WindowManager::windowsetsChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    WindowManager,
	    QList<WindowsetProjection*>,
	    bWindowsetProjections,
	    &WindowManager::windowsetProjectionsChanged
	);

private:
	static std::function<WindowManager*()> provider;
	QHash<QScreen*, ScreenProjection*> mScreenProjections;
};

///! Window management interfaces exposed by the window manager.
class WindowManagerQml: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(WindowManager);
	QML_SINGLETON;
	// clang-format off
	/// All windowsets tracked by the WM across all projections.
	Q_PROPERTY(QList<Windowset*> windowsets READ default BINDABLE bindableWindowsets);
	/// All windowset projections tracked by the WM. Does not include
	/// internal projections from @@screenProjection().
	Q_PROPERTY(QList<WindowsetProjection*> windowsetProjections READ default BINDABLE bindableWindowsetProjections);
	// clang-format on

public:
	/// Returns an internal WindowsetProjection that covers a single screen and contains all
	/// windowsets on that screen, regardless of the WM-specified projection. Depending on
	/// how the WM lays out its actual projections, multiple ScreenProjections may contain
	/// the same Windowsets.
	Q_INVOKABLE static ScreenProjection* screenProjection(QuickshellScreenInfo* screen) {
		return WindowManager::instance()->screenProjection(screen);
	}

	[[nodiscard]] static QBindable<QList<Windowset*>> bindableWindowsets() {
		return WindowManager::instance()->bindableWindowsets();
	}

	[[nodiscard]] static QBindable<QList<WindowsetProjection*>> bindableWindowsetProjections() {
		return WindowManager::instance()->bindableWindowsetProjections();
	}
};

} // namespace qs::wm
