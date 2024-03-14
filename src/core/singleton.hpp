#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
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
	QMap<QUrl, Singleton*> registry;
};
