#pragma once

#include <LayerShellQt/window.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "shellwindow.hpp"

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

/// Type of keyboard focus that will be accepted by a [ShellWindow]
///
/// [ShellWindow]: ../shellwindow
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

class WaylandShellWindowExtensions;

class WaylandShellWindow: public ProxyShellWindow {
	Q_OBJECT;
	Q_PROPERTY(WaylandShellWindowExtensions* wayland MEMBER mWayland CONSTANT);
	QML_NAMED_ELEMENT(ShellWindow);

public:
	explicit WaylandShellWindow(QObject* parent = nullptr);

	WaylandShellWindowExtensions* wayland();

	void setupWindow() override;
	QQuickWindow* disownWindow() override;

	void setWidth(qint32 width) override;
	void setHeight(qint32 height) override;

	void setAnchors(Anchors anchors) override;
	[[nodiscard]] Anchors anchors() const override;

	void setExclusiveZone(qint32 zone) override;
	[[nodiscard]] qint32 exclusiveZone() const override;

	void setExclusionMode(ExclusionMode::Enum exclusionMode) override;
	[[nodiscard]] ExclusionMode::Enum exclusionMode() const override;

	void setMargins(Margins margins) override;
	[[nodiscard]] Margins margins() const override;

protected slots:
	void updateExclusionZone();

private:
	WaylandShellWindowExtensions* mWayland = nullptr;

	LayerShellQt::Window* shellWindow = nullptr;
	Layer::Enum mLayer = Layer::Top;
	QString mScope;
	KeyboardFocus::Enum mKeyboardFocus = KeyboardFocus::None;
	ScreenConfiguration::Enum mScreenConfiguration = ScreenConfiguration::Window;

	bool connected = false;

	friend class WaylandShellWindowExtensions;
};

class WaylandShellWindowExtensions: public QObject {
	Q_OBJECT;
	/// The shell layer the window sits in. Defaults to `Layer.Top`.
	Q_PROPERTY(Layer::Enum layer READ layer WRITE setLayer NOTIFY layerChanged);
	Q_PROPERTY(QString scope READ scope WRITE setScope);
	/// The degree of keyboard focus taken. Defaults to `KeyboardFocus.None`.
	Q_PROPERTY(KeyboardFocus::Enum keyboardFocus READ keyboardFocus WRITE setKeyboardFocus NOTIFY
	               keyboardFocusChanged);
	Q_PROPERTY(ScreenConfiguration::Enum screenConfiguration READ screenConfiguration WRITE
	               setScreenConfiguration);
	QML_ELEMENT;
	QML_UNCREATABLE("WaylandShellWindowExtensions cannot be created");

public:
	explicit WaylandShellWindowExtensions(WaylandShellWindow* window):
	    QObject(window), window(window) {}

	void setLayer(Layer::Enum layer);
	[[nodiscard]] Layer::Enum layer() const;

	void setScope(const QString& scope);
	[[nodiscard]] QString scope() const;

	void setKeyboardFocus(KeyboardFocus::Enum focus);
	[[nodiscard]] KeyboardFocus::Enum keyboardFocus() const;

	void setScreenConfiguration(ScreenConfiguration::Enum configuration);
	[[nodiscard]] ScreenConfiguration::Enum screenConfiguration() const;

signals:
	void layerChanged();
	void keyboardFocusChanged();

private:
	WaylandShellWindow* window;

	friend class WaylandShellWindow;
};
