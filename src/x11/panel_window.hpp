#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qquickwindow.h>
#include <qscreen.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

#include "../core/doc.hpp"
#include "../window/panelinterface.hpp"
#include "../window/proxywindow.hpp"

class XPanelStack;

class XPanelEventFilter: public QObject {
	Q_OBJECT;

public:
	explicit XPanelEventFilter(QObject* parent = nullptr): QObject(parent) {}

signals:
	void surfaceCreated();

protected:
	bool eventFilter(QObject* watched, QEvent* event) override;
};

class XPanelWindow: public ProxyWindowBase {
	QSDOC_BASECLASS(PanelWindowInterface);
	Q_OBJECT;
	// clang-format off
	QSDOC_HIDE Q_PROPERTY(Anchors anchors READ anchors WRITE setAnchors NOTIFY anchorsChanged);
	QSDOC_HIDE Q_PROPERTY(qint32 exclusiveZone READ exclusiveZone WRITE setExclusiveZone NOTIFY exclusiveZoneChanged);
	QSDOC_HIDE Q_PROPERTY(ExclusionMode::Enum exclusionMode READ exclusionMode WRITE setExclusionMode NOTIFY exclusionModeChanged);
	QSDOC_HIDE Q_PROPERTY(Margins margins READ margins WRITE setMargins NOTIFY marginsChanged);
	QSDOC_HIDE Q_PROPERTY(bool aboveWindows READ aboveWindows WRITE setAboveWindows NOTIFY aboveWindowsChanged);
	QSDOC_HIDE Q_PROPERTY(bool focusable READ focusable WRITE setFocusable NOTIFY focusableChanged);
	// clang-format on
	QML_ELEMENT;

public:
	explicit XPanelWindow(QObject* parent = nullptr);
	~XPanelWindow() override;
	Q_DISABLE_COPY_MOVE(XPanelWindow);

	void connectWindow() override;

	void setWidth(qint32 width) override;
	void setHeight(qint32 height) override;

	void setScreen(QuickshellScreenInfo* screen) override;

	[[nodiscard]] Anchors anchors() const;
	void setAnchors(Anchors anchors);

	[[nodiscard]] qint32 exclusiveZone() const;
	void setExclusiveZone(qint32 exclusiveZone);

	[[nodiscard]] ExclusionMode::Enum exclusionMode() const;
	void setExclusionMode(ExclusionMode::Enum exclusionMode);

	[[nodiscard]] Margins margins() const;
	void setMargins(Margins margins);

	[[nodiscard]] bool aboveWindows() const;
	void setAboveWindows(bool aboveWindows);

	[[nodiscard]] bool focusable() const;
	void setFocusable(bool focusable);

signals:
	QSDOC_HIDE void anchorsChanged();
	QSDOC_HIDE void exclusiveZoneChanged();
	QSDOC_HIDE void exclusionModeChanged();
	QSDOC_HIDE void marginsChanged();
	QSDOC_HIDE void aboveWindowsChanged();
	QSDOC_HIDE void focusableChanged();

private slots:
	void xInit();
	void updatePanelStack();
	void updateDimensionsSlot();
	void onScreenVirtualGeometryChanged();

private:
	void connectScreen();
	void getExclusion(int& side, quint32& exclusiveZone);
	void updateStrut(bool propagate = true);
	void updateAboveWindows();
	void updateFocusable();
	void updateDimensions(bool propagate = true);

	QPointer<QScreen> mTrackedScreen = nullptr;
	bool mAboveWindows = true;
	bool mFocusable = false;
	Anchors mAnchors;
	Margins mMargins;
	qint32 mExclusiveZone = 0;
	ExclusionMode::Enum mExclusionMode = ExclusionMode::Auto;

	QRect lastScreenVirtualGeometry;
	XPanelEventFilter eventFilter;

	friend class XPanelStack;
};

class XPanelInterface: public PanelWindowInterface {
	Q_OBJECT;

public:
	explicit XPanelInterface(QObject* parent = nullptr);

	void onReload(QObject* oldInstance) override;

	[[nodiscard]] ProxyWindowBase* proxyWindow() const override;
	[[nodiscard]] QQuickItem* contentItem() const override;

	// NOLINTBEGIN
	[[nodiscard]] bool isVisible() const override;
	[[nodiscard]] bool isBackingWindowVisible() const override;
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

	// panel specific

	[[nodiscard]] Anchors anchors() const override;
	void setAnchors(Anchors anchors) override;

	[[nodiscard]] Margins margins() const override;
	void setMargins(Margins margins) override;

	[[nodiscard]] qint32 exclusiveZone() const override;
	void setExclusiveZone(qint32 exclusiveZone) override;

	[[nodiscard]] ExclusionMode::Enum exclusionMode() const override;
	void setExclusionMode(ExclusionMode::Enum exclusionMode) override;

	[[nodiscard]] bool aboveWindows() const override;
	void setAboveWindows(bool aboveWindows) override;

	[[nodiscard]] bool focusable() const override;
	void setFocusable(bool focusable) override;
	// NOLINTEND

private:
	XPanelWindow* panel;

	friend class WlrLayershell;
};
