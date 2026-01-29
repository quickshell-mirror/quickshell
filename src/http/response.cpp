#include "response.hpp"

#include <qcontainerfwd.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonvalue.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qobject.h>
#include <qqmlinfo.h>

namespace qs::http {

HttpResponse::HttpResponse(QNetworkReply* reply, QObject* parent): QObject(parent) {
	this->mStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	this->mStatusText = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString();
	this->mUrl = reply->url();
	this->mData = reply->readAll();

	auto replyErr = reply->error();
	this->mTimeouted =
	    replyErr == QNetworkReply::TimeoutError || replyErr == QNetworkReply::OperationCanceledError;

	auto headersList = reply->rawHeaderList();
	for (auto& header: headersList) {
		this->mHeadersMap.insert(
		    QString::fromUtf8(header),
		    QString::fromUtf8(reply->rawHeader(header))
		);
	}
}

QUrl HttpResponse::url() { return this->mUrl; }

QString HttpResponse::text() { return QString::fromUtf8(this->mData); }

QJsonValue HttpResponse::json() {
	QJsonParseError error;
	auto json = QJsonDocument::fromJson(this->mData, &error);

	if (error.error != QJsonParseError::NoError) {
		qmlWarning(this) << "Failed to deserialize json: " << error.errorString();
		return QJsonValue::Undefined;
	}

	if (json.isArray()) {
		return json.array();
	}

	return json.object();
}

QByteArray HttpResponse::arrayBuffer() { return this->mData; }

QVariantMap HttpResponse::headers() { return this->mHeadersMap; }

QString HttpResponse::header(const QString& name) {
	return this->mHeadersMap.value(name).toString();
}

} // namespace qs::http
