#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "connection.hpp"

/// This object provides access to a running greetd instance if present.
/// With it you can authenticate a user and launch a session.
///
/// See [the greetd wiki] for instructions on how to set up a graphical greeter.
///
/// [the greetd wiki]: https://man.sr.ht/~kennylevinsen/greetd/#setting-up-greetd-with-gtkgreet
class Greetd: public QObject {
	Q_OBJECT;
	/// If the greetd socket is available.
	Q_PROPERTY(bool available READ isAvailable CONSTANT);
	/// The current state of the greetd connection.
	Q_PROPERTY(GreetdState::Enum state READ state NOTIFY stateChanged);
	/// The currently authenticating user.
	Q_PROPERTY(QString user READ user NOTIFY userChanged);
	QML_ELEMENT;
	QML_SINGLETON;

public:
	explicit Greetd(QObject* parent = nullptr);

	/// Create a greetd session for the given user.
	Q_INVOKABLE static void createSession(QString user);
	/// Cancel the active greetd session.
	Q_INVOKABLE static void cancelSession();
	/// Respond to an authentication message.
	///
	/// May only be called in response to an @@authMessage(s) with `responseRequired` set to true.
	Q_INVOKABLE static void respond(QString response);

	// docgen currently can't handle default params

	// clang-format off
	/// Launch the session, exiting quickshell.
	/// @@state must be `GreetdState.ReadyToLaunch` to call this function.
	Q_INVOKABLE static void launch(const QList<QString>& command);
	/// Launch the session, exiting quickshell.
	/// @@state must be `GreetdState.ReadyToLaunch` to call this function.
	Q_INVOKABLE static void launch(const QList<QString>& command, const QList<QString>& environment);
	/// Launch the session, exiting quickshell if @@quit is true.
	/// @@state must be `GreetdState.ReadyToLaunch` to call this function.
	///
	/// The @@launched signal can be used to perform an action after greetd has acknowledged
	/// the desired session.
	///
	/// > [!WARNING] Note that greetd expects the greeter to terminate as soon as possible
	/// > after setting a target session, and waiting too long may lead to unexpected behavior
	/// > such as the greeter restarting.
	/// >
	/// > Performing animations and such should be done *before* calling @@launch.
	Q_INVOKABLE static void launch(const QList<QString>& command, const QList<QString>& environment, bool quit);
	// clang-format on

	[[nodiscard]] static bool isAvailable();
	[[nodiscard]] static GreetdState::Enum state();
	[[nodiscard]] static QString user();

signals:
	/// An authentication message has been sent by greetd.
	/// - `message` - the text of the message
	/// - `error` - if the message should be displayed as an error
	/// - `responseRequired` - if a response via `respond()` is required for this message
	/// - `echoResponse` - if the response should be displayed in clear text to the user
	///
	/// Note that `error` and `responseRequired` are mutually exclusive.
	///
	/// Errors are sent through `authMessage` when they are recoverable, such as a fingerprint scanner
	/// not being able to read a finger correctly, while definite failures such as a bad password are
	/// sent through `authFailure`.
	void authMessage(QString message, bool error, bool responseRequired, bool echoResponse);
	/// Authentication has failed an the session has terminated.
	///
	/// Usually this is something like a timeout or a failed password entry.
	void authFailure(QString message);
	/// Authentication has finished successfully and greetd can now launch a session.
	void readyToLaunch();
	/// Greetd has acknowledged the launch request and the greeter should quit as soon as possible.
	///
	/// This signal is sent right before quickshell exits automatically if the launch was not specifically
	/// requested not to exit. You usually don't need to use this signal.
	void launched();
	/// Greetd has encountered an error.
	void error(QString error);

	void stateChanged();
	void userChanged();
};
