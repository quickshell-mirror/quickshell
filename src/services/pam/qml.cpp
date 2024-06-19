#include "qml.hpp"
#include <utility>

#include <pwd.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qlogging.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qtmetamacros.h>
#include <unistd.h>

#include "conversation.hpp"

void PamContext::componentComplete() {
	this->postInit = true;

	if (this->mTargetActive) {
		this->startConversation();
	}
}

void PamContext::startConversation() {
	if (!this->postInit || this->conversation != nullptr) return;

	QString user;

	{
		auto configDirInfo = QFileInfo(this->mConfigDirectory);
		if (!configDirInfo.isDir()) {
			qCritical() << "Cannot start" << this << "because specified config directory"
			            << this->mConfigDirectory << "is not a directory.";
			this->mTargetActive = false;
			return;
		}

		auto configFilePath = QDir(this->mConfigDirectory).filePath(this->mConfig);
		auto configFileInfo = QFileInfo(configFilePath);
		if (!configFileInfo.isFile()) {
			qCritical() << "Cannot start" << this << "because specified config file" << configFilePath
			            << "is not a file.";
			this->mTargetActive = false;
			return;
		}

		auto pwuidbufSize = sysconf(_SC_GETPW_R_SIZE_MAX);
		if (pwuidbufSize == -1) pwuidbufSize = 8192;
		char pwuidbuf[pwuidbufSize]; // NOLINT

		passwd pwuid {};
		passwd* pwuidResult = nullptr;

		if (this->mUser.isEmpty()) {
			auto r = getpwuid_r(getuid(), &pwuid, pwuidbuf, pwuidbufSize, &pwuidResult);
			if (pwuidResult == nullptr) {
				qCritical() << "Cannot start" << this << "due to error in getpwuid_r: " << r;
				this->mTargetActive = false;
				return;
			}

			user = pwuid.pw_name;
		} else {
			auto r = getpwnam_r(
			    this->mUser.toStdString().c_str(),
			    &pwuid,
			    pwuidbuf,
			    pwuidbufSize,
			    &pwuidResult
			);

			if (pwuidResult == nullptr) {
				if (r == 0) {
					qCritical() << "Cannot start" << this
					            << "because specified user was not found: " << this->mUser;
				} else {
					qCritical() << "Cannot start" << this << "due to error in getpwnam_r: " << r;
				}

				this->mTargetActive = false;
				return;
			}

			user = pwuid.pw_name;
		}
	}

	this->conversation = new PamConversation(this);
	QObject::connect(this->conversation, &PamConversation::completed, this, &PamContext::onCompleted);
	QObject::connect(this->conversation, &PamConversation::error, this, &PamContext::onError);
	QObject::connect(this->conversation, &PamConversation::message, this, &PamContext::onMessage);
	emit this->activeChanged();
	this->conversation->start(this->mConfigDirectory, this->mConfig, user);
}

void PamContext::abortConversation() {
	if (this->conversation == nullptr) return;
	this->mTargetActive = false;

	QObject::disconnect(this->conversation, nullptr, this, nullptr);
	this->conversation->deleteLater();
	this->conversation = nullptr;
	emit this->activeChanged();

	if (!this->mMessage.isEmpty()) {
		this->mMessage.clear();
		emit this->messageChanged();
	}

	if (this->mMessageIsError) {
		this->mMessageIsError = false;
		emit this->messageIsErrorChanged();
	}

	if (this->mIsResponseRequired) {
		this->mIsResponseRequired = false;
		emit this->responseRequiredChanged();
	}
}

void PamContext::respond(const QString& response) {
	if (this->isActive() && this->mIsResponseRequired) {
		this->conversation->respond(response);
	} else {
		qWarning() << "PamContext response was ignored as this context does not require one.";
	}
}

bool PamContext::start() {
	this->setActive(true);
	return this->isActive();
}

void PamContext::abort() { this->setActive(false); }

bool PamContext::isActive() const { return this->conversation != nullptr; }

void PamContext::setActive(bool active) {
	if (active == this->mTargetActive) return;
	this->mTargetActive = active;

	if (active) this->startConversation();
	else this->abortConversation();
}

QString PamContext::config() const { return this->mConfig; }

void PamContext::setConfig(QString config) {
	if (config == this->mConfig) return;

	if (this->isActive()) {
		qCritical() << "Cannot set config on PamContext while it is active.";
		return;
	}

	this->mConfig = std::move(config);
	emit this->configChanged();
}

QString PamContext::configDirectory() const { return this->mConfigDirectory; }

void PamContext::setConfigDirectory(QString configDirectory) {
	if (configDirectory == this->mConfigDirectory) return;

	if (this->isActive()) {
		qCritical() << "Cannot set configDirectory on PamContext while it is active.";
		return;
	}

	auto* context = QQmlEngine::contextForObject(this);
	if (context != nullptr) {
		configDirectory = context->resolvedUrl(configDirectory).path();
	}

	this->mConfigDirectory = std::move(configDirectory);
	emit this->configDirectoryChanged();
}

QString PamContext::user() const { return this->mUser; }

void PamContext::setUser(QString user) {
	if (user == this->mUser) return;

	if (this->isActive()) {
		qCritical() << "Cannot set user on PamContext while it is active.";
		return;
	}

	this->mUser = std::move(user);
	emit this->userChanged();
}

QString PamContext::message() const { return this->mMessage; }
bool PamContext::messageIsError() const { return this->mMessageIsError; }
bool PamContext::isResponseRequired() const { return this->mIsResponseRequired; }
bool PamContext::isResponseVisible() const { return this->mIsResponseVisible; }

void PamContext::onCompleted(PamResult::Enum result) {
	this->abortConversation();
	emit this->completed(result);
}

void PamContext::onError(PamError::Enum error) {
	this->abortConversation();
	emit this->error(error);
	emit this->completed(PamResult::Error);
}

void PamContext::onMessage(
    QString message,
    bool isError,
    bool responseRequired,
    bool responseVisible
) {
	if (message != this->mMessage) {
		this->mMessage = std::move(message);
		emit this->messageChanged();
	}

	if (isError != this->mMessageIsError) {
		this->mMessageIsError = isError;
		emit this->messageIsErrorChanged();
	}

	if (responseVisible != this->mIsResponseVisible) {
		this->mIsResponseVisible = responseVisible;
		emit this->responseVisibleChanged();
	}

	if (responseRequired != this->mIsResponseRequired) {
		this->mIsResponseRequired = responseRequired;
		emit this->responseRequiredChanged();
	}

	emit this->pamMessage();
}
