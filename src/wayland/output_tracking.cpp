#include "output_tracking.hpp"

#include <private/qwaylanddisplay_p.h>
#include <private/qwaylandintegration_p.h>
#include <private/qwaylandscreen_p.h>
#include <qguiapplication.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qscreen.h>
#include <qtmetamacros.h>

namespace qs::wayland {

void WlOutputTracker::addOutput(::wl_output* output) {
	auto* display = QtWaylandClient::QWaylandIntegration::instance()->display();

	if (auto* platformScreen = display->screenForOutput(output)) {
		auto* screen = platformScreen->screen();
		this->mScreens.append(screen);
		emit this->screenAdded(screen);
	} else {
		QObject::connect(
		    static_cast<QGuiApplication*>(QGuiApplication::instance()), // NOLINT
		    &QGuiApplication::screenAdded,
		    this,
		    &WlOutputTracker::onQScreenAdded,
		    Qt::UniqueConnection
		);

		this->mOutputs.append(output);
	}
}

void WlOutputTracker::removeOutput(::wl_output* output) {
	auto* display = QtWaylandClient::QWaylandIntegration::instance()->display();

	if (auto* platformScreen = display->screenForOutput(output)) {
		auto* screen = platformScreen->screen();
		this->mScreens.removeOne(screen);
		emit this->screenRemoved(screen);
	} else {
		this->mOutputs.removeOne(output);

		if (this->mOutputs.isEmpty()) {
			QObject::disconnect(
			    static_cast<QGuiApplication*>(QGuiApplication::instance()), // NOLINT
			    nullptr,
			    this,
			    nullptr
			);
		}
	}
}

void WlOutputTracker::onQScreenAdded(QScreen* screen) {
	if (auto* platformScreen = dynamic_cast<QtWaylandClient::QWaylandScreen*>(screen->handle())) {
		if (this->mOutputs.removeOne(platformScreen->output())) {
			this->mScreens.append(screen);
			emit this->screenAdded(screen);

			if (this->mOutputs.isEmpty()) {
				QObject::disconnect(
				    static_cast<QGuiApplication*>(QGuiApplication::instance()), // NOLINT
				    nullptr,
				    this,
				    nullptr
				);
			}
		}
	}
}

} // namespace qs::wayland
