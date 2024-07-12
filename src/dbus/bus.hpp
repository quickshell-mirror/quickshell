#pragma once

#include <functional>

#include <qcontainerfwd.h>
#include <qdbusconnection.h>
#include <qobject.h>

namespace qs::dbus {

void tryLaunchService(
    QObject* parent,
    QDBusConnection& connection,
    const QString& serviceName,
    const std::function<void(bool)>& callback
);

}
