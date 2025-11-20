#include "qml.hpp"
#include <cassert>
#include <cstdint>
#include <utility>

#include <qlogging.h>
#include <qobject.h>
#include <qpointer.h>
#include <qqmlengine.h>
#include <qqmlinfo.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "input_method.hpp"
#include "keyboard_grab.hpp"
#include "manager.hpp"
#include "types.hpp"

namespace qs::wayland::input_method {

using namespace impl;

InputMethod::InputMethod(QObject* parent): QObject(parent) { this->getInput(); }

void InputMethod::sendString(const QString& text) {
	if (!this->isActive()) return;

	this->handle->commitString(text);
	this->handle->commit();
}

void InputMethod::sendPreeditString(const QString& text, int32_t cursorBegin, int32_t cursorEnd) {
	if (!this->isActive()) return;

	this->handle->sendPreeditString(text, cursorBegin, cursorEnd);
	this->handle->commit();
}

void InputMethod::deleteText(int before, int after) {
	if (!this->isActive()) return;

	this->handle->deleteText(before, after);
	this->handle->commit();
}

bool InputMethod::isActive() const { return this->hasInput() && this->handle->isActive(); }

QQmlComponent* InputMethod::keyboardComponent() const { return this->mKeyboardComponent; }
void InputMethod::setKeyboardComponent(QQmlComponent* keyboardComponent) {
	if (this->mKeyboardComponent == keyboardComponent) return;
	this->mKeyboardComponent = keyboardComponent;
	emit this->keyboardComponentChanged();

	if (this->keyboard) {
		this->keyboard->deleteLater();
		this->keyboard = nullptr;
	}

	this->handleKeyboardActive();
}

bool InputMethod::hasInput() const { return this->handle && this->handle->isAvailable(); }

void InputMethod::getInput() {
	if (this->hasInput()) return;

	this->handle = InputMethodManager::instance()->acquireInput();

	QObject::connect(
	    this->handle.get(),
	    &InputMethodHandle::activated,
	    this,
	    &InputMethod::onHandleActiveChanged
	);
	QObject::connect(
	    this->handle.get(),
	    &InputMethodHandle::deactivated,
	    this,
	    &InputMethod::onHandleActiveChanged
	);
	QObject::connect(
	    this->handle.get(),
	    &InputMethodHandle::contentHintChanged,
	    this,
	    &InputMethod::contentHintChanged
	);
	QObject::connect(
	    this->handle.get(),
	    &InputMethodHandle::contentPurposeChanged,
	    this,
	    &InputMethod::contentPurposeChanged
	);
	QObject::connect(
	    this->handle.get(),
	    &InputMethodHandle::surroundingTextChanged,
	    this,
	    &InputMethod::surroundingTextChanged
	);

	emit this->hasInputChanged();
}

void InputMethod::releaseInput() {
	if (!this->handle) return;
	InputMethodManager::instance()->releaseInput();
	emit this->hasInputChanged();
}

bool InputMethod::hasKeyboard() const {
	// The lifetime of keyboard should be less than handle's
	if (this->keyboard) {
		assert(this->handle->hasKeyboard());
	}
	return this->keyboard;
}

void InputMethod::grabKeyboard() {
	if (this->hasKeyboard()) return;
	if (this->handle->hasKeyboard()) {
		qmlDebug(this) << "Only one input method can grad a keyboard at any one time";
		return;
	}
	auto* instanceObj =
	    this->mKeyboardComponent->create(QQmlEngine::contextForObject(this->mKeyboardComponent));
	auto* instance = qobject_cast<Keyboard*>(instanceObj);

	if (instance == nullptr) {
		qWarning() << "Failed to create input method keyboard component";
		if (instanceObj != nullptr) instanceObj->deleteLater();
		return;
	}

	instance->setParent(this);
	instance->setKeyboard(this->handle->grabKeyboard());
	// Always have a way to release the keyboard
	QObject::connect(instance, &Keyboard::escapePress, this, &InputMethod::releaseKeyboard);

	this->keyboard = instance;
	emit this->hasKeyboardChanged();
}

void InputMethod::releaseKeyboard() {
	if (!this->hasKeyboard()) return;
	this->keyboard->deleteLater();
	this->keyboard = nullptr;
	this->handle->releaseKeyboard();
	if (this->mClearPreeditOnKeyboardRelease) this->sendPreeditString("");
	emit this->hasKeyboardChanged();
}

void InputMethod::handleKeyboardActive() {
	if (!this->mKeyboardComponent) return;
	if (this->keyboard) {
		this->releaseKeyboard();
	}
}

const QString& InputMethod::surroundingText() const { return this->handle->surroundingText(); }
uint32_t InputMethod::surroundingTextCursor() const { return this->handle->surroundingTextCursor(); }
uint32_t InputMethod::surroundingTextAnchor() const { return this->handle->surroundingTextAnchor(); }

QMLContentHint::Enum InputMethod::contentHint() const { return this->handle->contentHint(); }
QMLContentPurpose::Enum InputMethod::contentPurpose() const {
	return this->handle->contentPurpose();
}

void InputMethod::onHandleActiveChanged() {
	this->handleKeyboardActive();
	emit this->activeChanged();
}

Keyboard::Keyboard(QObject* parent): QObject(parent) {}

void Keyboard::setKeyboard(QPointer<InputMethodKeyboardGrab> keyboard) {
	if (this->mKeyboard == keyboard) return;
	if (keyboard == nullptr) return;
	this->mKeyboard = std::move(keyboard);

	QObject::connect(this->mKeyboard, &InputMethodKeyboardGrab::keyPress, this, &Keyboard::keyPress);
	QObject::connect(
	    this->mKeyboard,
	    &InputMethodKeyboardGrab::escapePress,
	    this,
	    &Keyboard::escapePress
	);
	QObject::connect(
	    this->mKeyboard,
	    &InputMethodKeyboardGrab::returnPress,
	    this,
	    &Keyboard::returnPress
	);
	QObject::connect(
	    this->mKeyboard,
	    &InputMethodKeyboardGrab::backspacePress,
	    this,
	    &Keyboard::backspacePress
	);
	QObject::connect(
	    this->mKeyboard,
	    &InputMethodKeyboardGrab::deletePress,
	    this,
	    &Keyboard::deletePress
	);
}

} // namespace qs::wayland::input_method
