#include "connection.hpp"
#include <utility>

#include <qcontainerfwd.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlocalsocket.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../../core/generation.hpp"
#include "../../core/logcat.hpp"

namespace {
QS_LOGGING_CATEGORY(logGreetd, "quickshell.service.greetd");
}

QString GreetdState::toString(GreetdState::Enum value) {
	switch (value) {
	case GreetdState::Inactive: return "Inactive";
	case GreetdState::Authenticating: return "Authenticating";
	case GreetdState::ReadyToLaunch: return "Ready to Launch";
	case GreetdState::Launching: return "Launching";
	case GreetdState::Launched: return "Launched";
	default: return "Invalid";
	}
}

GreetdConnection::GreetdConnection() {
	auto socket = qEnvironmentVariable("GREETD_SOCK");

	if (socket.isEmpty()) {
		this->mAvailable = false;
		return;
	}

	this->mAvailable = true;

	// clang-format off
	QObject::connect(&this->socket, &QLocalSocket::connected, this, &GreetdConnection::onSocketConnected);
	QObject::connect(&this->socket, &QLocalSocket::readyRead, this, &GreetdConnection::onSocketReady);
	// clang-format on

	this->socket.connectToServer(socket, QLocalSocket::ReadWrite);
}

void GreetdConnection::createSession(QString user) {
	if (!this->mAvailable) {
		qCCritical(logGreetd) << "Greetd is not available.";
		return;
	}

	if (user != this->mUser) {
		this->mUser = std::move(user);
		emit this->userChanged();
	}

	this->setActive(true);
}

void GreetdConnection::cancelSession() { this->setActive(false); }

void GreetdConnection::respond(QString response) {
	if (!this->mResponseRequired) {
		qCCritical(logGreetd) << "Cannot respond to greetd as a response is not currently required.";
		return;
	}

	this->sendRequest({
	    {"type", "post_auth_message_response"},
	    {"response", response},
	});

	this->mResponseRequired = false;
}

void GreetdConnection::launch(
    const QList<QString>& command,
    const QList<QString>& environment,
    bool exit
) {
	if (this->mState != GreetdState::ReadyToLaunch) {
		qCCritical(logGreetd) << "Cannot call launch() as state is not currently ReadyToLaunch.";
		return;
	}

	this->mState = GreetdState::Launching;
	this->mExitAfterLaunch = exit;

	this->sendRequest({
	    {"type", "start_session"},
	    {"cmd", QJsonArray::fromStringList(command)},
	    {"env", QJsonArray::fromStringList(environment)},
	});
}

bool GreetdConnection::isAvailable() const { return this->mAvailable; }
GreetdState::Enum GreetdConnection::state() const { return this->mState; }

void GreetdConnection::setActive(bool active) {
	if (this->socket.state() == QLocalSocket::ConnectedState) {
		this->mTargetActive = active;
		if (active == (this->mState != GreetdState::Inactive)) return;

		if (active) {
			if (this->mUser.isEmpty()) {
				qCCritical(logGreetd) << "Cannot activate greetd with unset user.";
				this->setActive(false);
				return;
			}

			this->sendRequest({
			    {"type", "create_session"},
			    {"username", this->mUser},
			});

			this->mState = GreetdState::Authenticating;
			emit this->stateChanged();
		} else {
			this->sendRequest({
			    {"type", "cancel_session"},
			});

			this->setInactive();
		}
	} else {
		if (active != this->mTargetActive) {
			this->mTargetActive = active;
		}
	}
}

void GreetdConnection::setInactive() {
	this->mTargetActive = false;
	this->mResponseRequired = false;
	this->mState = GreetdState::Inactive;
	emit this->stateChanged();
}

QString GreetdConnection::user() const { return this->mUser; }

void GreetdConnection::onSocketConnected() {
	qCDebug(logGreetd) << "Connected to greetd socket.";

	if (this->mTargetActive) {
		this->setActive(true);
	}
}

void GreetdConnection::onSocketError(QLocalSocket::LocalSocketError error) {
	qCCritical(logGreetd) << "Greetd socket encountered an error and cannot continue:" << error;

	this->mAvailable = false;
	this->setActive(false);
}

void GreetdConnection::onSocketReady() {
	qint32 length = 0;

	this->socket.read(reinterpret_cast<char*>(&length), sizeof(qint32));

	auto text = this->socket.read(length);
	auto json = QJsonDocument::fromJson(text).object();
	auto type = json.value("type").toString();

	qCDebug(logGreetd).noquote() << "Received greetd response:" << text;

	if (type == "success") {
		switch (this->mState) {
		case GreetdState::Authenticating:
			qCDebug(logGreetd) << "Authentication complete.";
			this->mState = GreetdState::ReadyToLaunch;
			emit this->stateChanged();
			emit this->readyToLaunch();
			break;
		case GreetdState::Launching:
			qCDebug(logGreetd) << "Target session set successfully.";
			this->mState = GreetdState::Launched;
			emit this->stateChanged();
			emit this->launched();

			if (this->mExitAfterLaunch) {
				qCDebug(logGreetd) << "Quitting.";
				EngineGeneration::currentGeneration()->quit();
			}

			break;
		default: goto unexpected;
		}
	} else if (type == "error") {
		auto errorType = json.value("error_type").toString();
		auto desc = json.value("description").toString();

		// Special case this error in case a session was already running.
		// This cancels and restarts the session.
		if (errorType == "error" && desc == "a session is already being configured") {
			qCDebug(logGreetd
			) << "A session was already in progress, cancelling it and starting a new one.";
			this->setActive(false);
			this->setActive(true);
			return;
		}

		if (errorType == "auth_error") {
			emit this->authFailure(desc);
			this->setActive(false);
		} else if (errorType == "error") {
			qCWarning(logGreetd) << "Greetd error occurred" << desc;
			emit this->error(desc);
		} else goto unexpected;

		// errors terminate the session
		this->setInactive();
	} else if (type == "auth_message") {
		auto message = json.value("auth_message").toString();
		auto type = json.value("auth_message_type").toString();
		auto error = type == "error";
		auto responseRequired = type == "visible" || type == "secret";
		auto echoResponse = type != "secret";

		this->mResponseRequired = responseRequired;
		emit this->authMessage(message, error, responseRequired, echoResponse);
	} else goto unexpected;

	return;
unexpected:
	qCCritical(logGreetd) << "Received unexpected greetd response" << text;
	this->setActive(false);
}

void GreetdConnection::sendRequest(const QJsonObject& json) {
	auto text = QJsonDocument(json).toJson(QJsonDocument::Compact);
	auto length = static_cast<qint32>(text.length());

	if (logGreetd().isDebugEnabled()) {
		auto debugJson = json;

		if (json.value("type").toString() == "post_auth_message_response") {
			debugJson["response"] = "<CENSORED>";
		}

		qCDebug(logGreetd).noquote() << "Sending greetd request:"
		                             << QJsonDocument(debugJson).toJson(QJsonDocument::Compact);
	}

	this->socket.write(reinterpret_cast<char*>(&length), sizeof(qint32));

	this->socket.write(text);
	this->socket.flush();
}

GreetdConnection* GreetdConnection::instance() {
	static auto* instance = new GreetdConnection(); // NOLINT
	return instance;
}
