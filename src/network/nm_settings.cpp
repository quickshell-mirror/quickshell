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

void NMSettings::setId(const QString& id) {
	if (this->bId == id) return;
	emit this->requestSetId();
}

void NMSettings::setAutoconnect(bool autoconnect) {
	if (this->bAutoconnect == autoconnect) return;
	emit this->requestSetAutoconnect();
}

void NMSettings::setAutoconnectPriority(qint32 autoconnectPriority) {
	if (this->bAutoconnectPriority == autoconnectPriority) return;
	if (autoconnectPriority < -999 || autoconnectPriority > 999) return;
	emit this->requestSetAutoconnectPriority();
}

void NMSettings::setDnsOverTls(qint32 dnsOverTls) {
	if (this->bDnsOverTls == dnsOverTls) return;
	if (dnsOverTls < -1 || dnsOverTls > 2) return;
	emit this->requestSetDnsOverTls();
}

void NMSettings::setDnssec(qint32 dnssec) {
	if (this->bDnssec == dnssec) return;
	if (dnssec < -1 || dnssec > 2) return;
	emit this->requestSetDnssec();
}

NMWifiSettings::NMWifiSettings(QObject* parent): NMSettings(parent) {}

void NMWifiSettings::setPsk(const QString& psk) {
	if (this->bWifiSecurity != WifiSecurityType::WpaPsk
	    && this->bWifiSecurity != WifiSecurityType::Wpa2Psk
	    && this->bWifiSecurity != WifiSecurityType::Sae)
	{
		return;
	}
	if (!wpaPskIsValid(psk)) return;
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
