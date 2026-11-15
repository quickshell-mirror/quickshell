#pragma once

#include <qlist.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "../../core/model.hpp"
#include "dbus_service.h"
#include "device.hpp"

namespace qs::services::fprint {

class FprintManager: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] ObjectModel<FprintDevice>* devices();
	[[nodiscard]] FprintDevice* defaultDevice();

	static FprintManager* instance();

private:
	explicit FprintManager();

	void init();
	void registerDevice(const QString& path);
	void registerDevices();
	void registerDefaultDevice();

	FprintDevice mDefaultDevice {this};
	ObjectModel<FprintDevice> mDevices {this};

	QS_DBUS_BINDABLE_PROPERTY_GROUP(FprintManager, serviceProperties);

	DBusFprintService* service = nullptr;
};

class FprintManagerQml: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(FprintManager);
	QML_SINGLETON;

	Q_PROPERTY(qs::services::fprint::FprintDevice* defaultDevice READ defaultDevice CONSTANT);
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::service::fprint::FprintDevice>*);
	Q_PROPERTY(UntypedObjectModel* devices READ devices CONSTANT);

public:
	explicit FprintManagerQml(QObject* parent = nullptr);

	[[nodiscard]] ObjectModel<FprintDevice>* devices();
	[[nodiscard]] FprintDevice* defaultDevice();
};

} // namespace qs::services::fprint
