#pragma once

#include <qlist.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "../../core/doc.hpp"
#include "../../core/model.hpp"
#include "../../core/qmlscreen.hpp"
#include "../../core/util.hpp"
#include "../../window/proxywindow.hpp"

namespace qs::wayland::toplevel_management {

namespace impl {
class ToplevelManager; // NOLINT
class ToplevelHandle;
} // namespace impl

///! Window from another application.
/// A window/toplevel from another application, retrievable from
/// the @@ToplevelManager.
class Toplevel: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QString appId READ appId NOTIFY appIdChanged);
	Q_PROPERTY(QString title READ title NOTIFY titleChanged);
	/// Parent toplevel if this toplevel is a modal/dialog, otherwise null.
	Q_PROPERTY(qs::wayland::toplevel_management::Toplevel* parent READ parent NOTIFY parentChanged);
	/// If the window is currently activated or focused.
	///
	/// Activation can be requested with the @@activate() function.
	Q_PROPERTY(bool activated READ activated NOTIFY activatedChanged);
	/// Screens the toplevel is currently visible on.
	/// Screens are listed in the order they have been added by the compositor.
	///
	/// > [!NOTE]	Some compositors only list a single screen, even if a window is visible on multiple.
	Q_PROPERTY(QList<QuickshellScreenInfo*> screens READ screens NOTIFY screensChanged);
	/// If the window is currently maximized.
	///
	/// Maximization can be requested by setting this property, though it may
	/// be ignored by the compositor.
	Q_PROPERTY(bool maximized READ maximized WRITE setMaximized NOTIFY maximizedChanged);
	/// If the window is currently minimized.
	///
	/// Minimization can be requested by setting this property, though it may
	/// be ignored by the compositor.
	Q_PROPERTY(bool minimized READ minimized WRITE setMinimized NOTIFY minimizedChanged);
	/// If the window is currently fullscreen.
	///
	/// Fullscreen can be requested by setting this property, though it may
	/// be ignored by the compositor.
	/// Fullscreen can be requested on a specific screen with the @@fullscreenOn() function.
	Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY fullscreenChanged);
	QML_ELEMENT;
	QML_UNCREATABLE("Toplevels must be acquired from the ToplevelManager.");

public:
	explicit Toplevel(impl::ToplevelHandle* handle, QObject* parent);

	/// Request that this toplevel is activated.
	/// The request may be ignored by the compositor.
	Q_INVOKABLE void activate();

	/// Request that this toplevel is closed.
	/// The request may be ignored by the compositor or the application.
	Q_INVOKABLE void close();

	/// Request that this toplevel is fullscreened on a specific screen.
	/// The request may be ignored by the compositor.
	Q_INVOKABLE void fullscreenOn(QuickshellScreenInfo* screen);

	/// Provide a hint to the compositor where the visual representation
	/// of this toplevel is relative to a quickshell window.
	/// This hint can be used visually in operations like minimization.
	Q_INVOKABLE void setRectangle(QObject* window, QRect rect);
	Q_INVOKABLE void unsetRectangle();

	[[nodiscard]] QString appId() const;
	[[nodiscard]] QString title() const;
	[[nodiscard]] Toplevel* parent() const;
	[[nodiscard]] bool activated() const;
	[[nodiscard]] QList<QuickshellScreenInfo*> screens() const;

	[[nodiscard]] bool maximized() const;
	void setMaximized(bool maximized);

	[[nodiscard]] bool minimized() const;
	void setMinimized(bool minimized);

	[[nodiscard]] bool fullscreen() const;
	void setFullscreen(bool fullscreen);

	[[nodiscard]] impl::ToplevelHandle* implHandle() const { return this->handle; }

signals:
	void closed();
	void appIdChanged();
	void titleChanged();
	void parentChanged();
	void activatedChanged();
	void screensChanged();
	void maximizedChanged();
	void minimizedChanged();
	void fullscreenChanged();

private slots:
	void onClosed();
	void onRectangleProxyConnected();
	void onRectangleProxyDestroyed();

private:
	impl::ToplevelHandle* handle;
	ProxyWindowBase* rectWindow = nullptr;
	QRect rectangle;

	friend class ToplevelManager;
};

class ToplevelManager: public QObject {
	Q_OBJECT;

public:
	Toplevel* forImpl(impl::ToplevelHandle* impl) const;

	[[nodiscard]] ObjectModel<Toplevel>* toplevels();

	static ToplevelManager* instance();

signals:
	void activeToplevelChanged();

private slots:
	void onToplevelReady(impl::ToplevelHandle* handle);
	void onToplevelActiveChanged();
	void onToplevelClosed();

private:
	explicit ToplevelManager();

	ObjectModel<Toplevel> mToplevels {this};
	Toplevel* mActiveToplevel = nullptr;

	DECLARE_PRIVATE_MEMBER(
	    ToplevelManager,
	    activeToplevel,
	    setActiveToplevel,
	    mActiveToplevel,
	    activeToplevelChanged
	);
};

///! Exposes a list of Toplevels.
/// Exposes a list of windows from other applications as @@Toplevel$s via the
/// [zwlr-foreign-toplevel-management-v1](https://wayland.app/protocols/wlr-foreign-toplevel-management-unstable-v1)
/// wayland protocol.
class ToplevelManagerQml: public QObject {
	Q_OBJECT;
	// clang-format off
	/// All toplevel windows exposed by the compositor.
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::wayland::toplevel_management::Toplevel>*);
	Q_PROPERTY(UntypedObjectModel* toplevels READ toplevels CONSTANT);
	/// Active toplevel or null.
	///
	/// > [!INFO] If multiple are active, this will be the most recently activated one.
	/// > Usually compositors will not report more than one toplevel as active at a time.
	Q_PROPERTY(qs::wayland::toplevel_management::Toplevel* activeToplevel READ activeToplevel NOTIFY activeToplevelChanged);
	// clang-format on
	QML_NAMED_ELEMENT(ToplevelManager);
	QML_SINGLETON;

public:
	explicit ToplevelManagerQml(QObject* parent = nullptr);

	[[nodiscard]] static ObjectModel<Toplevel>* toplevels();
	[[nodiscard]] static Toplevel* activeToplevel();

signals:
	void activeToplevelChanged();
};

} // namespace qs::wayland::toplevel_management
