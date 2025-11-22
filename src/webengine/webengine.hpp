#include <QDebug>
#include <qdebug.h>
#include <qlibrary.h>
#include <qlogging.h>

namespace qs::web_engine {

inline void printNotLoaded() {
	qWarning() << "QtWebEngineQuick is not Loaded. Using the qml type WebEngineView from "
	              "QtWebEngine might lead to undefined behaviour!";
}

inline bool init() {
	using InitializeFunc = void (*)();

	QLibrary lib("Qt6WebEngineQuick");
	if (!lib.load()) {
		qWarning() << "Failed to load library:" << lib.errorString();
		qWarning() << "You might need to install the necessary package for Qt6WebEngineQuick.";
		printNotLoaded();
		return false;
	}

	qDebug() << "Loaded library Qt6WebEngineQuick";

	auto initialize =
	    reinterpret_cast<InitializeFunc>(lib.resolve("_ZN16QtWebEngineQuick10initializeEv"));
	if (!initialize) {
		qWarning() << "Failed to resolve symbol 'void QtWebEngineQuick::initialize()' in lib "
		              "Qt6WebEngineQuick. This should not happen";

		printNotLoaded();
		return false;
	}

	qDebug() << "Found symbol QtWebEngineQuick::initialize(). Initializing WebEngine...";

	try {
		initialize();
		qDebug() << "Successfully initialized QtWebEngineQuick";
	} catch (const std::exception& e) {
		qWarning() << "Exception while calling QtWebEngineQuick::initialize()" << e.what();
		printNotLoaded();
		return false;
	} catch (...) {
		qWarning() << "Unknown Exception while calling QtWebEngineQuick::initialize()";
		printNotLoaded();
		return false;
	}

	return true;
}

} // namespace qs::web_engine