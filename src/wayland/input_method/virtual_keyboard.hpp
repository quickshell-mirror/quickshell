#pragma once

#include <qobject.h>
#include <qtclasshelpermacros.h>
#include <qwayland-virtual-keyboard-unstable-v1.h>
#include <xkbcommon/xkbcommon.h>

#include "key_map_state.hpp"

namespace qs::wayland::input_method::impl {

class VirtualKeyboardHandle: private QtWayland::zwp_virtual_keyboard_v1 {

public:
	explicit VirtualKeyboardHandle(::zwp_virtual_keyboard_v1* keyboard, const KeyMapState& keymap);
	~VirtualKeyboardHandle() override;
	Q_DISABLE_COPY_MOVE(VirtualKeyboardHandle);

	KeyMapState getKeyMapState();
	void setKeymapState(const KeyMapState& keymap);

	void sendKey(xkb_keycode_t keycode, wl_keyboard_key_state state);
	void sendModifiers();

	[[nodiscard]] bool getGood() const;

private:
	KeyMapState mKeyMapState;
	bool good = false;
};

} // namespace qs::wayland::input_method::impl
