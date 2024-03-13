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
	~SingletonRegistry();
	Q_DISABLE_COPY_MOVE(SingletonRegistry);

	void install(const QUrl& url, Singleton* singleton);
	void flip();

	static SingletonRegistry* instance();

private:
	QMap<QUrl, QObject*>* previousRegistry = nullptr;
	QMap<QUrl, QObject*>* currentRegistry = nullptr;
};
