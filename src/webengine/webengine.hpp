#include <QDebug>
#include <qdebug.h>
#include <qlibrary.h>
#include <qlogging.h>

namespace qs::web_engine {

inline void init() {
	using InitializeFunc = void (*)();

	QLibrary lib("Qt6WebEngineQuick");
	if (!lib.load()) {
		qWarning() << "Failed to load library:" << lib.errorString();
		qWarning() << "You might need to install the necessary package for Qt6WebEngineQuick.";
		qWarning() << "QtWebEngineQuick is not Loaded. Using the qml type WebEngineView from "
		              "QtWebEngine might lead to undefined behaviour!";
		return;
	}

	qDebug() << "Loaded library Qt6WebEngineQuick";

	auto initialize =
	    reinterpret_cast<InitializeFunc>(lib.resolve("_ZN16QtWebEngineQuick10initializeEv"));
	if (!initialize) {
		qWarning() << "Failed to resolve symbol 'void QtWebEngineQuick::initialize()' in lib "
		              "Qt6WebEngineQuick. This should not happen";
		qWarning() << "QtWebEngineQuick is not Loaded. Using the qml type WebEngineView from "
		              "QtWebEngine might lead to undefined behaviour!";
		return;
	}

	qDebug() << "Found symbol QtWebEngineQuick::initialize(). Initializing WebEngine...";

	initialize();

	qDebug() << "Successfully initialized QtWebEngineQuick";
}

} // namespace qs::web_engine