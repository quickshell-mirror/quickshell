#pragma once

#include <qnamespace.h>
#include <qobject.h>
#include <qproperty.h>
#include <qsize.h>
#include <qtmetamacros.h>

#include "proxywindow.hpp"
#include "windowinterface.hpp"

// see #include <qpa/qplatformwindow.h>
static const int QWINDOWSIZE_MAX = ((1 << 24) - 1);

class ProxyFloatingWindow: public ProxyWindowBase {
	Q_OBJECT;

public:
	explicit ProxyFloatingWindow(QObject* parent = nullptr): ProxyWindowBase(parent) {}

	void connectWindow() override;
	void setVisible(bool visible) override;

	[[nodiscard]] QObject* parentWindow() const;
	void setParentWindow(QObject* window);

	// Setting geometry while the window is visible makes the content item shrink but not the window
	// which is awful so we disable it for floating windows.
	void trySetWidth(qint32 implicitWidth) override;
	void trySetHeight(qint32 implicitHeight) override;

signals:
	void minimumSizeChanged();
	void maximumSizeChanged();
	void titleChanged();

private slots:
	void onParentDestroyed();
	void onParentVisible();

private:
	void onMinimumSizeChanged();
	void onMaximumSizeChanged();
	void onTitleChanged();

	QObject* mParentWindow = nullptr;
	ProxyWindowBase* mParentProxyWindow = nullptr;

public:
	Q_OBJECT_BINDABLE_PROPERTY(
	    ProxyFloatingWindow,
	    QString,
	    bTitle,
	    &ProxyFloatingWindow::onTitleChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY(
	    ProxyFloatingWindow,
	    QSize,
	    bMinimumSize,
	    &ProxyFloatingWindow::onMinimumSizeChanged
	);

	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(
	    ProxyFloatingWindow,
	    QSize,
	    bMaximumSize,
	    QSize(QWINDOWSIZE_MAX, QWINDOWSIZE_MAX),
	    &ProxyFloatingWindow::onMaximumSizeChanged
	);
};

///! Standard toplevel operating system window that looks like any other application.
class FloatingWindowInterface: public WindowInterface {
	Q_OBJECT;
	// clang-format off
	/// Window title.
	Q_PROPERTY(QString title READ default WRITE default NOTIFY titleChanged BINDABLE bindableTitle);
	/// Minimum window size given to the window system.
	Q_PROPERTY(QSize minimumSize READ default WRITE default NOTIFY minimumSizeChanged BINDABLE bindableMinimumSize);
	/// Maximum window size given to the window system.
	Q_PROPERTY(QSize maximumSize READ default WRITE default NOTIFY maximumSizeChanged BINDABLE bindableMaximumSize);
	/// Whether the window is currently minimized.
	Q_PROPERTY(bool minimized READ isMinimized WRITE setMinimized NOTIFY minimizedChanged);
	/// Whether the window is currently maximized.
	Q_PROPERTY(bool maximized READ isMaximized WRITE setMaximized NOTIFY maximizedChanged);
	/// Whether the window is currently fullscreen.
	Q_PROPERTY(bool fullscreen READ isFullscreen WRITE setFullscreen NOTIFY fullscreenChanged);
	/// The parent window of this window. Setting this makes the window a child of the parent,
	/// which affects window stacking behavior.
	///
	/// > [!NOTE] This property cannot be changed after the window is visible.
	Q_PROPERTY(QObject* parentWindow READ parentWindow WRITE setParentWindow);
	// clang-format on
	QML_NAMED_ELEMENT(FloatingWindow);

public:
	explicit FloatingWindowInterface(QObject* parent = nullptr);

	void onReload(QObject* oldInstance) override;

	[[nodiscard]] ProxyWindowBase* proxyWindow() const override;

	[[nodiscard]] QBindable<QSize> bindableMinimumSize() { return &this->window->bMinimumSize; }
	[[nodiscard]] QBindable<QSize> bindableMaximumSize() { return &this->window->bMaximumSize; }
	[[nodiscard]] QBindable<QString> bindableTitle() { return &this->window->bTitle; }

	[[nodiscard]] bool isMinimized() const;
	void setMinimized(bool minimized);
	[[nodiscard]] bool isMaximized() const;
	void setMaximized(bool maximized);
	[[nodiscard]] bool isFullscreen() const;
	void setFullscreen(bool fullscreen);

	/// Start a system move operation. Must be called during a pointer press/drag.
	Q_INVOKABLE [[nodiscard]] bool startSystemMove() const;
	/// Start a system resize operation. Must be called during a pointer press/drag.
	Q_INVOKABLE [[nodiscard]] bool startSystemResize(Qt::Edges edges) const;

	[[nodiscard]] QObject* parentWindow() const;
	void setParentWindow(QObject* window);

signals:
	void minimumSizeChanged();
	void maximumSizeChanged();
	void titleChanged();
	void minimizedChanged();
	void maximizedChanged();
	void fullscreenChanged();

private slots:
	void onWindowConnected();
	void onWindowStateChanged();

private:
	ProxyFloatingWindow* window;
	bool mMinimized = false;
	bool mMaximized = false;
	bool mFullscreen = false;
	bool mWasMinimized = false;
	bool mWasMaximized = false;
	bool mWasFullscreen = false;
};
