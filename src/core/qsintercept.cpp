#include "qsintercept.hpp"
#include <cstring>

#include <qhash.h>
#include <qiodevice.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qminmax.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkrequest.h>
#include <qobject.h>
#include <qqmlabstracturlinterceptor.h>
#include <qstring.h>
#include <qtypes.h>
#include <qurl.h>

#include "logcat.hpp"

QS_LOGGING_CATEGORY(logQsIntercept, "quickshell.interceptor", QtWarningMsg);

QUrl QsUrlInterceptor::intercept(
    const QUrl& originalUrl,
    QQmlAbstractUrlInterceptor::DataType type
) {
	auto url = originalUrl;

	if (url.scheme() == "root") {
		url.setScheme("qsintercept");

		auto path = url.path();
		if (path.startsWith('/')) path = path.sliced(1);
		url.setPath(this->configRoot.filePath(path));

		qCDebug(logQsIntercept) << "Rewrote root intercept" << originalUrl << "to" << url;
	}

	// Some types such as Image take into account where they are loading from, and force
	// asynchronous loading over a network. qsintercept is considered to be over a network.
	if (type == QQmlAbstractUrlInterceptor::DataType::UrlString && url.scheme() == "qsintercept") {
		// Qt.resolvedUrl and context->resolvedUrl can use this on qml files, in which
		// case we want to keep the intercept, otherwise objects created from those paths
		// will not be able to use singletons.
		if (url.path().endsWith(".qml")) return url;

		auto newUrl = url;
		newUrl.setScheme("file");
		qCDebug(logQsIntercept) << "Rewrote intercept" << url << "to" << newUrl;
		return newUrl;
	}

	return url;
}

QsInterceptDataReply::QsInterceptDataReply(const QString& data, QObject* parent)
    : QNetworkReply(parent)
    , content(data.toUtf8()) {
	this->setOpenMode(QIODevice::ReadOnly);
	this->setFinished(true);
}

qint64 QsInterceptDataReply::readData(char* data, qint64 maxSize) {
	auto size = qMin(maxSize, this->content.length() - this->offset);
	if (size == 0) return -1;
	memcpy(data, this->content.constData() + this->offset, size); // NOLINT
	this->offset += size;
	return size;
}

QsInterceptNetworkAccessManager::QsInterceptNetworkAccessManager(
    const QHash<QString, QString>& fileIntercepts,
    QObject* parent
)
    : QNetworkAccessManager(parent)
    , fileIntercepts(fileIntercepts) {}

QNetworkReply* QsInterceptNetworkAccessManager::createRequest(
    QNetworkAccessManager::Operation op,
    const QNetworkRequest& req,
    QIODevice* outgoingData
) {
	auto url = req.url();
	if (url.scheme() == "qsintercept") {
		auto path = url.path();
		qCDebug(logQsIntercept) << "Got intercept for" << path << "contains"
		                        << this->fileIntercepts.value(path);
		auto data = this->fileIntercepts.value(path);
		if (data != nullptr) {
			return new QsInterceptDataReply(data, this);
		}

		auto fileReq = req;
		auto fileUrl = req.url();
		fileUrl.setScheme("file");
		qCDebug(logQsIntercept) << "Passing through intercept" << url << "to" << fileUrl;
		fileReq.setUrl(fileUrl);
		return this->QNetworkAccessManager::createRequest(op, fileReq, outgoingData);
	}

	return this->QNetworkAccessManager::createRequest(op, req, outgoingData);
}

QNetworkAccessManager* QsInterceptNetworkAccessManagerFactory::create(QObject* parent) {
	return new QsInterceptNetworkAccessManager(this->fileIntercepts, parent);
}
