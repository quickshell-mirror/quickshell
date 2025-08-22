#pragma once

#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qwayland-input-method-unstable-v2.h>
#include <qtimer.h>

#include "key_map_state.hpp"
#include "types.hpp"

namespace qs::wayland::input_method::impl {

class VirtualKeyboardHandle;

// TODO(cmurtagh-lgtm) Implement the popup-surface part of the protocol
class InputMethodPopupSurface: QObject {
	Q_OBJECT;
};

/// A wrapper around the input_method_keyboard_grab protocol
/// Only some control keys and keys that are representable by utf32 are captured,
/// other keys are passed back to the compositor via virtual keyboard.
class InputMethodKeyboardGrab
    : public QObject
    , private QtWayland::zwp_input_method_keyboard_grab_v2 {
	Q_OBJECT;

public:
	explicit InputMethodKeyboardGrab(QObject* parent, ::zwp_input_method_keyboard_grab_v2* keyboard);
	~InputMethodKeyboardGrab() override;
	Q_DISABLE_COPY_MOVE(InputMethodKeyboardGrab);

signals:
	void keyPress(QChar character);
	void escapePress();
	void returnPress();
	void directionPress(DirectionKey);
	void backspacePress();
	void deletePress();

private:
	void
	zwp_input_method_keyboard_grab_v2_keymap(uint32_t format, int32_t fd, uint32_t size) override;
	void zwp_input_method_keyboard_grab_v2_key(
	    uint32_t serial,
	    uint32_t time,
	    uint32_t key,
	    uint32_t state
	) override;
	void zwp_input_method_keyboard_grab_v2_modifiers(
	    uint32_t serial,
	    uint32_t modsDepressed,
	    uint32_t modsLatched,
	    uint32_t modsLocked,
	    uint32_t group
	) override;
	void zwp_input_method_keyboard_grab_v2_repeat_info(int32_t rate, int32_t delay) override;

	bool handleKey(xkb_keycode_t key);

	uint32_t mSerial = 0;

	enum class KeyState : bool {
		PRESSED = WL_KEYBOARD_KEY_STATE_PRESSED,
		RELEASED = WL_KEYBOARD_KEY_STATE_RELEASED
	};
	std::vector<KeyState> mKeyState;
	KeyMapState mKeyMapState;

	QTimer mRepeatTimer;
	// Keys per second
	int32_t mRepeatRate = 25;
	// milliseconds before first repeat
	int32_t mRepeatDelay = 400;
	xkb_keycode_t mRepeatKey = 0;

	std::unique_ptr<VirtualKeyboardHandle> mVirturalKeyboard;
};

} // namespace qs::wayland::input_method::impl
