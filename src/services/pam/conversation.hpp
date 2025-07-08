#pragma once

#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qsocketnotifier.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/logcat.hpp"
#include "ipc.hpp"

QS_DECLARE_LOGGING_CATEGORY(logPam);

///! The result of an authentication.
/// See @@PamContext.completed(s).
class PamResult: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// Authentication was successful.
		Success = 0,
		/// Authentication failed.
		Failed = 1,
		/// An error occurred while trying to authenticate.
		Error = 2,
		/// The authentication method ran out of tries and should not be used again.
		MaxTries = 3,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(PamResult::Enum value);
};

///! An error that occurred during an authentication.
/// See @@PamContext.error(s).
class PamError: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// Failed to start the pam session.
		StartFailed = 1,
		/// Failed to try to authenticate the user.
		/// This is not the same as the user failing to authenticate.
		TryAuthFailed = 2,
		/// An error occurred inside quickshell's pam interface.
		InternalError = 3,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(PamError::Enum value);
};

// PAM has no way to abort a running module except when it sends a message,
// meaning aborts for things like fingerprint scanners
// and hardware keys don't actually work without aborting the process...
// so we have a subprocess.
class PamConversation: public QObject {
	Q_OBJECT;

public:
	explicit PamConversation(QObject* parent): QObject(parent) {}
	~PamConversation() override;
	Q_DISABLE_COPY_MOVE(PamConversation);

public:
	void start(const QString& configDir, const QString& config, const QString& user);

	void abort();
	void respond(const QString& response);

signals:
	void completed(PamResult::Enum result);
	void error(PamError::Enum error);
	void message(QString message, bool messageChanged, bool isError, bool responseRequired);

private slots:
	void onMessage();

private:
	static pid_t createSubprocess(
	    PamIpcPipes* pipes,
	    const QString& configDir,
	    const QString& config,
	    const QString& user
	);

	void internalError();

	pid_t childPid = 0;
	PamIpcPipes pipes;
	QSocketNotifier notifier {QSocketNotifier::Read};
};
