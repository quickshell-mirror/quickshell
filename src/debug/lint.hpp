#pragma once

#include <qobject.h>
#include <qquickitem.h>

namespace qs::debug {

void lintObjectTree(QObject* object);
void lintItemTree(QQuickItem* item);

} // namespace qs::debug
