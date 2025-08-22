#include "input_method.hpp"
#include <cstddef>

#include <qdebug.h>
#include <qlogging.h>
#include <qobject.h>
#include <qpointer.h>
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

bool InputMethodHandle::isActive() const { return this->mActivated; }
bool InputMethodHandle::isAvailable() const { return this->mAvailable; }

void InputMethodHandle::zwp_input_method_v2_activate() { this->mNewActive = true; }

void InputMethodHandle::zwp_input_method_v2_deactivate() { this->mNewActive = false; }

void InputMethodHandle::zwp_input_method_v2_done() {
	if (this->mActivated == this->mNewActive) return;
	this->mActivated = this->mNewActive;
	if (this->mActivated) emit activated();
	else emit deactivated();
}

void InputMethodHandle::zwp_input_method_v2_unavailable() {
	if (!this->mAvailable) return;
	this->mAvailable = false;
	InputMethodManager::instance()->releaseInput();
	qDebug()
	    << "Compositor denied input method request, likely due to one already existing elsewhere";
}

} // namespace qs::wayland::input_method::impl
