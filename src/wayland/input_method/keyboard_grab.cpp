#include "keyboard_grab.hpp"
#include <cassert>
#include <cstdint>
#include <vector>

#if INPUT_METHOD_PRINT
#include <qdebug.h>
#endif
#include <qobject.h>
#include <qtmetamacros.h>
#include <qwayland-input-method-unstable-v2.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-input-method-unstable-v2-client-protocol.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "manager.hpp"
#include "types.hpp"
#include "virtual_keyboard.hpp"

namespace qs::wayland::input_method::impl {

InputMethodKeyboardGrab::InputMethodKeyboardGrab(
    QObject* parent,
    ::zwp_input_method_keyboard_grab_v2* keyboard
)
    : QObject(parent)
    , zwp_input_method_keyboard_grab_v2(keyboard) {
	this->mRepeatTimer.callOnTimeout(this, [&](){
		this->mRepeatTimer.setInterval(1000 / this->mRepeatRate);
		handleKey(mRepeatKey);
	});
}

InputMethodKeyboardGrab::~InputMethodKeyboardGrab() {
	this->release();

	// Release forward the pressed keys to the text input
	for (xkb_keycode_t key = 0; key < this->mKeyState.size(); ++key) {
		if (this->mKeyState[key] == KeyState::PRESSED) {
			this->mVirturalKeyboard->sendKey(
			    key + this->mKeyMapState.minKeycode(),
			    WL_KEYBOARD_KEY_STATE_PRESSED
			);
		}
	}
}

void InputMethodKeyboardGrab::zwp_input_method_keyboard_grab_v2_keymap(
    uint32_t format [[maybe_unused]],
    int32_t fd,
    uint32_t size
) {
	// https://wayland-book.com/seat/example.html
	assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

	char* mapShm = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
	assert(mapShm != MAP_FAILED);

	this->mKeyMapState = KeyMapState(mapShm);

	munmap(mapShm, size);
	close(fd);

	this->mVirturalKeyboard =
	    VirtualKeyboardManager::instance()->createVirtualKeyboard(this->mKeyMapState);
	this->mKeyState = std::vector<KeyState>(
	    this->mKeyMapState.maxKeycode() - this->mKeyMapState.minKeycode() - WAYLAND_KEY_OFFSET,
	    KeyState::RELEASED
	);

	// Tell the text input to release all the keys
	for (xkb_keycode_t key = 0; key < this->mKeyState.size(); ++key) {
		this->mVirturalKeyboard->sendKey(
		    key + this->mKeyMapState.minKeycode(),
		    WL_KEYBOARD_KEY_STATE_RELEASED
		);
	}
}

void InputMethodKeyboardGrab::zwp_input_method_keyboard_grab_v2_key(
    uint32_t serial,
    uint32_t /*time*/,
    uint32_t key,
    uint32_t state
) {
	if (serial <= this->mSerial) return;
	this->mSerial = serial;

	key += WAYLAND_KEY_OFFSET;

#if INPUT_METHOD_PRINT
	qDebug() << KeyMapState::keyStateName(static_cast<wl_keyboard_key_state>(state))
	         << this->mKeyMapState.keyName(key) << "[" << key << "]"
	         << this->mKeyMapState.getChar(key);
#endif

	const xkb_keysym_t sym = this->mKeyMapState.getOneSym(key);

	if (sym == XKB_KEY_Escape) {
		if (state == WL_KEYBOARD_KEY_STATE_PRESSED) emit escapePress();
		return;
	}
	if (sym == XKB_KEY_Return) {
		if (state == WL_KEYBOARD_KEY_STATE_PRESSED) emit returnPress();
		return;
	}

	// Skip adding the control keys because we've consumed them
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		this->mKeyState[key - this->mKeyMapState.minKeycode()] = KeyState::PRESSED;
	} else if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
		this->mKeyState[key - this->mKeyMapState.minKeycode()] = KeyState::RELEASED;
	}

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		const bool keyHandled = handleKey(key);
		if (keyHandled){
			if (this->mKeyMapState.keyRepeats(key) && this->mRepeatRate > 0) {
				this->mRepeatKey = key;
				this->mRepeatTimer.setInterval(this->mRepeatDelay);
				this->mRepeatTimer.start();
			}
			return;
		}
	}

	if (this->mRepeatKey == key) {
		this->mRepeatTimer.stop();
	}

	this->mVirturalKeyboard->sendKey(key, static_cast<wl_keyboard_key_state>(state));
}

bool InputMethodKeyboardGrab::handleKey(xkb_keycode_t key){
	const xkb_keysym_t sym = this->mKeyMapState.getOneSym(key);
	if (sym == XKB_KEY_Up) {
		emit directionPress(DirectionKey::Up);
		return true;
	}
	if (sym == XKB_KEY_Down) {
		emit directionPress(DirectionKey::Down);
		return true;
	}
	if (sym == XKB_KEY_Left) {
		emit directionPress(DirectionKey::Left);
		return true;
	}
	if (sym == XKB_KEY_Right) {
		emit directionPress(DirectionKey::Right);
		return true;
	}
	if (sym == XKB_KEY_BackSpace) {
		emit backspacePress();
		return true;
	}
	if (sym == XKB_KEY_Delete) {
		emit deletePress();
		return true;
	}

	const QChar character = this->mKeyMapState.getChar(key);
	if (character != '\0') {
		emit keyPress(character);
		return true;
	}
	return false;
}

void InputMethodKeyboardGrab::zwp_input_method_keyboard_grab_v2_modifiers(
    uint32_t serial,
    uint32_t modsDepressed,
    uint32_t modsLatched,
    uint32_t modsLocked,
    uint32_t group
) {
	if (serial <= this->mSerial) return;
	this->mSerial = serial;
	this->mKeyMapState.setModifiers(
	    KeyMapState::ModifierState(modsDepressed, modsLatched, modsLocked, group, group, group)
	);
	this->mVirturalKeyboard->sendModifiers();
}

void InputMethodKeyboardGrab::zwp_input_method_keyboard_grab_v2_repeat_info(
    int32_t rate,
    int32_t delay
) {
	mRepeatRate = rate;
	mRepeatDelay = delay;
}

} // namespace qs::wayland::input_method::impl
