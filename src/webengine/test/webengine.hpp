#include <qobject.h>

class WebEngineInitTest: public QObject {
	Q_OBJECT

private slots:
	static void init();
};
