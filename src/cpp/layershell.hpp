#pragma once

#include <LayerShellQt/window.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qquickwindow.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <qwindow.h>

#include "proxywindow.hpp"
#include "qmlscreen.hpp"

class Anchors {
	Q_GADGET;
	Q_PROPERTY(bool left MEMBER mLeft);
	Q_PROPERTY(bool right MEMBER mRight);
	Q_PROPERTY(bool top MEMBER mTop);
	Q_PROPERTY(bool bottom MEMBER mBottom);

public:
	bool mLeft = false;
	bool mRight = false;
	bool mTop = false;
	bool mBottom = false;
};

class Margins {
	Q_GADGET;
	Q_PROPERTY(qint32 left MEMBER mLeft);
	Q_PROPERTY(qint32 right MEMBER mRight);
	Q_PROPERTY(qint32 top MEMBER mTop);
	Q_PROPERTY(qint32 bottom MEMBER mBottom);

public:
	qint32 mLeft = 0;
	qint32 mRight = 0;
	qint32 mTop = 0;
	qint32 mBottom = 0;
};

namespace ExclusionMode { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	/// Respect the exclusion zone of other shell layers and optionally set one
	Normal = 0,
	/// Ignore exclusion zones of other shell layers. You cannot set an exclusion zone in this mode.
	Ignore = 1,
	/// Decide the exclusion zone based on the window dimensions and anchors.
	///
	/// Will attempt to reseve exactly enough space for the window and its margins if
	/// exactly 3 anchors are connected.
	Auto = 2,
};
Q_ENUM_NS(Enum);

} // namespace ExclusionMode

namespace Layer { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	Background = 0,
	Bottom = 1,
	Top = 2,
	Overlay = 3,
};
Q_ENUM_NS(Enum);

} // namespace Layer

/// Type of keyboard focus that will be accepted by a [ProxyShellWindow]
///
/// [ProxyShellWindow]: ../proxyshellwindow
namespace KeyboardFocus { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	/// No keyboard input will be accepted.
	None = 0,
	/// Exclusive access to the keyboard, locking out all other windows.
	Exclusive = 1,
	/// Access to the keyboard as determined by the operating system.
	///
	/// > [!WARNING] On some systems, `OnDemand` may cause the shell window to
	/// > retain focus over another window unexpectedly.
	/// > You should try `None` if you experience issues.
	OnDemand = 2,
};
Q_ENUM_NS(Enum);

} // namespace KeyboardFocus

namespace ScreenConfiguration { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	Window = 0,
	Compositor = 1,
};
Q_ENUM_NS(Enum);

} // namespace ScreenConfiguration

