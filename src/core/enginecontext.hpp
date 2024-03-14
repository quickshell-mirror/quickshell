#pragma once

#include "qsintercept.hpp"
#include "scan.hpp"
#include "singleton.hpp"

class EngineContext {
public:
	explicit EngineContext(const QmlScanner& scanner);

private:
	const QmlScanner& scanner;
	QQmlEngine engine;
	QsInterceptNetworkAccessManagerFactory interceptFactory;
	SingletonRegistry singletonRegistry;
};
