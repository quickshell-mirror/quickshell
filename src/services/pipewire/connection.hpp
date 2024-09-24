#pragma once

#include "core.hpp"
#include "defaults.hpp"
#include "registry.hpp"

namespace qs::service::pipewire {

class PwConnection: public QObject {
	Q_OBJECT;

public:
	explicit PwConnection(QObject* parent = nullptr);

	PwRegistry registry;
	PwDefaultTracker defaults {&this->registry};

	static PwConnection* instance();

private:
	// init/destroy order is important. do not rearrange.
	PwCore core;
};

} // namespace qs::service::pipewire
