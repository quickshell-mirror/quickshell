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

#include "../api.hpp"
#include "dbus_nm_wireless.h"

namespace qs::network {

class NMWireless: public Wireless {
	Q_OBJECT;

public:
	explicit NMWireless(QObject* parent = nullptr);
	void disconnect() override;
	void scan() override;
	void setPowered(bool powered) override;
	void init(const QString& path);
	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QString path() const;
	[[nodiscard]] QString address() const;

private:
	DBusNMWireless* wireless = nullptr;
};

} // namespace qs::network