///! Decorationless window attached to screen edges by anchors.
/// Decorationless window attached to screen edges by anchors.
///
/// #### Example
/// The following snippet creates a white bar attached to the bottom of [TODO] screen.
///
/// ```qml
/// ProxyShellWindow {
///   anchors {
///     left: true
///     bottom: true
///     right: true
///   }
///
///   Text {
///     anchors.horizontalCenter: parent.horizontalCenter
///     anchors.verticalCenter: parent.verticalCenter
///     text: "Hello!"
///   }
/// }
/// ```
class ProxyShellWindow: public ProxyWindowBase {
	// clang-format off
	Q_OBJECT;
	/// The screen that the shell window currently occupies.
	///
	/// > [!INFO] This cannot be changed while the shell window is visible.
	Q_PROPERTY(QuickShellScreenInfo* screen READ screen WRITE setScreen NOTIFY screenChanged);
	/// Anchors attach a shell window to the sides of the screen.
	/// By default all anchors are disabled to avoid blocking the entire screen due to a misconfiguration.
	///
	/// > [!INFO] When two opposite anchors are attached at the same time, the corrosponding dimension
	/// > (width or height) will be forced to equal the screen width/height.
	/// > Margins can be used to create anchored windows that are also disconnected from the monitor sides.
	Q_PROPERTY(Anchors anchors READ anchors WRITE setAnchors NOTIFY anchorsChanged);
	/// The amount of space reserved for the shell layer relative to its anchors.
	///
	/// > [!INFO] Some systems will require exactly 3 anchors to be attached for the exclusion zone to take
	/// > effect.
	Q_PROPERTY(qint32 exclusionZone READ exclusiveZone WRITE setExclusiveZone NOTIFY exclusionZoneChanged);
	/// Defaults to `ExclusionMode.Normal`.
	Q_PROPERTY(ExclusionMode::Enum exclusionMode READ exclusionMode WRITE setExclusionMode NOTIFY exclusionModeChanged);
	/// Offsets from the sides of the screen.
	///
	/// > [!INFO] Only applies to edges with anchors
	Q_PROPERTY(Margins margins READ margins WRITE setMargins NOTIFY marginsChanged);
	/// The shell layer the window sits in. Defaults to `Layer.Top`.
	Q_PROPERTY(Layer::Enum layer READ layer WRITE setLayer NOTIFY layerChanged);
	Q_PROPERTY(QString scope READ scope WRITE setScope);
	/// The degree of keyboard focus taken. Defaults to `KeyboardFocus.None`.
	Q_PROPERTY(KeyboardFocus::Enum keyboardFocus READ keyboardFocus WRITE setKeyboardFocus NOTIFY keyboardFocusChanged);
	Q_PROPERTY(ScreenConfiguration::Enum screenConfiguration READ screenConfiguration WRITE setScreenConfiguration);
	Q_PROPERTY(bool closeOnDismissed READ closeOnDismissed WRITE setCloseOnDismissed);
	QML_ELEMENT;
	// clang-format on

protected:
	void earlyInit(QObject* old) override;

public:
	void componentComplete() override;
	QQuickWindow* disownWindow() override;

	QQmlListProperty<QObject> data();

	void setVisible(bool visible) override;
	bool isVisible() override;

	void setWidth(qint32 width) override;
	qint32 width() override;

	void setHeight(qint32 height) override;
	qint32 height() override;

	void setScreen(QuickShellScreenInfo* screen);
	[[nodiscard]] QuickShellScreenInfo* screen() const;

	void setAnchors(Anchors anchors);
	[[nodiscard]] Anchors anchors() const;

	void setExclusiveZone(qint32 zone);
	[[nodiscard]] qint32 exclusiveZone() const;

	void setExclusionMode(ExclusionMode::Enum exclusionMode);
	[[nodiscard]] ExclusionMode::Enum exclusionMode() const;

	void setMargins(Margins margins);
	[[nodiscard]] Margins margins() const;

	void setLayer(Layer::Enum layer);
	[[nodiscard]] Layer::Enum layer() const;

	void setScope(const QString& scope);
	[[nodiscard]] QString scope() const;

	void setKeyboardFocus(KeyboardFocus::Enum focus);
	[[nodiscard]] KeyboardFocus::Enum keyboardFocus() const;

	void setScreenConfiguration(ScreenConfiguration::Enum configuration);
	[[nodiscard]] ScreenConfiguration::Enum screenConfiguration() const;

	void setCloseOnDismissed(bool close);
	[[nodiscard]] bool closeOnDismissed() const;

signals:
	void screenChanged();
	void anchorsChanged();
	void marginsChanged();
	void exclusionZoneChanged();
	void exclusionModeChanged();
	void layerChanged();
	void keyboardFocusChanged();

private slots:
	void updateExclusionZone();

private:
	LayerShellQt::Window* shellWindow = nullptr;
	bool anchorsInitialized = false;
	ExclusionMode::Enum mExclusionMode = ExclusionMode::Normal;
	qint32 requestedExclusionZone = 0;

	// needed to ensure size dosent fuck up when changing layershell attachments
	// along with setWidth and setHeight overrides
	qint32 requestedWidth = 100;
	qint32 requestedHeight = 100;

	// width/height must be set before anchors, so we have to track anchors and apply them late
	bool complete = false;
	bool stagingVisible = false;
	Anchors stagingAnchors;
};
