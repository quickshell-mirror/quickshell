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
#include <qtimer.h>
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
		emit this->window->xdgActivationTokenCreated(token);
	}

	QtWaylandClient::QWaylandWindow* window;
};

class ActivationRequest: public QObject {
public:
	ActivationRequest(
	    QtWaylandClient::QWaylandWindow* window,
	    QObject* context,
	    std::function<void(const QString&)> callback
	)
	    : QObject(window)
	    , context(context)
	    , callback(std::move(callback)) {
		this->setObjectName("qs-xdg-activation-request");
		QObject::connect(
		    window,
		    &QtWaylandClient::QWaylandWindow::xdgActivationTokenCreated,
		    this,
		    [this](const QString& token) {
			    this->deleteLater();
			    this->finish(token);
		    }
		);
		// Keep the request until Qt responds, even after timing out. Its signal has
		// no request ID, so a late token must not be used for a subsequent click.
		QTimer::singleShot(1000, this, [this]() { this->finish({}); });
	}

	~ActivationRequest() override { this->finish({}); }
	Q_DISABLE_COPY_MOVE(ActivationRequest);

private:
	void finish(const QString& token) {
		auto callback = std::exchange(this->callback, {});
		if (this->context && callback) callback(token);
	}

	QPointer<QObject> context;
	std::function<void(const QString&)> callback;
};

} // namespace

bool requestActivationToken(QObject* context, std::function<void(const QString&)> callback) {
	if (QGuiApplication::platformName() != "wayland") return false;

	auto* display = QtWaylandClient::QWaylandIntegration::instance()->display();
	auto* window = display->lastInputWindow();
	if (!window || !window->surface() || !display->lastInputDevice()) return false;

	// Only one request can be matched to the window's token-created signal.
	if (window->findChild<QObject*>("qs-xdg-activation-request", Qt::FindDirectChildrenOnly)) {
		return false;
	}

	new ActivationRequest(window, context, std::move(callback));
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
