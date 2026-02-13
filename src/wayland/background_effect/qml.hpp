#pragma once

#include <memory>

#include <private/qwaylandwindow_p.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qwindow.h>

#include "../../core/region.hpp"
#include "../../window/proxywindow.hpp"
#include "surface.hpp"

namespace qs::wayland::background_effect {

///! Background blur effect for Wayland surfaces.
/// Applies background blur behind a @@Quickshell.QsWindow or subclass
/// using the [ext-background-effect-v1] Wayland protocol.
///
/// > [!NOTE] Using a background effect requires the compositor support the
/// > [ext-background-effect-v1] protocol.
///
/// [ext-background-effect-v1]: https://wayland.app/protocols/ext-background-effect-v1
///
/// #### Example
/// ```qml
/// @@Quickshell.PanelWindow {
///   id: root
///   color: "#80000000"
///
///   BackgroundEffect {
///     window: root
///     enabled: true
///     blurRegion: Region { item: root.contentItem }
///   }
/// }
/// ```
class BackgroundEffect: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	// clang-format off
	/// If the background effect should be enabled. Defaults to false.
	Q_PROPERTY(bool enabled READ default WRITE default NOTIFY enabledChanged BINDABLE bindableEnabled);
	/// The window to apply the background effect to.
	///
	/// Must be set to a non null value to enable the effect.
	Q_PROPERTY(QObject* window READ window WRITE setWindow NOTIFY windowChanged);
	/// Region to blur behind the surface. Set to null to remove blur.
	Q_PROPERTY(PendingRegion* blurRegion READ blurRegion WRITE setBlurRegion NOTIFY blurRegionChanged);
	/// Whether blur is currently active on the window. The effect is active when @@enabled is true,
	/// @@window has a valid Wayland surface, and the compositor supports blur.
	Q_PROPERTY(bool active READ active NOTIFY activeChanged);
	// clang-format on

public:
	BackgroundEffect();
	~BackgroundEffect() override;
	Q_DISABLE_COPY_MOVE(BackgroundEffect);

	[[nodiscard]] QBindable<bool> bindableEnabled() { return &this->bEnabled; }

	[[nodiscard]] QObject* window() const;
	void setWindow(QObject* window);

	[[nodiscard]] PendingRegion* blurRegion() const;
	void setBlurRegion(PendingRegion* region);

	[[nodiscard]] bool active() const;

signals:
	void enabledChanged();
	void windowChanged();
	void blurRegionChanged();
	void activeChanged();

private slots:
	void onWindowDestroyed();
	void onWindowVisibilityChanged();
	void onWaylandWindowDestroyed();
	void onWaylandSurfaceCreated();
	void onWaylandSurfaceDestroyed();
	void onWindowPolished();

private:
	void onBoundWindowChanged();
	void updateBlurRegion();
	void updateActive();

	ProxyWindowBase* proxyWindow = nullptr;
	QWindow* mWindow = nullptr;
	QtWaylandClient::QWaylandWindow* mWaylandWindow = nullptr;

	PendingRegion* mBlurRegion = nullptr;
	bool pendingBlurRegion = false;
	bool mActive = false;

	std::unique_ptr<impl::BackgroundEffectSurface> surface;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(BackgroundEffect, bool, bEnabled, &BackgroundEffect::enabledChanged);
	Q_OBJECT_BINDABLE_PROPERTY(BackgroundEffect, QObject*, bWindowObject, &BackgroundEffect::windowChanged);
	Q_OBJECT_BINDABLE_PROPERTY(BackgroundEffect, QObject*, bBoundWindow, &BackgroundEffect::onBoundWindowChanged);
	// clang-format on
};

} // namespace qs::wayland::background_effect
