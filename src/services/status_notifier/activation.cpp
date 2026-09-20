#include "activation.hpp"
#include <functional>
#include <utility>

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandintegration_p.h>
#include <private/qwaylandwindow_p.h>
#include <qguiapplication.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpointer.h>
#include <qstring.h>
#include <qtclasshelpermacros.h>
#include <qtimer.h>

namespace qs::service::sni {
namespace {

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
		this->setObjectName("qs-tray-activation-request");
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
	if (window->findChild<QObject*>("qs-tray-activation-request", Qt::FindDirectChildrenOnly)) {
		return false;
	}

	new ActivationRequest(window, context, std::move(callback));
	window->requestXdgActivationToken(display->lastInputSerial());
	return true;
}

} // namespace qs::service::sni
