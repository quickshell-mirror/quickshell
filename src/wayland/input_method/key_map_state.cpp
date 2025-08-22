#include "key_map_state.hpp"
#include <string_view>
#include <utility>

#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "c_helpers.hpp"

namespace qs::wayland::input_method::impl {

KeyMapState::KeyMapState(const char* string)
    : xkbKeymap(xkb_keymap_new_from_string(
          xkbContext,
          string,
          XKB_KEYMAP_FORMAT_TEXT_V1,
          XKB_KEYMAP_COMPILE_NO_FLAGS
      ))
    , xkbState(xkb_state_new(this->xkbKeymap)) {}
KeyMapState::KeyMapState(const KeyMapState& state)
    : xkbKeymap(xkb_keymap_ref(state.xkbKeymap))
    , xkbState(xkb_state_ref(state.xkbState)) {}
KeyMapState::KeyMapState(KeyMapState&& state) noexcept
    : xkbKeymap(std::exchange(state.xkbKeymap, nullptr))
    , xkbState(std::exchange(state.xkbState, nullptr)) {}
KeyMapState& KeyMapState::operator=(const KeyMapState& state) {
	if (this == &state) return *this;
	this->xkbKeymap = xkb_keymap_ref(state.xkbKeymap);
	this->xkbState = xkb_state_ref(state.xkbState);
	return *this;
}
KeyMapState& KeyMapState::operator=(KeyMapState&& state) noexcept {
	if (this == &state) return *this;
	this->xkbKeymap = std::exchange(state.xkbKeymap, nullptr);
	this->xkbState = std::exchange(state.xkbState, nullptr);
	return *this;
}
KeyMapState::~KeyMapState() {
	xkb_keymap_unref(std::exchange(this->xkbKeymap, nullptr));
	xkb_state_unref(std::exchange(this->xkbState, nullptr));
}

uniqueCString KeyMapState::keyMapAsString() const {
	return uniqueCString(xkb_keymap_get_as_string(this->xkbKeymap, XKB_KEYMAP_FORMAT_TEXT_V1));
}

bool KeyMapState::operator==(const KeyMapState& other) const {
	return this->xkbKeymap == other.xkbKeymap && this->xkbState == other.xkbState;
}
KeyMapState::operator bool() const {
	return this->xkbKeymap != nullptr && this->xkbState != nullptr;
}

const char* KeyMapState::keyName(xkb_keycode_t key) const {
	return xkb_keymap_key_get_name(this->xkbKeymap, key);
}

xkb_keycode_t KeyMapState::maxKeycode() const { return xkb_keymap_max_keycode(this->xkbKeymap); }
xkb_keycode_t KeyMapState::minKeycode() const { return xkb_keymap_min_keycode(this->xkbKeymap); }

xkb_keysym_t KeyMapState::getOneSym(xkb_keycode_t key) const {
	return xkb_state_key_get_one_sym(this->xkbState, key);
}
QChar KeyMapState::getChar(xkb_keycode_t key) const {
	return QChar(xkb_state_key_get_utf32(this->xkbState, key));
}

void KeyMapState::setModifiers(ModifierState modifierState) {
	if (!*this) return;
	xkb_state_update_mask(
	    this->xkbState,
	    modifierState.depressed,
	    modifierState.latched,
	    modifierState.locked,
	    modifierState.depressedLayout,
	    modifierState.latchedLayout,
	    modifierState.lockedLayout
	);
}

KeyMapState::ModifierState KeyMapState::serialiseMods() const {
	return {
	    .depressed = xkb_state_serialize_mods(this->xkbState, XKB_STATE_MODS_DEPRESSED),
	    .latched = xkb_state_serialize_mods(this->xkbState, XKB_STATE_MODS_LATCHED),
	    .locked = xkb_state_serialize_mods(this->xkbState, XKB_STATE_MODS_LOCKED),
	    .depressedLayout = xkb_state_serialize_mods(this->xkbState, XKB_STATE_LAYOUT_DEPRESSED),
	    .latchedLayout = xkb_state_serialize_mods(this->xkbState, XKB_STATE_LAYOUT_LATCHED),
	    .lockedLayout = xkb_state_serialize_mods(this->xkbState, XKB_STATE_LAYOUT_LOCKED),
	};
}

std::string_view KeyMapState::keyStateName(wl_keyboard_key_state state) {
	switch (state) {
	case WL_KEYBOARD_KEY_STATE_RELEASED: return "released";
	case WL_KEYBOARD_KEY_STATE_PRESSED: return "pressed";
	default: return "unknown state";
	}
}

bool KeyMapState::keyRepeats(xkb_keycode_t key) const {
	 return xkb_keymap_key_repeats(this->xkbKeymap, key);
}

xkb_context* KeyMapState::xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

} // namespace qs::wayland::input_method::impl
