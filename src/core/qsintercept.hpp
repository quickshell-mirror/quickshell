#pragma once

#include <qdir.h>
#include <qhash.h>
#include <qloggingcategory.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qqmlabstracturlinterceptor.h>
#include <qqmlnetworkaccessmanagerfactory.h>
#include <qurl.h>

#include "logcat.hpp"

QS_DECLARE_LOGGING_CATEGORY(logQsIntercept);

class QsUrlInterceptor: public QQmlAbstractUrlInterceptor {
public:
	explicit QsUrlInterceptor(const QDir& configRoot): configRoot(configRoot) {}

	QUrl intercept(const QUrl& originalUrl, QQmlAbstractUrlInterceptor::DataType type) override;

private:
	QDir configRoot;
};

class QsInterceptDataReply: public QNetworkReply {
	Q_OBJECT;

public:
	QsInterceptDataReply(const QString& data, QObject* parent = nullptr);

	qint64 readData(char* data, qint64 maxSize) override;

private slots:
	void abort() override {}

private:
	qint64 offset = 0;
	QByteArray content;
};

class QsInterceptNetworkAccessManager: public QNetworkAccessManager {
	Q_OBJECT;

public:
	QsInterceptNetworkAccessManager(
	    const QDir& configRoot,
	    const QHash<QString, QString>& fileIntercepts,
	    QObject* parent = nullptr
	);

protected:
	QNetworkReply* createRequest(
	    QNetworkAccessManager::Operation op,
	    const QNetworkRequest& req,
	    QIODevice* outgoingData = nullptr
	) override;

private:
	QDir configRoot;
	const QHash<QString, QString>& fileIntercepts;
};

class QsInterceptNetworkAccessManagerFactory: public QQmlNetworkAccessManagerFactory {
public:
	QsInterceptNetworkAccessManagerFactory(
	    const QDir& configRoot,
	    const QHash<QString, QString>& fileIntercepts
	)
	    : configRoot(configRoot)
	    , fileIntercepts(fileIntercepts) {}
	QNetworkAccessManager* create(QObject* parent) override;

private:
	QDir configRoot;
	const QHash<QString, QString>& fileIntercepts;
};
