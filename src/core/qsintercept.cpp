#include "qsintercept.hpp"
#include <cstring>

#include <qdir.h>
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
		url.setScheme("qs");

		auto path = url.path();
		if (path.startsWith('/')) path = path.sliced(1);
		url.setPath("@/qs/" % path);

		qCDebug(logQsIntercept) << "Rewrote root intercept" << originalUrl << "to" << url;
	}

	if (url.scheme() == "qs") {
		auto path = url.path();

		// Our import path is on "qs:@/".
		// We want to blackhole any import resolution outside of the config folder as it breaks Qt
		// but NOT file lookups that might be on "qs:/" due to a missing "file:/" prefix.
		if (path.startsWith("@/qs/")) {
			path = this->configRoot.filePath(path.sliced(5));
		} else if (!path.startsWith("/")) {
			qCDebug(logQsIntercept) << "Blackholed import URL" << url;
			return QUrl("qrc:/qs-blackhole");
		}

		// Documents must resolve to their vfs path. Qt keys types by intercepted url, so a
		// second url for the same document means a second compile and singleton instance.
		if (!this->vfsPath.isEmpty()
		    && (type != QQmlAbstractUrlInterceptor::DataType::UrlString || path.endsWith(".qml")))
		{
			auto newUrl = url;
			newUrl.setScheme("file");
			newUrl.setPath(this->toVfsPath(path));
			qCDebug(logQsIntercept) << "Rewrote intercept" << url << "to" << newUrl;
			return newUrl;
		}

		// Some types such as Image take into account where they are loading from, and force
		// asynchronous loading over a network. qs: is considered to be over a network.
		// In those cases we want to return a file:// url so asynchronous loading is not forced.
		if (type == QQmlAbstractUrlInterceptor::DataType::UrlString) {
			// Qt.resolvedUrl and context->resolvedUrl can use this on qml files, in which
			// case we want to keep the intercept, otherwise objects created from those paths
			// will not be able to use singletons.
			if (path.endsWith(".qml")) return url;

			auto newUrl = url;
			newUrl.setScheme("file");
			// above check asserts path starts with /qs/
			newUrl.setPath(path);
			qCDebug(logQsIntercept) << "Rewrote intercept" << url << "to" << newUrl;
			return newUrl;
		}
	}

	// Real config paths, e.g. from Quickshell.shellDir, are folded in for the same reason.
	if (!this->vfsPath.isEmpty() && url.scheme() == "file") {
		auto isDocument =
		    type == QQmlAbstractUrlInterceptor::DataType::QmlFile
		    || type == QQmlAbstractUrlInterceptor::DataType::JavaScriptFile
		    || type == QQmlAbstractUrlInterceptor::DataType::QmldirFile
		    || (type == QQmlAbstractUrlInterceptor::DataType::UrlString && url.path().endsWith(".qml"));

		if (isDocument) {
			auto newPath = this->toVfsPath(url.path());

			if (newPath != url.path()) {
				auto newUrl = url;
				newUrl.setPath(newPath);
				qCDebug(logQsIntercept) << "Canonicalized intercept" << url << "to" << newUrl;
				return newUrl;
			}
		}
	}

	return url;
}

QString QsUrlInterceptor::toVfsPath(const QString& path) const {
	const auto& rootPath = this->configRoot.path();
	if (!path.startsWith(rootPath % '/')) return path;
	return this->vfsPath % "/qs" % path.sliced(rootPath.length());
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
    const QDir& configRoot,
    const QHash<QString, QString>& fileIntercepts,
    QObject* parent
)
    : QNetworkAccessManager(parent)
    , configRoot(configRoot)
    , fileIntercepts(fileIntercepts) {}

QNetworkReply* QsInterceptNetworkAccessManager::createRequest(
    QNetworkAccessManager::Operation op,
    const QNetworkRequest& req,
    QIODevice* outgoingData
) {
	auto url = req.url();

	if (url.scheme() == "qs") {
		auto path = url.path();

		if (path.startsWith("@/qs/")) path = this->configRoot.filePath(path.sliced(5));
		// otherwise pass through to fs

		qCDebug(logQsIntercept) << "Got intercept for" << path << "contains"
		                        << this->fileIntercepts.value(path);

		if (auto data = this->fileIntercepts.value(path); !data.isEmpty()) {
			return new QsInterceptDataReply(data, this);
		}

		auto fileReq = req;
		auto fileUrl = req.url();
		fileUrl.setScheme("file");
		fileUrl.setPath(path);
		qCDebug(logQsIntercept) << "Passing through intercept" << url << "to" << fileUrl;

		fileReq.setUrl(fileUrl);
		return this->QNetworkAccessManager::createRequest(op, fileReq, outgoingData);
	}

	return this->QNetworkAccessManager::createRequest(op, req, outgoingData);
}

QNetworkAccessManager* QsInterceptNetworkAccessManagerFactory::create(QObject* parent) {
	return new QsInterceptNetworkAccessManager(this->configRoot, this->fileIntercepts, parent);
}
