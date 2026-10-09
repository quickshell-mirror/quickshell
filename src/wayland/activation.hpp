#pragma once

#include <functional>

#include <qobject.h>
#include <qstring.h>
#include <qtypes.h>

namespace QtWaylandClient { // NOLINT(readability-identifier-naming)
class QWaylandWindow;
}

namespace qs::wayland::activation {

// Request a token using Qt's most recent input event, including layer-shell clicks.
// Returns false when input context is unavailable or a request is already pending.
bool requestActivationToken(QObject* context, std::function<void(const QString&)> callback);

// Shell integrations use this to deliver a protocol token through Qt's window signal.
// Qt's xdg-shell client headers are not installed (see xdgshell.hpp).
void requestXdgActivationToken(QtWaylandClient::QWaylandWindow* window, quint32 serial);

} // namespace qs::wayland::activation
