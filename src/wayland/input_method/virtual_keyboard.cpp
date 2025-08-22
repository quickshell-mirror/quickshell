#include "virtual_keyboard.hpp"
#include <chrono>
#include <cstddef>
#include <cstring>

#include <fcntl.h>
#include <qwayland-virtual-keyboard-unstable-v1.h>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>

#include "c_helpers.hpp"
#include "key_map_state.hpp"
#include "types.hpp"

namespace qs::wayland::input_method::impl {

namespace {
using namespace std::chrono;
int64_t now() {
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
} // namespace

VirtualKeyboardHandle::VirtualKeyboardHandle(
    ::zwp_virtual_keyboard_v1* keyboard,
    const KeyMapState& keymap
)
    : QtWayland::zwp_virtual_keyboard_v1(keyboard) {
	setKeymapState(keymap);
}

VirtualKeyboardHandle::~VirtualKeyboardHandle() { this->destroy(); }

void VirtualKeyboardHandle::setKeymapState(const KeyMapState& keymap) {
	if (this->mKeyMapState == keymap) return;
	if (!keymap) return;
	this->mKeyMapState = keymap;

	static const char* shmName = "/quickshellvirtualkeyboardformat";

	auto keymapString = keymap.keyMapAsString();
	const size_t size = strlen(keymapString.get()) + 1;
	auto shm = SharedMemory(shmName, O_CREAT | O_RDWR, size);
	shm.write(keymapString.get());

	if (shm) {
		this->zwp_virtual_keyboard_v1::keymap(WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, shm.get(), size);
		this->good = true;
	}
}

void VirtualKeyboardHandle::sendKey(xkb_keycode_t keycode, wl_keyboard_key_state state) {
	if (!this->good) return;
	if (keycode == XKB_KEYCODE_INVALID) return;
	this->zwp_virtual_keyboard_v1::key(now(), keycode - WAYLAND_KEY_OFFSET, state);
}

void VirtualKeyboardHandle::sendModifiers() {
	auto mods = this->mKeyMapState.serialiseMods();
	modifiers(mods.depressed, mods.latched, mods.locked, mods.depressedLayout);
}

bool VirtualKeyboardHandle::getGood() const { return good; }

}; // namespace qs::wayland::input_method::impl
