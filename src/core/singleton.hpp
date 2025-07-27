#pragma once

#include <qhash.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qurl.h>

#include "reload.hpp"

///! The root component for reloadable singletons.
/// All singletons should inherit from this type.
class Singleton: public ReloadPropagator {
	Q_OBJECT;
	QML_ELEMENT;

public:
	void componentComplete() override;
};

class SingletonRegistry {
public:
	SingletonRegistry() = default;

	void registerSingleton(const QUrl& url, Singleton* singleton);
	void onReload(SingletonRegistry* old);

private:
	QHash<QUrl, Singleton*> registry;
};
