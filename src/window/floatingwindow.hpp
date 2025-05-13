#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

#include "proxywindow.hpp"
#include "windowinterface.hpp"

class ProxyFloatingWindow: public ProxyWindowBase {
	Q_OBJECT;

public:
	explicit ProxyFloatingWindow(QObject* parent = nullptr): ProxyWindowBase(parent) {}

	// Setting geometry while the window is visible makes the content item shrink but not the window
	// which is awful so we disable it for floating windows.
	void trySetWidth(qint32 implicitWidth) override;
	void trySetHeight(qint32 implicitHeight) override;
};

///! Standard toplevel operating system window that looks like any other application.
class FloatingWindowInterface: public WindowInterface {
	Q_OBJECT;
	QML_NAMED_ELEMENT(FloatingWindow);

public:
	explicit FloatingWindowInterface(QObject* parent = nullptr);

	void onReload(QObject* oldInstance) override;

	[[nodiscard]] ProxyWindowBase* proxyWindow() const override;
	[[nodiscard]] QQuickItem* contentItem() const override;

	// NOLINTBEGIN
	[[nodiscard]] bool isVisible() const override;
	[[nodiscard]] bool isBackingWindowVisible() const override;
	void setVisible(bool visible) override;

	[[nodiscard]] qint32 implicitWidth() const override;
	void setImplicitWidth(qint32 implicitWidth) override;

	[[nodiscard]] qint32 implicitHeight() const override;
	void setImplicitHeight(qint32 implicitHeight) override;

	[[nodiscard]] qint32 width() const override;
	void setWidth(qint32 width) override;

	[[nodiscard]] qint32 height() const override;
	void setHeight(qint32 height) override;

	[[nodiscard]] virtual qreal devicePixelRatio() const override;

	[[nodiscard]] QuickshellScreenInfo* screen() const override;
	void setScreen(QuickshellScreenInfo* screen) override;

	[[nodiscard]] QColor color() const override;
	void setColor(QColor color) override;

	[[nodiscard]] PendingRegion* mask() const override;
	void setMask(PendingRegion* mask) override;

	[[nodiscard]] QsSurfaceFormat surfaceFormat() const override;
	void setSurfaceFormat(QsSurfaceFormat mask) override;

	[[nodiscard]] QQmlListProperty<QObject> data() override;
	// NOLINTEND

private:
	ProxyFloatingWindow* window;
};
