#include <functional>
#include <utility>

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandinputdevice_p.h>
#include <private/qwaylandwindow_p.h>
#include <qobject.h>
#include <qstring.h>
#include <qtclasshelpermacros.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwayland-xdg-activation-v1.h>
#include <qwaylandclientextension.h>

#include "surface.hpp"
#include "wayland-xdg-activation-v1-client-protocol.h"

namespace qs::wayland::layershell {
namespace {

class ActivationManager
    : public QWaylandClientExtensionTemplate<ActivationManager>
    , public QtWayland::xdg_activation_v1 {
public:
	ActivationManager(): QWaylandClientExtensionTemplate(1) { this->initialize(); }
};

class ActivationToken
    : public QObject
    , public QtWayland::xdg_activation_token_v1 {
public:
	ActivationToken(
	    ::xdg_activation_token_v1* token,
	    QObject* context,
	    std::function<void(const QString&)> callback
	)
	    : QObject(context)
	    , QtWayland::xdg_activation_token_v1(token)
	    , callback(std::move(callback)) {
		// A compositor that does not answer must not swallow the click.
		QTimer::singleShot(1000, this, [this]() { this->finish({}); });
	}

	~ActivationToken() override { this->destroy(); }
	Q_DISABLE_COPY_MOVE(ActivationToken);

private:
	void xdg_activation_token_v1_done(const QString& token) override { this->finish(token); }

	void finish(const QString& token) {
		auto callback = std::exchange(this->callback, {});
		if (!callback) return;
		this->deleteLater();
		callback(token);
	}

	std::function<void(const QString&)> callback;
};

} // namespace

void LayerSurface::requestXdgActivationToken(quint32 serial) {
	auto* window = this->window();
	auto* display = window->display();
	auto* seat = display->lastInputDevice();

	static auto* manager = new ActivationManager(); // NOLINT
	if (!manager->isActive() || !window->surface() || !seat) {
		this->QWaylandShellSurface::requestXdgActivationToken(serial);
		return;
	}

	auto* token =
	    new ActivationToken(manager->get_activation_token(), this, [window](const QString& token) {
		    emit window->xdgActivationTokenCreated(token);
	    });
	token->set_serial(serial, seat->object());
	token->set_surface(window->surface());
	token->commit();
}

} // namespace qs::wayland::layershell
