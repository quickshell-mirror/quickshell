#pragma once

#include <qcontainerfwd.h>
#include <qjsvalue.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

namespace qs::http {

class HttpClient: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	explicit HttpClient(QObject* parent = nullptr);

	void connectReply(QNetworkReply* reply, const QJSValue& callback, int timeout);

	Q_INVOKABLE void request(
	    const QString& url,
	    const QString& verb,
	    const QJSValue& options,
	    const QJSValue& callback = QJSValue()
	);
	Q_INVOKABLE void
	get(const QString& url, const QJSValue& options, const QJSValue& callback = QJSValue());
	Q_INVOKABLE void post(
	    const QString& url,
	    const QVariant& body,
	    const QJSValue& options,
	    const QJSValue& callback = QJSValue()
	);
	Q_INVOKABLE void
	put(const QString& url,
	    const QVariant& body,
	    const QJSValue& options,
	    const QJSValue& callback = QJSValue());
	// cannot use delete since it's a reserved keyword
	Q_INVOKABLE void
	del(const QString& url, const QJSValue& options, const QJSValue& callback = QJSValue());
	Q_INVOKABLE void
	head(const QString& url, const QJSValue& options, const QJSValue& callback = QJSValue());

private:
	QNetworkAccessManager* mManager;
};

} // namespace qs::http
