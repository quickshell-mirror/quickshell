#pragma once

#include <memory>

#include <private/qwaylandwindow_p.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qwindow.h>

#include "../../core/region.hpp"
#include "../../window/proxywindow.hpp"
#include "surface.hpp"

namespace qs::wayland::background_effect {

///! Background blur effect for Wayland surfaces.
/// Applies background blur behind a @@Quickshell.QsWindow or subclass,
/// as an attached object, using the [ext-background-effect-v1] Wayland protocol.
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
///   BackgroundEffect.blurRegion: Region { item: root.contentItem }
/// }
/// ```
class BackgroundEffect: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Region to blur behind the surface. Set to null to remove blur.
	Q_PROPERTY(PendingRegion* blurRegion READ blurRegion WRITE setBlurRegion NOTIFY blurRegionChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("BackgroundEffect can only be used as an attached object.");
	QML_ATTACHED(BackgroundEffect);

public:
	explicit BackgroundEffect(ProxyWindowBase* window);

	[[nodiscard]] PendingRegion* blurRegion() const;
	void setBlurRegion(PendingRegion* region);

	static BackgroundEffect* qmlAttachedProperties(QObject* object);

signals:
	void blurRegionChanged();

private slots:
	void onWindowConnected();
	void onWindowVisibleChanged();
	void onWaylandWindowDestroyed();
	void onWaylandSurfaceCreated();
	void onWaylandSurfaceDestroyed();
	void onProxyWindowDestroyed();
	void onBlurRegionDestroyed();
	void onWindowPolished();
	void updateBlurRegion();

private:
	ProxyWindowBase* proxyWindow = nullptr;
	QWindow* mWindow = nullptr;
	QtWaylandClient::QWaylandWindow* mWaylandWindow = nullptr;

	bool pendingBlurRegion = false;
	PendingRegion* mBlurRegion = nullptr;
	std::unique_ptr<impl::BackgroundEffectSurface> surface;
};

} // namespace qs::wayland::background_effect
