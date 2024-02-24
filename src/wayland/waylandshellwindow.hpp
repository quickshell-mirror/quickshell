#pragma once

#include <qobject.h>
#include <qqmlengine.h>
#include <qtypes.h>

#include "../core/shellwindow.hpp"
#include "layershell.hpp"

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
	void onWidthChanged() override;
	void onHeightChanged() override;

private:
	WaylandShellWindowExtensions* mWayland = nullptr;

	LayershellWindowExtension* windowExtension;
	bool connected = false;

	friend class WaylandShellWindowExtensions;
};

class WaylandShellWindowExtensions: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The shell layer the window sits in. Defaults to `Layer.Top`.
	Q_PROPERTY(Layer::Enum layer READ layer WRITE setLayer NOTIFY layerChanged);
	/// Similar to the class property of windows. Can be used to identify the window to external tools.
	Q_PROPERTY(QString namespace READ ns WRITE setNamespace);
	/// The degree of keyboard focus taken. Defaults to `KeyboardFocus.None`.
	Q_PROPERTY(KeyboardFocus::Enum keyboardFocus READ keyboardFocus WRITE setKeyboardFocus NOTIFY keyboardFocusChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("WaylandShellWindowExtensions cannot be created");

public:
	explicit WaylandShellWindowExtensions(WaylandShellWindow* window)
	    : QObject(window)
	    , window(window) {}

	void setLayer(Layer::Enum layer);
	[[nodiscard]] Layer::Enum layer() const;

	void setNamespace(const QString& ns);
	[[nodiscard]] QString ns() const;

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
