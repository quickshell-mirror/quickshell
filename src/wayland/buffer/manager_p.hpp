#pragma once

#include "dmabuf.hpp"
#include "manager.hpp"

namespace qs::wayland::buffer {

class WlBufferManagerPrivate {
public:
	explicit WlBufferManagerPrivate(WlBufferManager* manager);

	void dmabufReady();

	WlBufferManager* manager;
	dmabuf::LinuxDmabufManager dmabuf;

	bool mReady = false;
};

} // namespace qs::wayland::buffer
