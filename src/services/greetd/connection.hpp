#pragma once

#include <qcontainerfwd.h>
#include <qjsonobject.h>
#include <qlocalsocket.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

///! State of the Greetd connection.
/// See @@Greetd.state.
class GreetdState: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Inactive = 0,
		Authenticating = 1,
		ReadyToLaunch = 2,
		Launching = 3,
		Launched = 4,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(GreetdState::Enum value);
};

class GreetdConnection: public QObject {
	Q_OBJECT;

public:
	void createSession(QString user);
	void cancelSession();
	void respond(QString response);
	void launch(const QList<QString>& command, const QList<QString>& environment, bool exit);

	[[nodiscard]] bool isAvailable() const;
	[[nodiscard]] GreetdState::Enum state() const;

	[[nodiscard]] QString user() const;

	static GreetdConnection* instance();

signals:
	void authMessage(QString message, bool error, bool responseRequired, bool echoResponse);
	void authFailure(QString message);
	void readyToLaunch();
	void launched();
	void error(QString error);

	void stateChanged();
	void userChanged();

private slots:
	void onSocketConnected();
	void onSocketError(QLocalSocket::LocalSocketError error);
	void onSocketReady();

private:
	explicit GreetdConnection();

	void sendRequest(const QJsonObject& json);
	void setActive(bool active);
	void setInactive();

	bool mAvailable = false;
	GreetdState::Enum mState = GreetdState::Inactive;
	bool mTargetActive = false;
	bool mExitAfterLaunch = false;
	QString mMessage;
	bool mResponseRequired = false;
	QString mUser;
	QLocalSocket socket;
};
