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

class ActivationToken
    : public QObject
    , public QtWayland::xdg_activation_token_v1 {
public:
	ActivationToken(::xdg_activation_token_v1* token, QtWaylandClient::QWaylandWindow* window)
	    : QObject(window)
	    , QtWayland::xdg_activation_token_v1(token)
	    , window(window) {}

	~ActivationToken() override { this->destroy(); }
	Q_DISABLE_COPY_MOVE(ActivationToken);

private:
	void xdg_activation_token_v1_done(const QString& token) override {
		this->deleteLater();
		if (this->window) emit this->window->xdgActivationTokenCreated(token);
	}

	QPointer<QtWaylandClient::QWaylandWindow> window;
};

} // namespace

bool requestActivationToken(std::function<void(const QString&)> callback) {
	if (QGuiApplication::platformName() != "wayland") return false;

	auto* display = QtWaylandClient::QWaylandIntegration::instance()->display();
	auto* window = display->lastInputWindow();
	if (!window || !window->surface() || !display->lastInputDevice()) return false;

	QObject::connect(
	    window,
	    &QtWaylandClient::QWaylandWindow::xdgActivationTokenCreated,
	    window,
	    std::move(callback),
	    Qt::SingleShotConnection
	);
	window->requestXdgActivationToken(display->lastInputSerial());
	return true;
}

void requestXdgActivationToken(QtWaylandClient::QWaylandWindow* window, quint32 serial) {
	auto* display = window->display();
	auto* seat = display->lastInputDevice();

	static auto* manager = new ActivationManager(); // NOLINT
	if (!manager->isActive() || !window->surface() || !seat) {
		emit window->xdgActivationTokenCreated({});
		return;
	}

	auto* token = new ActivationToken(manager->get_activation_token(), window);
	token->set_serial(serial, seat->object());
	token->set_surface(window->surface());
	token->commit();
}

} // namespace qs::wayland::activation
