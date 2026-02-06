#include "nm_connection.hpp"

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
QS_LOGGING_CATEGORY(logNmConnection, "quickshell.network.nm_connection", QtWarningMsg);

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

NMConnection::NMConnection(QObject* parent): QObject(parent) {}

void NMConnection::clearSecrets() { emit this->requestClearSecrets(); }

void NMConnection::forget() { emit this->requestForget(); }

void NMConnection::setGeneralSettings(const QVariantMap& settings) {
	if (this->bGeneralSettings == settings) return;
	emit this->requestSetGeneralSettings(settings);
}

NMWifiConnection::NMWifiConnection(QObject* parent): NMConnection(parent) {}

void NMWifiConnection::setWifiPsk(const QString& psk) {
	if (this->bWifiSecurity != WifiSecurityType::WpaPsk
	    && this->bWifiSecurity != WifiSecurityType::Wpa2Psk)
	{
		return;
	}
	if (!wpaPskIsValid(psk)) {
		qCWarning(logNmConnection) << "Malformed PSK provided to" << this;
		return;
	}
	emit this->requestSetWifiPsk(psk);
}

void NMWifiConnection::setWifiSettings(const QVariantMap& settings) {
	if (this->bWifiSettings == settings) return;
	emit this->requestSetWifiSettings(settings);
}

void NMWifiConnection::setWifiSecuritySettings(const QVariantMap& settings) {
	if (this->bWifiSecuritySettings == settings) return;
	emit this->requestSetWifiSecuritySettings(settings);
}

void NMWifiConnection::setWifiAuthSettings(const QVariantMap& settings) {
	if (this->bWifiAuthSettings == settings) return;
	emit this->requestSetWifiAuthSettings(settings);
}

} // namespace qs::network

QDebug operator<<(QDebug debug, const qs::network::NMConnection* connection) {
	auto saver = QDebugStateSaver(debug);

	if (connection) {
		debug.nospace() << "NMConnection(" << static_cast<const void*>(connection)
		                << ", id=" << connection->id() << ")";
	} else {
		debug << "NMConnection(nullptr)";
	}

	return debug;
}
