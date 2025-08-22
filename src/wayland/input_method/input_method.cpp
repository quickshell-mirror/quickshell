#include "input_method.hpp"
#include <cstddef>

#include <qdebug.h>
#include <qlogging.h>
#include <qobject.h>
#include <qpointer.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qwayland-input-method-unstable-v2.h>
#include <wayland-input-method-unstable-v2-client-protocol.h>

#include "keyboard_grab.hpp"
#include "manager.hpp"

namespace qs::wayland::input_method::impl {

InputMethodHandle::InputMethodHandle(QObject* parent, ::zwp_input_method_v2* inputMethod)
    : QObject(parent)
    , zwp_input_method_v2(inputMethod) {}

InputMethodHandle::~InputMethodHandle() {
	this->sendPreeditString("");
	this->destroy();
}

void InputMethodHandle::commitString(const QString& text) { commit_string(text); }
void InputMethodHandle::sendPreeditString(
    const QString& text,
    int32_t cursorBegin,
    int32_t cursorEnd
) {
	set_preedit_string(text, cursorBegin, cursorEnd);
}
void InputMethodHandle::deleteText(int before, int after) {
	zwp_input_method_v2::delete_surrounding_text(before, after);
}
void InputMethodHandle::commit() { zwp_input_method_v2::commit(this->serial++); }

bool InputMethodHandle::hasKeyboard() const { return this->keyboard != nullptr; }
QPointer<InputMethodKeyboardGrab> InputMethodHandle::grabKeyboard() {
	if (this->keyboard) return this->keyboard;

	this->keyboard = new InputMethodKeyboardGrab(this, grab_keyboard());

	return this->keyboard;
}
void InputMethodHandle::releaseKeyboard() {
	if (!this->keyboard) return;
	this->keyboard->deleteLater();
	this->keyboard = nullptr;
}

bool InputMethodHandle::isActive() const { return this->mState.activated; }
bool InputMethodHandle::isAvailable() const { return this->mAvailable; }

const QString& InputMethodHandle::surroundingText() const {
	return this->mState.surroundingText.text;
}
uint32_t InputMethodHandle::surroundingTextCursor() const {
	return this->mState.surroundingText.cursor;
}
uint32_t InputMethodHandle::surroundingTextAnchor() const {
	return this->mState.surroundingText.anchor;
}

ContentHint InputMethodHandle::contentHint() const {
	return this->mState.contentHint;
}
ContentPurpose InputMethodHandle::contentPurpose() const {
	return this->mState.contentPurpose;
}

void InputMethodHandle::zwp_input_method_v2_activate() { this->mNewState.activated = true; }

void InputMethodHandle::zwp_input_method_v2_deactivate() { this->mNewState.activated = false; }

void InputMethodHandle::zwp_input_method_v2_surrounding_text(
    const QString& text,
    uint32_t cursor,
    uint32_t anchor
) {
	this->mNewState.surroundingText.text = text;
	this->mNewState.surroundingText.cursor = cursor;
	this->mNewState.surroundingText.anchor = anchor;
}

void InputMethodHandle::zwp_input_method_v2_text_change_cause(uint32_t cause) {
	this->mNewState.surroundingText.textChangeCause = static_cast<TextChangeCause>(cause);
}

void InputMethodHandle::zwp_input_method_v2_content_type(uint32_t hint, uint32_t purpose) {
	this->mNewState.contentHint = static_cast<ContentHint>(hint);
	this->mNewState.contentPurpose = static_cast<ContentPurpose>(purpose);
};

void InputMethodHandle::zwp_input_method_v2_done() {
	auto oldState= mState;
	this->mState = this->mNewState;


	if (this->mState.activated != oldState.activated) {
		if (this->mNewState.activated) emit activated();
		else emit deactivated();
	}

	if (this->mState.surroundingText != oldState.surroundingText) {
		emit surroundingTextChanged(
		    this->mNewState.surroundingText.textChangeCause
		);
	}

	if (this->mState.contentHint != oldState.contentHint) {
		emit contentHintChanged();
	}

	if (this->mState.contentPurpose != oldState.contentPurpose) {
		emit contentPurposeChanged();
	}
}

void InputMethodHandle::zwp_input_method_v2_unavailable() {
	if (!this->mAvailable) return;
	this->mAvailable = false;
	InputMethodManager::instance()->releaseInput();
	qDebug()
	    << "Compositor denied input method request, likely due to one already existing elsewhere";
}

} // namespace qs::wayland::input_method::impl
