#include "qml.hpp"
#include <cassert>
#include <cstdint>
#include <utility>

#include <qlogging.h>
#include <qobject.h>
#include <qpointer.h>
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

void InputMethod::commitString(const QString& text) {
	if (!this->isActive()) return;

	this->handle->commitString(text);
}

void InputMethod::setPreeditString() {
	if (!this->isActive()) return;

	if (mCursorEnd == -1) {
		this->handle->setPreeditString(this->mPreeditString, this->mCursorBegin, this->mCursorBegin);
	} else {
		this->handle->setPreeditString(this->mPreeditString, this->mCursorBegin, this->mCursorEnd);
	}
}

void InputMethod::deleteSuroundingText(int before, int after) {
	if (!this->isActive()) return;

	this->handle->deleteText(before, after);
}

void InputMethod::commit() {
	if (!this->isActive()) return;

	this->handle->commit();
}

bool InputMethod::isActive() const { return this->hasInput() && this->handle->isActive(); }

QPointer<Keyboard> InputMethod::keyboard() const { return this->mKeyboard; }

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
	if (this->mKeyboard) {
		assert(this->handle->hasKeyboard());
	}
	return this->mKeyboard;
}

void InputMethod::grabKeyboard() {
	if (this->hasKeyboard()) return;
	if (this->handle->hasKeyboard()) {
		qmlDebug(this) << "Only one input method can grad a keyboard at any one time";
		return;
	}
	this->mKeyboard = new Keyboard(this);

	this->mKeyboard->setKeyboard(this->handle->grabKeyboard());
	// Always have a way to release the keyboard
	QObject::connect(this->mKeyboard, &Keyboard::escapePress, this, &InputMethod::releaseKeyboard);

	emit this->hasKeyboardChanged();
}

void InputMethod::releaseKeyboard() {
	if (!this->hasKeyboard()) return;
	delete this->mKeyboard;
	this->mKeyboard = nullptr;
	this->handle->releaseKeyboard();
	if (this->mClearPreeditOnKeyboardRelease) {
		this->mPreeditString = "";
		this->mCursorBegin = -1;
		this->mCursorEnd = -1;
		this->setPreeditString();
		this->preeditStringChanged();
		this->cursorBeginChanged();
		this->cursorEndChanged();
	}
	emit this->hasKeyboardChanged();
}

const QString& InputMethod::preeditString() const { return this->mPreeditString; }
QString& InputMethod::preeditString() { return this->mPreeditString; }
void InputMethod::setPreeditString(const QString& string) {
	if (this->mPreeditString == string) return;
	this->mPreeditString = string;
	this->setPreeditString();
	this->commit();
	this->preeditStringChanged();
}
int32_t InputMethod::cursorBegin() const { return this->mCursorBegin; }
void InputMethod::setCursorBegin(int32_t position) {
	if (this->mCursorBegin == position) return;
	this->mCursorBegin = position;
	this->setPreeditString();
	this->commit();
	this->cursorBeginChanged();
}
int32_t InputMethod::cursorEnd() const { return this->mCursorEnd; }
void InputMethod::setCursorEnd(int32_t position) {
	if (this->mCursorEnd == position) return;
	this->mCursorEnd = position;
	this->setPreeditString();
	this->commit();
	this->cursorEndChanged();
}

void InputMethod::handleKeyboardActive() {
	if (this->mKeyboard) {
		this->releaseKeyboard();
	}
}

const QString& InputMethod::surroundingText() const { return this->handle->surroundingText(); }
uint32_t InputMethod::surroundingTextCursor() const {
	return this->handle->surroundingTextCursor();
}
uint32_t InputMethod::surroundingTextAnchor() const {
	return this->handle->surroundingTextAnchor();
}

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
