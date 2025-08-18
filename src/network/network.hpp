#pragma once

#include <qcontainerfwd.h>
#include <qdbusextratypes.h>
#include <qdbusservicewatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "wifi.hpp"

namespace qs::network {

///! The backend supplying the Network service.
class NetworkBackendType: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// There is no available Network backend.
		None = 0,
		/// The backend is NetworkManager.
		NetworkManager = 1,
	};
	Q_ENUM(Enum);
};

class NetworkBackend: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] virtual bool isAvailable() const = 0;

protected:
	explicit NetworkBackend(QObject* parent = nullptr): QObject(parent) {};
};

///! The Network service.
/// An interface to a network backend (currently only NetworkManager),
/// which can be used to view, configure, and connect to various networks.
class Network: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(Network);
	QML_SINGLETON;

	/// The wifi device service.
	Q_PROPERTY(Wifi* wifi READ wifi CONSTANT);
	/// The backend being used to power the Network service.
	Q_PROPERTY(NetworkBackendType::Enum backend READ backend);

public:
	explicit Network(QObject* parent = nullptr);

	[[nodiscard]] Wifi* wifi() { return this->mWifi; };
	[[nodiscard]] NetworkBackendType::Enum backend() { return this->mBackendType; };

private:
	Wifi* mWifi;
	NetworkBackend* mBackend = nullptr;
	NetworkBackendType::Enum mBackendType = NetworkBackendType::None;
};

} // namespace qs::network
