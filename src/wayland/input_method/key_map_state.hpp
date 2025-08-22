#pragma once

#include <memory>
#include <string_view>

#include <qchar.h>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "c_helpers.hpp"

namespace qs::wayland::input_method::impl {

/// A wrapper that handles xkb_keymap and xkb_state lifetime with some convenience functions
class KeyMapState {
public:
	KeyMapState() = default;
	KeyMapState(const char* string);
	KeyMapState(const KeyMapState& state);
	KeyMapState(KeyMapState&& state) noexcept;
	KeyMapState& operator=(const KeyMapState& state);
	KeyMapState& operator=(KeyMapState&& state) noexcept;
	~KeyMapState();

	// xkb expects us to free the returned string
	[[nodiscard]] uniqueCString keyMapAsString() const;

	[[nodiscard]] bool operator==(const KeyMapState& other) const;
	[[nodiscard]] operator bool() const;

	[[nodiscard]] const char* keyName(xkb_keycode_t key) const;
	[[nodiscard]] xkb_keycode_t maxKeycode() const;
	[[nodiscard]] xkb_keycode_t minKeycode() const;

	[[nodiscard]] xkb_keysym_t getOneSym(xkb_keycode_t key) const;
	[[nodiscard]] QChar getChar(xkb_keycode_t key) const;

	struct ModifierState {
		xkb_mod_mask_t depressed;
		xkb_mod_mask_t latched;
		xkb_mod_mask_t locked;
		xkb_layout_index_t depressedLayout;
		xkb_layout_index_t latchedLayout;
		xkb_layout_index_t lockedLayout;
	};
	// This is the server version
	// if we want to expand virtual keyboard support
	void setModifiers(ModifierState modifierState);
	[[nodiscard]] ModifierState serialiseMods() const;

	static std::string_view keyStateName(wl_keyboard_key_state state);

	[[nodiscard]] bool keyRepeats(xkb_keycode_t key) const;

private:
	static xkb_context* xkbContext;
	xkb_keymap* xkbKeymap = nullptr;
	xkb_state* xkbState = nullptr;
};

} // namespace qs::wayland::input_method::impl
