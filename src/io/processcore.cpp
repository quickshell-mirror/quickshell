#include "processcore.hpp"

#include <qcontainerfwd.h>
#include <qhash.h>
#include <qprocess.h>
#include <qvariant.h>

#include "../core/common.hpp"

namespace qs::io::process {

void setupProcessEnvironment(
    QProcess* process,
    bool clear,
    const QHash<QString, QVariant>& envChanges
) {
	const auto& sysenv = qs::Common::INITIAL_ENVIRONMENT;
	auto env = clear ? QProcessEnvironment() : sysenv;

	for (auto& name: envChanges.keys()) {
		auto value = envChanges.value(name);
		if (!value.isValid()) continue;

		if (clear) {
			if (value.isNull()) {
				if (sysenv.contains(name)) env.insert(name, sysenv.value(name));
			} else env.insert(name, value.toString());
		} else {
			if (value.isNull()) env.remove(name);
			else env.insert(name, value.toString());
		}
	}

	process->setProcessEnvironment(env);
}

} // namespace qs::io::process
