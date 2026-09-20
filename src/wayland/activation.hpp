#pragma once

#include <functional>

#include <qstring.h>
#include <qtypes.h>

namespace QtWaylandClient { // NOLINT(readability-identifier-naming)
class QWaylandWindow;
}

namespace qs::wayland::activation {

// Request a token using Qt's most recent input event. Returns false if unable to request a token.
bool requestActivationToken(std::function<void(const QString&)> callback);

	// Request an activation token for a window to be delivered via QWaylandWindow::xdgActivationTokenCreated
void requestXdgActivationToken(QtWaylandClient::QWaylandWindow* window, quint32 serial);

} // namespace qs::wayland::activation
