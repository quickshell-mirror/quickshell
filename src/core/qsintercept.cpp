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
#include <qstring.h>
#include <qtypes.h>

Q_LOGGING_CATEGORY(logQsIntercept, "quickshell.interceptor", QtWarningMsg);

QsInterceptDataReply::QsInterceptDataReply(const QString& qmldir, QObject* parent)
    : QNetworkReply(parent)
    , content(qmldir.toUtf8()) {
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
    const QHash<QString, QString>& qmldirIntercepts,
    QObject* parent
)
    : QNetworkAccessManager(parent)
    , qmldirIntercepts(qmldirIntercepts) {}

QNetworkReply* QsInterceptNetworkAccessManager::createRequest(
    QNetworkAccessManager::Operation op,
    const QNetworkRequest& req,
    QIODevice* outgoingData
) {
	auto url = req.url();
	if (url.scheme() == "qsintercept") {
		auto path = url.path();
		qCDebug(logQsIntercept) << "Got intercept for" << path << "contains"
		                        << this->qmldirIntercepts.value(path);
		auto qmldir = this->qmldirIntercepts.value(path);
		if (qmldir != nullptr) {
			return new QsInterceptDataReply(qmldir, this);
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
	return new QsInterceptNetworkAccessManager(this->qmldirIntercepts, parent);
}
