#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/proxywindow.hpp"
#include "layershell.hpp"

class WaylandLayershell: public ProxyWindowBase {
	// clang-format off
	Q_OBJECT;
	/// The shell layer the window sits in. Defaults to `Layer.Top`.
	Q_PROPERTY(Layer::Enum layer READ layer WRITE setLayer NOTIFY layerChanged);
	/// Similar to the class property of windows. Can be used to identify the window to external tools.
	///
	/// Cannot be set after windowConnected.
	Q_PROPERTY(QString namespace READ ns WRITE setNamespace);
	/// The degree of keyboard focus taken. Defaults to `KeyboardFocus.None`.
	Q_PROPERTY(KeyboardFocus::Enum keyboardFocus READ keyboardFocus WRITE setKeyboardFocus NOTIFY keyboardFocusChanged);

	Q_PROPERTY(Anchors anchors READ anchors WRITE setAnchors NOTIFY anchorsChanged);
	Q_PROPERTY(qint32 exclusiveZone READ exclusiveZone WRITE setExclusiveZone NOTIFY exclusiveZoneChanged);
	Q_PROPERTY(ExclusionMode::Enum exclusionMode READ exclusionMode WRITE setExclusionMode NOTIFY exclusionModeChanged);
	Q_PROPERTY(Margins margins READ margins WRITE setMargins NOTIFY marginsChanged);
	QML_ELEMENT;
	// clang-format on

public:
	explicit WaylandLayershell(QObject* parent = nullptr);

	void setupWindow() override;

	void setWidth(qint32 width) override;
	void setHeight(qint32 height) override;

	void setScreen(QuickShellScreenInfo* screen) override;

	[[nodiscard]] Layer::Enum layer() const;
	void setLayer(Layer::Enum layer); // NOLINT

	[[nodiscard]] QString ns() const;
	void setNamespace(QString ns);

	[[nodiscard]] KeyboardFocus::Enum keyboardFocus() const;
	void setKeyboardFocus(KeyboardFocus::Enum focus); // NOLINT

	[[nodiscard]] Anchors anchors() const;
	void setAnchors(Anchors anchors);

	[[nodiscard]] qint32 exclusiveZone() const;
	void setExclusiveZone(qint32 exclusiveZone);

	[[nodiscard]] ExclusionMode::Enum exclusionMode() const;
	void setExclusionMode(ExclusionMode::Enum exclusionMode);

	[[nodiscard]] Margins margins() const;
	void setMargins(Margins margins); // NOLINT

signals:
	void layerChanged();
	void namespaceChanged();
	void keyboardFocusChanged();
	void anchorsChanged();
	void exclusiveZoneChanged();
	void exclusionModeChanged();
	void marginsChanged();

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
	QML_NAMED_ELEMENT(PanelWindow); // temp

public:
	explicit WaylandPanelInterface(QObject* parent = nullptr);

	void onReload(QObject* oldInstance) override;

	[[nodiscard]] QQuickItem* contentItem() const override;

	// NOLINTBEGIN
	[[nodiscard]] bool isVisible() const override;
	void setVisible(bool visible) override;

	[[nodiscard]] qint32 width() const override;
	void setWidth(qint32 width) override;

	[[nodiscard]] qint32 height() const override;
	void setHeight(qint32 height) override;

	[[nodiscard]] QuickShellScreenInfo* screen() const override;
	void setScreen(QuickShellScreenInfo* screen) override;

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
	// NOLINTEND

private:
	WaylandLayershell* layer;
};
