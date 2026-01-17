#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qtmetamacros.h>

namespace qs::scan::env {

class PreprocEnv: public QObject {
	Q_OBJECT;

public:
	Q_INVOKABLE static bool
	hasVersion(int major, int minor, const QStringList& features = QStringList());
};

} // namespace qs::scan::env
