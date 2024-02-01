#pragma once

#include <qobject.h>
#include <qobjectdefs.h>
#include <qqmlengine.h>
#include <qtmetamacros.h>
#include <qurl.h>

#include "shell.hpp"

class RootWrapper: public QObject {
	Q_OBJECT;

public:
	explicit RootWrapper(QUrl rootUrl);

	void reloadGraph();
	void changeRoot(QtShell* newRoot);

private slots:
	void destroy();

private:
	QUrl rootUrl;
	QQmlEngine engine;
	QtShell* activeRoot = nullptr;
	QMetaObject::Connection destroyConnection;
};
