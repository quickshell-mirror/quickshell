#pragma once

#include <qcontainerfwd.h>
#include <qhash.h>
#include <qprocess.h>
#include <qvariant.h>

namespace qs::core::process {

void setupProcessEnvironment(
    QProcess* process,
    bool clear,
    const QHash<QString, QVariant>& envChanges
);

}
