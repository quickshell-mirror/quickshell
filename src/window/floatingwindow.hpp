#pragma once

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
	void postCompleteWindow() override;

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

	QBindable<QSize> bindableMinimumSize() { return &this->window->bMinimumSize; }
	QBindable<QSize> bindableMaximumSize() { return &this->window->bMaximumSize; }
	QBindable<QString> bindableTitle() { return &this->window->bTitle; }

	[[nodiscard]] QObject* parentWindow() const;
	void setParentWindow(QObject* window);

signals:
	void minimumSizeChanged();
	void maximumSizeChanged();
	void titleChanged();

private:
	ProxyFloatingWindow* window;
};
