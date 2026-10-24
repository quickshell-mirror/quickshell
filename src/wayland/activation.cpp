#include "activation.hpp"
#include <functional>
#include <utility>

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandinputdevice_p.h>
#include <private/qwaylandintegration_p.h>
#include <private/qwaylandwindow_p.h>
#include <qguiapplication.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpointer.h>
#include <qstring.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwayland-xdg-activation-v1.h>
#include <qwaylandclientextension.h>

#include "wayland-xdg-activation-v1-client-protocol.h"

namespace qs::wayland::activation {
namespace {

class ActivationManager
    : public QWaylandClientExtensionTemplate<ActivationManager>
    , public QtWayland::xdg_activation_v1 {
public:
	ActivationManager(): QWaylandClientExtensionTemplate(1) { this->initialize(); }
};

// One token request with its own callback. It isn't owned by the window, so the token still
// arrives if the window is closed or destroyed before the compositor answers.
class ActivationToken
    : public QObject
    , public QtWayland::xdg_activation_token_v1 {
public:
	ActivationToken(::xdg_activation_token_v1* token, std::function<void(const QString&)> callback)
	    : QtWayland::xdg_activation_token_v1(token)
	    , callback(std::move(callback)) {}

	~ActivationToken() override { this->destroy(); }
	Q_DISABLE_COPY_MOVE(ActivationToken);

private:
	void xdg_activation_token_v1_done(const QString& token) override {
		this->deleteLater();
		this->callback(token);
	}

	std::function<void(const QString&)> callback;
};

bool sendRequest(
    QtWaylandClient::QWaylandWindow* window,
    quint32 serial,
    std::function<void(const QString&)> callback
) {
	auto* seat = window->display()->lastInputDevice();

	static auto* manager = new ActivationManager(); // NOLINT
	if (!manager->isActive() || !window->surface() || !seat) return false;

	auto* token = new ActivationToken(manager->get_activation_token(), std::move(callback));
	token->set_serial(serial, seat->object());
	token->set_surface(window->surface());
	token->commit();
	return true;
}

} // namespace

bool requestActivationToken(std::function<void(const QString&)> callback) {
	if (QGuiApplication::platformName() != "wayland") return false;

	auto* display = QtWaylandClient::QWaylandIntegration::instance()->display();
	auto* window = display->lastInputWindow();
	if (!window) return false;

	return sendRequest(window, display->lastInputSerial(), std::move(callback));
}

void requestXdgActivationToken(QtWaylandClient::QWaylandWindow* window, quint32 serial) {
	const auto callback = [window = QPointer(window)](const QString& token) {
		if (window) emit window->xdgActivationTokenCreated(token);
	};

	if (!sendRequest(window, serial, callback)) emit window->xdgActivationTokenCreated({});
}

} // namespace qs::wayland::activation
