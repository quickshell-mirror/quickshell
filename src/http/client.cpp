#include "client.hpp"

#include <qjsengine.h>
#include <qjsvalue.h>
#include <qjsvalueiterator.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qobject.h>
#include <qstringview.h>
#include <qtimer.h>
#include <qurl.h>

#include "response.hpp"

namespace {

QNetworkRequest createRequest(const QString& url, const QJSValue& headers) {
	QNetworkRequest req;

	req.setUrl(QUrl(url));

	if (headers.isObject()) {
		QJSValueIterator iter(headers);
		while (iter.hasNext()) {
			iter.next();
			req.setRawHeader(iter.name().toUtf8(), iter.value().toString().toUtf8());
		}
	}

	return req;
}

} // namespace

namespace qs::http {

HttpClient::HttpClient(QObject* parent)
    : QObject(parent)
    , mManager(new QNetworkAccessManager(this)) {}

void HttpClient::connectReply(QNetworkReply* reply, const QJSValue& callback, int timeout) {
	if (timeout > 0) {
		auto* timer = new QTimer(reply);
		timer->setSingleShot(true);
		timer->setInterval(timeout);
		connect(timer, &QTimer::timeout, reply, &QNetworkReply::abort);
		timer->start();
	}

	QJSEngine* engine = qjsEngine(this);

	if (!engine) {
		return;
	}
	if (callback.isCallable()) {
		connect(reply, &QNetworkReply::finished, this, [reply, engine, callback]() mutable {
			reply->deleteLater();
			auto* resp = new HttpResponse(reply);
			auto jsResp = engine->newQObject(resp);
			QJSEngine::setObjectOwnership(resp, QJSEngine::JavaScriptOwnership);

			callback.call(QJSValueList() << jsResp);
		});
	}
}

void HttpClient::request(
    const QString& url,
    const QString& verb,
    const QJSValue& options,
    const QJSValue& callback
) {
	QJSValue headers;
	QByteArray body;
	int timeout = -1;

	if (options.isObject()) {
		if (options.hasOwnProperty("headers")) {
			headers = options.property("headers");
		}
		if (options.hasOwnProperty("body")) {
			auto bodyProp = options.property("body");
			body = bodyProp.toVariant().toByteArray();
		}
		if (options.hasOwnProperty("timeout")) {
			auto timeoutProp = options.property("timeout");
			if (timeoutProp.isNumber()) {
				timeout = static_cast<int>(timeoutProp.toNumber());
			}
		}
	}

	auto req = createRequest(url, headers);
	auto* reply = this->mManager->sendCustomRequest(req, verb.toUtf8(), body);

	this->connectReply(reply, callback, timeout);
}

void HttpClient::get(const QString& url, const QJSValue& options, const QJSValue& callback) {
	QJSValue headers;
	int timeout = -1;

	if (options.isObject()) {
		if (options.hasOwnProperty("headers")) {
			headers = options.property("headers");
		}
		if (options.hasOwnProperty("timeout")) {
			auto timeoutProp = options.property("timeout");
			if (timeoutProp.isNumber()) {
				timeout = static_cast<int>(timeoutProp.toNumber());
			}
		}
	}

	auto req = createRequest(url, headers);
	auto* reply = this->mManager->get(req);

	this->connectReply(reply, callback, timeout);
}

void HttpClient::post(
    const QString& url,
    const QVariant& body,
    const QJSValue& options,
    const QJSValue& callback
) {
	auto bodyBytes = body.toByteArray();
	QJSValue headers;
	int timeout = -1;

	if (options.isObject()) {
		if (options.hasOwnProperty("headers")) {
			headers = options.property("headers");
		}
		if (options.hasOwnProperty("timeout")) {
			auto timeoutProp = options.property("timeout");
			if (timeoutProp.isNumber()) {
				timeout = static_cast<int>(timeoutProp.toNumber());
			}
		}
	}

	auto req = createRequest(url, headers);
	auto* reply = this->mManager->post(req, bodyBytes);

	this->connectReply(reply, callback, timeout);
}

void HttpClient::put(
    const QString& url,
    const QVariant& body,
    const QJSValue& options,
    const QJSValue& callback
) {
	auto bodyBytes = body.toByteArray();
	QJSValue headers;
	int timeout = -1;

	if (options.isObject()) {
		if (options.hasOwnProperty("headers")) {
			headers = options.property("headers");
		}
		if (options.hasOwnProperty("timeout")) {
			auto timeoutProp = options.property("timeout");
			if (timeoutProp.isNumber()) {
				timeout = static_cast<int>(timeoutProp.toNumber());
			}
		}
	}

	auto req = createRequest(url, headers);
	auto* reply = this->mManager->put(req, bodyBytes);

	this->connectReply(reply, callback, timeout);
}

void HttpClient::del(const QString& url, const QJSValue& options, const QJSValue& callback) {
	QJSValue headers;
	int timeout = -1;

	if (options.isObject()) {
		if (options.hasOwnProperty("headers")) {
			headers = options.property("headers");
		}
		if (options.hasOwnProperty("timeout")) {
			auto timeoutProp = options.property("timeout");
			if (timeoutProp.isNumber()) {
				timeout = static_cast<int>(timeoutProp.toNumber());
			}
		}
	}

	auto req = createRequest(url, headers);
	auto* reply = this->mManager->deleteResource(req);

	this->connectReply(reply, callback, timeout);
}

void HttpClient::head(const QString& url, const QJSValue& options, const QJSValue& callback) {
	QJSValue headers;
	int timeout = -1;

	if (options.isObject()) {
		if (options.hasOwnProperty("headers")) {
			headers = options.property("headers");
		}
		if (options.hasOwnProperty("timeout")) {
			auto timeoutProp = options.property("timeout");
			if (timeoutProp.isNumber()) {
				timeout = static_cast<int>(timeoutProp.toNumber());
			}
		}
	}

	auto req = createRequest(url, headers);
	auto* reply = this->mManager->head(req);

	this->connectReply(reply, callback, timeout);
}

} // namespace qs::http
