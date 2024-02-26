#pragma once

#include <qobject.h>
#include <qtmetamacros.h>

#include "proxywindow.hpp"

class ProxyFloatingWindow: public ProxyWindowBase {
	Q_OBJECT;

public:
	explicit ProxyFloatingWindow(QObject* parent = nullptr): ProxyWindowBase(parent) {}

	// Setting geometry while the window is visible makes the content item shrinks but not the window
	// which is awful so we disable it for floating windows.
	void setWidth(qint32 width) override;
	void setHeight(qint32 height) override;
};

///! Standard floating window.
class FloatingWindowInterface: public WindowInterface {
	Q_OBJECT;
	QML_NAMED_ELEMENT(FloatingWindow);

public:
	explicit FloatingWindowInterface(QObject* parent = nullptr);

	void onReload(QObject* oldInstance) override;

	[[nodiscard]] QQuickItem* contentItem() const override;

	// NOLINTBEGIN
	[[nodiscard]] bool isVisible() const override;
	void setVisible(bool visible) override;

	[[nodiscard]] qint32 width() const override;
	void setWidth(qint32 width) override;

	[[nodiscard]] qint32 height() const override;
	void setHeight(qint32 height) override;

	[[nodiscard]] QuickshellScreenInfo* screen() const override;
	void setScreen(QuickshellScreenInfo* screen) override;

	[[nodiscard]] QColor color() const override;
	void setColor(QColor color) override;

	[[nodiscard]] PendingRegion* mask() const override;
	void setMask(PendingRegion* mask) override;

	[[nodiscard]] QQmlListProperty<QObject> data() override;
	// NOLINTEND

private:
	ProxyFloatingWindow* window;
};
