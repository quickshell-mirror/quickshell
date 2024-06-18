#pragma once

#include <utility>

#include <qmutex.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qthread.h>
#include <qtmetamacros.h>
#include <qwaitcondition.h>
#include <security/pam_appl.h>

/// The result of an authentication.
class PamResult: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum {
		/// Authentication was successful.
		Success = 0,
		/// Authentication failed.
		Failed = 1,
		/// An error occurred while trying to authenticate.
		Error = 2,
		/// The authentication method ran out of tries and should not be used again.
		MaxTries = 3,
		// The account has expired.
		// Expired  4,
		// Permission denied.
		// PermissionDenied  5,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(PamResult::Enum value);
};

/// An error that occurred during an authentication.
class PamError: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum {
		/// Failed to initiate the pam connection.
		ConnectionFailed = 1,
		/// Failed to try to authenticate the user.
		/// This is not the same as the user failing to authenticate.
		TryAuthFailed = 2,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(PamError::Enum value);
};

class PamConversation: public QThread {
	Q_OBJECT;

public:
	explicit PamConversation(QString config, QString configDir, QString user)
	    : config(std::move(config))
	    , configDir(std::move(configDir))
	    , user(std::move(user)) {}

public:
	void run() override;

	void abort();
	void respond(QString response);

signals:
	void completed(PamResult::Enum result);
	void error(PamError::Enum error);
	void message(QString message, bool messageChanged, bool isError, bool responseRequired);

private:
	static int conversation(
	    int msgCount,
	    const pam_message** msgArray,
	    pam_response** responseArray,
	    void* appdata
	);

	QString config;
	QString configDir;
	QString user;

	QMutex wakeMutex;
	QWaitCondition waker;
	bool mAbort = false;
	bool hasResponse = false;
	QString response;
};
