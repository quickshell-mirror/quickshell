#pragma once

#include <functional>

#include <qobject.h>
#include <qstring.h>

namespace qs::service::sni {

// Request a token using Qt's most recent input event, including layer-shell clicks.
// Returns false when input context is unavailable or a request is already pending.
bool requestActivationToken(QObject* context, std::function<void(const QString&)> callback);

} // namespace qs::service::sni
