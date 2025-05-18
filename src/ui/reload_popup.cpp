#include "reload_popup.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qtenvironmentvariables.h>
#include <qtimer.h>

#include "../core/generation.hpp"

namespace qs::ui {

ReloadPopup::ReloadPopup(QString instanceId, bool failed, QString errorString)
    : generation(new EngineGeneration())
    , instanceId(std::move(instanceId))
    , failed(failed)
    , errorString(std::move(errorString)) {
	auto component = QQmlComponent(
	    this->generation->engine,
	    "qrc:/qt/qml/Quickshell/_InternalUi/ReloadPopup.qml",
	    this
	);

	this->popup = component.createWithInitialProperties({{"reloadInfo", QVariant::fromValue(this)}});

	if (!popup) {
		qCritical() << "Failed to open reload popup:" << component.errorString();
	}

	this->generation->onReload(nullptr);
}

void ReloadPopup::closed() {
	if (ReloadPopup::activePopup == this) ReloadPopup::activePopup = nullptr;

	if (!this->deleting) {
		this->deleting = true;

		QTimer::singleShot(0, [this]() {
			if (this->popup) this->popup->deleteLater();
			this->generation->destroy();
			this->deleteLater();
		});
	}
}

void ReloadPopup::spawnPopup(QString instanceId, bool failed, QString errorString) {
	if (qEnvironmentVariableIsSet("QS_NO_RELOAD_POPUP")) return;

	if (ReloadPopup::activePopup) ReloadPopup::activePopup->closed();
	ReloadPopup::activePopup = new ReloadPopup(std::move(instanceId), failed, std::move(errorString));
}

ReloadPopup* ReloadPopup::activePopup = nullptr;

} // namespace qs::ui
