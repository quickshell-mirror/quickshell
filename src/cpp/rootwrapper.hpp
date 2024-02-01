#pragma once

#include <qobject.h>
#include <qobjectdefs.h>
#include <qqmlengine.h>
#include <qtmetamacros.h>
#include <qurl.h>

#include "scavenge.hpp"
#include "shell.hpp"

class RootWrapper: public QObject, virtual public Scavengeable {
	Q_OBJECT;

public:
	explicit RootWrapper(QUrl rootUrl);

	void reloadGraph(bool hard);
	void changeRoot(QtShell* newRoot);

	QObject* scavengeTargetFor(QObject* child) override;

private slots:
	void destroy();

private:
	QUrl rootUrl;
	QQmlEngine engine;
	QtShell* activeRoot = nullptr;
	QMetaObject::Connection destroyConnection;
};
