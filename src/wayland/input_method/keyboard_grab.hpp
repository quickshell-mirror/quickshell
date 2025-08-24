#pragma once

#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qwayland-input-method-unstable-v2.h>

#include "key_map_state.hpp"
#include "types.hpp"

namespace qs::wayland::input_method::impl {

class VirtualKeyboardHandle;

class InputMethodPopupSurface: QObject {
	Q_OBJECT;
};

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

	uint32_t mSerial = 0;

	enum class KeyState : bool {
		PRESSED = WL_KEYBOARD_KEY_STATE_PRESSED,
		RELEASED = WL_KEYBOARD_KEY_STATE_RELEASED
	};
	std::vector<KeyState> mKeyState;
	KeyMapState mKeyMapState;

	std::unique_ptr<VirtualKeyboardHandle> mVirturalKeyboard;
};

} // namespace qs::wayland::input_method::impl
