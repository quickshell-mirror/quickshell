#pragma once

#include <qcontainerfwd.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qsize.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/doc.hpp"
#include "../../core/util.hpp"
#include "../../window/panelinterface.hpp"
#include "../../window/proxywindow.hpp"

namespace qs::wayland::layershell {

struct LayerSurfaceState;
class LayerSurfaceBridge;

///! WlrLayershell layer.
/// See @@WlrLayershell.layer.
namespace WlrLayer { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum : quint8 {
	/// Below bottom
	Background = 0,
	/// Above background, usually below windows
	Bottom = 1,
	/// Commonly used for panels, app launchers, and docks.
	/// Usually renders over normal windows and below fullscreen windows.
	Top = 2,
	/// Usually renders over fullscreen windows
	Overlay = 3,
};
Q_ENUM_NS(Enum);

} // namespace WlrLayer

///! WlrLayershell keyboard focus mode
/// See @@WlrLayershell.keyboardFocus.
namespace WlrKeyboardFocus { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum : quint8 {
	/// No keyboard input will be accepted.
	None = 0,
	/// Exclusive access to the keyboard, locking out all other windows.
	///
	/// > [!WARNING] You **CANNOT** use this to make a secure lock screen.
	/// >
	/// > If you want to make a lock screen, use @@WlSessionLock.
	Exclusive = 1,
	/// Access to the keyboard as determined by the operating system.
	///
	/// > [!WARNING] On some systems, `OnDemand` may cause the shell window to
	/// > retain focus over another window unexpectedly.
	/// > You should try `None` if you experience issues.
	OnDemand = 2,
};
Q_ENUM_NS(Enum);

} // namespace WlrKeyboardFocus

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
	Q_PROPERTY(qs::wayland::layershell::WlrLayer::Enum layer READ layer WRITE setLayer NOTIFY layerChanged);
	/// Similar to the class property of windows. Can be used to identify the window to external tools.
	///
	/// Cannot be set after windowConnected.
	Q_PROPERTY(QString namespace READ ns WRITE setNamespace NOTIFY namespaceChanged);
	/// The degree of keyboard focus taken. Defaults to `KeyboardFocus.None`.
	Q_PROPERTY(qs::wayland::layershell::WlrKeyboardFocus::Enum keyboardFocus READ keyboardFocus WRITE setKeyboardFocus NOTIFY keyboardFocusChanged);

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
	ProxiedWindow* disownWindow(bool keepItemOwnership = false) override;
	[[nodiscard]] bool deleteOnInvisible() const override;

	void onPolished() override;
	void trySetWidth(qint32 implicitWidth) override;
	void trySetHeight(qint32 implicitHeight) override;

	void setScreen(QuickshellScreenInfo* screen) override;

	[[nodiscard]] WlrLayer::Enum layer() const { return this->bLayer; }
	void setLayer(WlrLayer::Enum layer) { this->bLayer = layer; }

	[[nodiscard]] QString ns() const { return this->bNamespace; }
	void setNamespace(const QString& ns) { this->bNamespace = ns; }

	[[nodiscard]] WlrKeyboardFocus::Enum keyboardFocus() const { return this->bKeyboardFocus; }
	void setKeyboardFocus(WlrKeyboardFocus::Enum focus) { this->bKeyboardFocus = focus; }

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
	void onBridgeDestroyed();

private:
	[[nodiscard]] LayerSurfaceState computeState() const;

	void connectBridge(LayerSurfaceBridge* bridge);
	void onStateChanged();

	bool compositorPicksScreen = true;
	LayerSurfaceBridge* bridge = nullptr;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WlrLayershell, WlrLayer::Enum, bLayer, WlrLayer::Top, &WlrLayershell::layerChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WlrLayershell, QString, bNamespace, "quickshell", &WlrLayershell::namespaceChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WlrLayershell, Anchors, bAnchors, &WlrLayershell::anchorsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WlrLayershell, Margins, bMargins, &WlrLayershell::marginsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WlrLayershell, qint32, bExclusiveZone, &WlrLayershell::exclusiveZoneChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WlrLayershell, WlrKeyboardFocus::Enum, bKeyboardFocus, &WlrLayershell::keyboardFocusChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WlrLayershell, ExclusionMode::Enum, bExclusionMode, ExclusionMode::Auto, &WlrLayershell::exclusionModeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(WlrLayershell, qint32, bcExclusiveZone);
	Q_OBJECT_BINDABLE_PROPERTY(WlrLayershell, Qt::Edge, bcExclusionEdge);

	QS_BINDING_SUBSCRIBE_METHOD(WlrLayershell, bLayer, onStateChanged, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(WlrLayershell, bAnchors, onStateChanged, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(WlrLayershell, bMargins, onStateChanged, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(WlrLayershell, bcExclusiveZone, onStateChanged, onValueChanged);
	QS_BINDING_SUBSCRIBE_METHOD(WlrLayershell, bKeyboardFocus, onStateChanged, onValueChanged);
	// clang-format on
};

class WaylandPanelInterface: public PanelWindowInterface {
	Q_OBJECT;

public:
	explicit WaylandPanelInterface(QObject* parent = nullptr);

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
	WlrLayershell* layer;

	friend class WlrLayershell;
};

} // namespace qs::wayland::layershell
