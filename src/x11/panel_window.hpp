#pragma once

#include <qnamespace.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qquickwindow.h>
#include <qscreen.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

#include "../core/doc.hpp"
#include "../core/util.hpp"
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

	void trySetWidth(qint32 implicitWidth) override;
	void trySetHeight(qint32 implicitHeight) override;

	void setScreen(QuickshellScreenInfo* screen) override;

	[[nodiscard]] bool aboveWindows() const { return this->bAboveWindows; }
	void setAboveWindows(bool aboveWindows) { this->bAboveWindows = aboveWindows; }

	[[nodiscard]] Anchors anchors() const { return this->bAnchors; }
	void setAnchors(Anchors anchors) { this->bAnchors = anchors; }

	[[nodiscard]] qint32 exclusiveZone() const { return this->bExclusiveZone; }
	void setExclusiveZone(qint32 exclusiveZone) {
		Qt::beginPropertyUpdateGroup();
		this->bExclusiveZone = exclusiveZone;
		this->bExclusionMode = ExclusionMode::Normal;
		Qt::endPropertyUpdateGroup();
	}

	[[nodiscard]] ExclusionMode::Enum exclusionMode() const { return this->bExclusionMode; }
	void setExclusionMode(ExclusionMode::Enum exclusionMode) { this->bExclusionMode = exclusionMode; }

	[[nodiscard]] Margins margins() const { return this->bMargins; }
	void setMargins(Margins margins) { this->bMargins = margins; }

	[[nodiscard]] bool focusable() const { return this->bFocusable; }
	void setFocusable(bool focusable) { this->bFocusable = focusable; }

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
	void updateScreen();
	void updateStrut(bool propagate = true);
	void updateStrutCb() { this->updateStrut(); }
	void updateAboveWindows();
	void updateFocusable();
	void updateDimensions(bool propagate = true);
	void updateDimensionsCb() { this->updateDimensions(); }

	QPointer<QScreen> mTrackedScreen = nullptr;
	EngineGeneration* knownGeneration = nullptr;

	QRect lastScreenVirtualGeometry;
	XPanelEventFilter eventFilter;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(XPanelWindow, bool, bAboveWindows, true, &XPanelWindow::aboveWindowsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(XPanelWindow, bool, bFocusable, &XPanelWindow::focusableChanged);
	Q_OBJECT_BINDABLE_PROPERTY(XPanelWindow, Anchors, bAnchors, &XPanelWindow::anchorsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(XPanelWindow, Margins, bMargins, &XPanelWindow::marginsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(XPanelWindow, qint32, bExclusiveZone, &XPanelWindow::exclusiveZoneChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(XPanelWindow, ExclusionMode::Enum, bExclusionMode, ExclusionMode::Auto, &XPanelWindow::exclusionModeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(XPanelWindow, qint32, bcExclusiveZone);
	Q_OBJECT_BINDABLE_PROPERTY(XPanelWindow, Qt::Edge, bcExclusionEdge);

	QS_BINDING_SUBSCRIBE_METHOD(XPanelWindow, bAboveWindows, updateAboveWindows, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(XPanelWindow, bAnchors, updateDimensionsCb, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(XPanelWindow, bMargins, updateDimensionsCb, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(XPanelWindow, bcExclusiveZone, updateStrutCb, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(XPanelWindow, bFocusable, updateFocusable, onValueChanged);
	// clang-format on

	friend class XPanelStack;
};

class XPanelInterface: public PanelWindowInterface {
	Q_OBJECT;

public:
	explicit XPanelInterface(QObject* parent = nullptr);

	void onReload(QObject* oldInstance) override;

	[[nodiscard]] ProxyWindowBase* proxyWindow() const override;

	// NOLINTBEGIN
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
