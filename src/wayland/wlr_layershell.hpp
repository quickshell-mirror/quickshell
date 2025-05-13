#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/doc.hpp"
#include "../window/panelinterface.hpp"
#include "../window/proxywindow.hpp"
#include "wlr_layershell/window.hpp"

///! Wlroots layershell window
/// Decorationless window that can be attached to the screen edges using the [zwlr_layer_shell_v1] protocol.
///
/// #### Attached object
/// `WlrLayershell` works as an attached object of @@Quickshell.PanelWindow which you should use instead if you can,
/// as it is platform independent.
///
/// ```qml
/// PanelWindow {
///   // When PanelWindow is backed with WlrLayershell this will work
///   WlrLayershell.layer: WlrLayer.Bottom
/// }
/// ```
///
/// To maintain platform compatibility you can dynamically set layershell specific properties.
/// ```qml
/// PanelWindow {
///   Component.onCompleted: {
///     if (this.WlrLayershell != null) {
///       this.WlrLayershell.layer = WlrLayer.Bottom;
///     }
///   }
/// }
/// ```
///
/// [zwlr_layer_shell_v1]: https://wayland.app/protocols/wlr-layer-shell-unstable-v1
class WlrLayershell: public ProxyWindowBase {
	QSDOC_BASECLASS(PanelWindowInterface);
	// clang-format off
	Q_OBJECT;
	/// The shell layer the window sits in. Defaults to `WlrLayer.Top`.
	Q_PROPERTY(WlrLayer::Enum layer READ layer WRITE setLayer NOTIFY layerChanged);
	/// Similar to the class property of windows. Can be used to identify the window to external tools.
	///
	/// Cannot be set after windowConnected.
	Q_PROPERTY(QString namespace READ ns WRITE setNamespace NOTIFY namespaceChanged);
	/// The degree of keyboard focus taken. Defaults to `KeyboardFocus.None`.
	Q_PROPERTY(WlrKeyboardFocus::Enum keyboardFocus READ keyboardFocus WRITE setKeyboardFocus NOTIFY keyboardFocusChanged);

	QSDOC_HIDE Q_PROPERTY(Anchors anchors READ anchors WRITE setAnchors NOTIFY anchorsChanged);
	QSDOC_HIDE Q_PROPERTY(qint32 exclusiveZone READ exclusiveZone WRITE setExclusiveZone NOTIFY exclusiveZoneChanged);
	QSDOC_HIDE Q_PROPERTY(ExclusionMode::Enum exclusionMode READ exclusionMode WRITE setExclusionMode NOTIFY exclusionModeChanged);
	QSDOC_HIDE Q_PROPERTY(Margins margins READ margins WRITE setMargins NOTIFY marginsChanged);
	QSDOC_HIDE Q_PROPERTY(bool aboveWindows READ aboveWindows WRITE setAboveWindows NOTIFY layerChanged);
	QSDOC_HIDE Q_PROPERTY(bool focusable READ focusable WRITE setFocusable NOTIFY keyboardFocusChanged);
	QML_ATTACHED(WlrLayershell);
	QML_ELEMENT;
	// clang-format on

public:
	explicit WlrLayershell(QObject* parent = nullptr);

	ProxiedWindow* retrieveWindow(QObject* oldInstance) override;
	ProxiedWindow* createQQuickWindow() override;
	void connectWindow() override;
	[[nodiscard]] bool deleteOnInvisible() const override;

	void trySetWidth(qint32 implicitWidth) override;
	void trySetHeight(qint32 implicitHeight) override;

	void setScreen(QuickshellScreenInfo* screen) override;

	[[nodiscard]] WlrLayer::Enum layer() const;
	void setLayer(WlrLayer::Enum layer); // NOLINT

	[[nodiscard]] QString ns() const;
	void setNamespace(QString ns);

	[[nodiscard]] WlrKeyboardFocus::Enum keyboardFocus() const;
	void setKeyboardFocus(WlrKeyboardFocus::Enum focus); // NOLINT

	[[nodiscard]] Anchors anchors() const;
	void setAnchors(Anchors anchors);

	[[nodiscard]] qint32 exclusiveZone() const;
	void setExclusiveZone(qint32 exclusiveZone);

	[[nodiscard]] ExclusionMode::Enum exclusionMode() const;
	void setExclusionMode(ExclusionMode::Enum exclusionMode);

	[[nodiscard]] Margins margins() const;
	void setMargins(Margins margins); // NOLINT

	[[nodiscard]] bool aboveWindows() const;
	void setAboveWindows(bool aboveWindows);

	[[nodiscard]] bool focusable() const;
	void setFocusable(bool focusable);

	static WlrLayershell* qmlAttachedProperties(QObject* object);

signals:
	void layerChanged();
	void namespaceChanged();
	void keyboardFocusChanged();
	QSDOC_HIDE void anchorsChanged();
	QSDOC_HIDE void exclusiveZoneChanged();
	QSDOC_HIDE void exclusionModeChanged();
	QSDOC_HIDE void marginsChanged();

private slots:
	void updateAutoExclusion();

private:
	void setAutoExclusion();

	LayershellWindowExtension* ext;

	ExclusionMode::Enum mExclusionMode = ExclusionMode::Auto;
	qint32 mExclusiveZone = 0;
};

class WaylandPanelInterface: public PanelWindowInterface {
	Q_OBJECT;

public:
	explicit WaylandPanelInterface(QObject* parent = nullptr);

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
	WlrLayershell* layer;

	friend class WlrLayershell;
};
