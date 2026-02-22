#include "nm_settings.hpp"

#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "../core/logcat.hpp"
#include "enums.hpp"

namespace qs::network {

namespace {
QS_LOGGING_CATEGORY(logNmSettings, "quickshell.network.nm_settings", QtWarningMsg);

bool wpaPskIsValid(const QString& psk) {
	if (psk.isEmpty()) return false;
	const auto psklen = psk.length();

	// ASCII passphrase
	if (psklen < 8 || psklen > 64) return false;

	// Hex PSK
	if (psklen == 64) {
		for (int i = 0; i < psklen; ++i) {
			if (!psk.at(i).isLetterOrNumber()) {
				return false;
			}
		}
	}

	return true;
}

} // namespace

NMSettings::NMSettings(QObject* parent): QObject(parent) {}

void NMSettings::clearSecrets() { emit this->requestClearSecrets(); }

void NMSettings::forget() { emit this->requestForget(); }

NMWifiSettings::NMWifiSettings(QObject* parent): NMSettings(parent) {}

void NMWifiSettings::setWifiPsk(const QString& psk) {
	if (this->bWifiSecurity != WifiSecurityType::WpaPsk
	    && this->bWifiSecurity != WifiSecurityType::Wpa2Psk
	    && this->bWifiSecurity != WifiSecurityType::Sae)
	{
		return;
	}
	if (!wpaPskIsValid(psk)) {
		qCWarning(logNmSettings) << "Malformed PSK provided to" << this;
		return;
	}
	emit this->requestClearSecrets();
	emit this->requestSetWifiPsk(psk);
}

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::NMSettings* settings) {
	auto saver = QDebugStateSaver(debug);

	if (settings) {
		debug.nospace() << "NMSettings(" << static_cast<const void*>(settings)
		                << ", id=" << settings->id() << ")";
	} else {
		debug << "NMConnection(nullptr)";
	}

	return debug;
}
