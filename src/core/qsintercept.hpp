#pragma once

#include <qhash.h>
#include <qloggingcategory.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qqmlabstracturlinterceptor.h>
#include <qqmlnetworkaccessmanagerfactory.h>
#include <qurl.h>

Q_DECLARE_LOGGING_CATEGORY(logQsIntercept);

class QsUrlInterceptor: public QQmlAbstractUrlInterceptor {
public:
	QUrl intercept(const QUrl& url, QQmlAbstractUrlInterceptor::DataType type) override;
};

class QsInterceptDataReply: public QNetworkReply {
	Q_OBJECT;

public:
	QsInterceptDataReply(const QString& qmldir, QObject* parent = nullptr);

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
	    const QHash<QString, QString>& qmldirIntercepts,
	    QObject* parent = nullptr
	);

protected:
	QNetworkReply* createRequest(
	    QNetworkAccessManager::Operation op,
	    const QNetworkRequest& req,
	    QIODevice* outgoingData = nullptr
	) override;

private:
	const QHash<QString, QString>& qmldirIntercepts;
};

class QsInterceptNetworkAccessManagerFactory: public QQmlNetworkAccessManagerFactory {
public:
	QsInterceptNetworkAccessManagerFactory(const QHash<QString, QString>& qmldirIntercepts)
	    : qmldirIntercepts(qmldirIntercepts) {}
	QNetworkAccessManager* create(QObject* parent) override;

private:
	const QHash<QString, QString>& qmldirIntercepts;
};
