#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmlparserstatus.h>
#include <qtclasshelpermacros.h>
#include <qthread.h>
#include <qtmetamacros.h>
#include <security/_pam_types.h>
#include <security/pam_appl.h>

#include "conversation.hpp"

///! Connection to pam.
/// Connection to pam. See [the module documentation](../) for pam configuration advice.
class PamContext
    : public QObject
    , public QQmlParserStatus {
	Q_OBJECT;
	QML_ELEMENT;
	Q_INTERFACES(QQmlParserStatus);

	// clang-format off
	/// If the pam context is actively performing an authentication.
	///
	/// Setting this value behaves exactly the same as calling @@start() and @@abort().
	Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged);
	/// The pam configuration to use. Defaults to "login".
	///
	/// The configuration should name a file inside @@configDirectory.
	///
	/// This property may not be set while @@active is true.
	Q_PROPERTY(QString config READ config WRITE setConfig NOTIFY configChanged);
	/// The pam configuration directory to use. Defaults to "/etc/pam.d".
	///
	/// The configuration directory is resolved relative to the current file if not an absolute path.
	///
	/// This property may not be set while @@active is true.
	Q_PROPERTY(QString configDirectory READ configDirectory WRITE setConfigDirectory NOTIFY configDirectoryChanged);
	/// The user to authenticate as. If unset the current user will be used.
	///
	/// This property may not be set while @@active is true.
	Q_PROPERTY(QString user READ user WRITE setUser NOTIFY userChanged);
	/// The last message sent by pam.
	Q_PROPERTY(QString message READ message NOTIFY messageChanged);
	/// If the last message should be shown as an error.
	Q_PROPERTY(bool messageIsError READ messageIsError NOTIFY messageIsErrorChanged);
	/// If pam currently wants a response.
	///
	/// Responses can be returned with the @@respond() function.
	Q_PROPERTY(bool responseRequired READ isResponseRequired NOTIFY responseRequiredChanged);
	/// If the user's response should be visible. Only valid when @@responseRequired is true.
	Q_PROPERTY(bool responseVisible READ isResponseVisible NOTIFY responseVisibleChanged);
	// clang-format on

public:
	explicit PamContext(QObject* parent = nullptr): QObject(parent) {}

	void classBegin() override {}
	void componentComplete() override;

	void startConversation();
	void abortConversation();

	/// Start an authentication session. Returns if the session was started successfully.
	Q_INVOKABLE bool start();

	/// Abort a running authentication session.
	Q_INVOKABLE void abort();

	/// Respond to pam.
	///
	/// May not be called unless @@responseRequired is true.
	Q_INVOKABLE void respond(const QString& response);

	[[nodiscard]] bool isActive() const;
	void setActive(bool active);

	[[nodiscard]] QString config() const;
	void setConfig(QString config);

	[[nodiscard]] QString configDirectory() const;
	void setConfigDirectory(QString configDirectory);

	[[nodiscard]] QString user() const;
	void setUser(QString user);

	[[nodiscard]] QString message() const;
	[[nodiscard]] bool messageIsError() const;
	[[nodiscard]] bool isResponseRequired() const;
	[[nodiscard]] bool isResponseVisible() const;

signals:
	/// Emitted whenever authentication completes.
	void completed(PamResult::Enum result);
	/// Emitted if pam fails to perform authentication normally.
	///
	/// A `completed(PamResult.Error)` will be emitted after this event.
	void error(PamError::Enum error);

	/// Emitted whenever pam sends a new message, after the change signals for
	/// `message`, `messageIsError`, and `responseRequired`.
	void pamMessage();

	void activeChanged();
	void configChanged();
	void configDirectoryChanged();
	void userChanged();
	void messageChanged();
	void messageIsErrorChanged();
	void responseRequiredChanged();
	void responseVisibleChanged();

private slots:
	void onCompleted(PamResult::Enum result);
	void onError(PamError::Enum error);
	void onMessage(QString message, bool isError, bool responseRequired, bool responseVisible);

private:
	PamConversation* conversation = nullptr;

	bool postInit = false;
	bool mTargetActive = false;
	QString mConfig = "login";
	QString mConfigDirectory = "/etc/pam.d";
	QString mUser;
	QString mMessage;
	bool mMessageIsError = false;
	bool mIsResponseRequired = false;
	bool mIsResponseVisible = false;
};
