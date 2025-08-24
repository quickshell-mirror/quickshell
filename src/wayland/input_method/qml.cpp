#include "qml.hpp"
#include <cassert>
#include <cstddef>
#include <utility>

#include <qlogging.h>
#include <qobject.h>
#include <qpointer.h>
#include <qqmlengine.h>
#include <qqmlinfo.h>
#include <qstring.h>

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
	emit keyboardComponentChanged();

	if (this->keyboard) {
		this->keyboard->deleteLater();
		this->keyboard = nullptr;
	}

	this->handleKeyboardActive();
}

bool InputMethod::hasInput() const { return handle && handle->isAvailable(); }

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

	emit hasInputChanged();
}

void InputMethod::releaseInput() {
	if (!this->handle) return;
	InputMethodManager::instance()->releaseInput();
	emit hasInputChanged();
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
	instance->setKeyboard(handle->grabKeyboard());
	// Always have a way to release the keyboard
	QObject::connect(instance, &Keyboard::escapePress, this, &InputMethod::releaseKeyboard);

	this->keyboard = instance;
	emit hasKeyboardChanged();
}

void InputMethod::releaseKeyboard() {
	if (!this->hasKeyboard()) return;
	this->keyboard->deleteLater();
	this->keyboard = nullptr;
	this->handle->releaseKeyboard();
	if (this->mClearPreeditOnKeyboardRelease) this->sendPreeditString("");
	emit hasKeyboardChanged();
}

void InputMethod::handleKeyboardActive() {
	if (!this->mKeyboardComponent) return;
	if (this->keyboard) {
		this->releaseKeyboard();
	}
}

void InputMethod::onHandleActiveChanged() {
	this->handleKeyboardActive();
	emit activeChanged();
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
	    &InputMethodKeyboardGrab::directionPress,
	    this,
	    &Keyboard::onDirectionPress
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

void Keyboard::onDirectionPress(impl::DirectionKey direction) {
	emit directionPress(KeyboardDirectionKey::fromDirection(direction));
}

QString KeyboardDirectionKey::toString(Enum direction) {
	switch (direction) {
	case UP: return "UP";
	case DOWN: return "DOWN";
	case LEFT: return "LEFT";
	case RIGHT: return "RIGHT";
	}
	return "UNKNOWN";
}
KeyboardDirectionKey::Enum KeyboardDirectionKey::fromDirection(impl::DirectionKey direction) {
	return static_cast<KeyboardDirectionKey::Enum>(direction);
}

} // namespace qs::wayland::input_method
