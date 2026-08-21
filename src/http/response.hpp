#pragma once

#include <qcontainerfwd.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>
#include <qnetworkreply.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qstringview.h>
#include <qtmetamacros.h>
#include <qurl.h>

namespace qs::http {

class HttpResponse: public QObject {
	Q_OBJECT;
	QML_UNCREATABLE("HttpResponse can only be created by HttpClient");
	Q_PROPERTY(int status READ status CONSTANT);
	Q_PROPERTY(QString statusText READ statusText CONSTANT);
	Q_PROPERTY(bool timeouted READ timeouted CONSTANT);

public:
	explicit HttpResponse(QNetworkReply* reply, QObject* parent = nullptr);

	Q_INVOKABLE [[nodiscard]] bool success() const {
		return this->mStatus >= 200 && this->mStatus < 300;
	}
	Q_INVOKABLE [[nodiscard]] QUrl url();
	Q_INVOKABLE [[nodiscard]] QString text();
	Q_INVOKABLE [[nodiscard]] QJsonValue json();
	Q_INVOKABLE [[nodiscard]] QByteArray arrayBuffer();
	Q_INVOKABLE [[nodiscard]] QVariantMap headers();
	Q_INVOKABLE [[nodiscard]] QString header(const QString& name);

	[[nodiscard]] int status() const { return this->mStatus; }

	[[nodiscard]] QString statusText() const { return this->mStatusText; }

	[[nodiscard]] bool timeouted() const { return this->mTimeouted; }

private:
	int mStatus;
	QString mStatusText;
	bool mTimeouted;
	QUrl mUrl;
	QByteArray mData;
	QVariantMap mHeadersMap;
};

} // namespace qs::http
