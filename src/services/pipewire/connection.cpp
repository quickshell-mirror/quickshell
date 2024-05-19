#include "connection.hpp"

#include <qobject.h>

namespace qs::service::pipewire {

PwConnection::PwConnection(QObject* parent): QObject(parent) {
	if (this->core.isValid()) {
		this->registry.init(this->core);
	}
}

PwConnection* PwConnection::instance() {
	static PwConnection* instance = nullptr; // NOLINT

	if (instance == nullptr) {
		instance = new PwConnection();
	}

	return instance;
}

} // namespace qs::service::pipewire
