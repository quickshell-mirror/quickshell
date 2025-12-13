#pragma once

#include <memory.h>
#include <qpointer.h>
#include <qtclasshelpermacros.h>
#include <qwayland-input-method-unstable-v2.h>
#include <qwayland-virtual-keyboard-unstable-v1.h>
#include <qwaylandclientextension.h>

namespace qs::wayland::input_method::impl {

class InputMethodHandle;
class KeyMapState;

class InputMethodManager
    : public QWaylandClientExtensionTemplate<InputMethodManager>
    , private QtWayland::zwp_input_method_manager_v2 {
	friend class QWaylandClientExtensionTemplate<InputMethodManager>;

public:
	~InputMethodManager() override;
	Q_DISABLE_COPY_MOVE(InputMethodManager);

	static InputMethodManager* instance();

	QPointer<InputMethodHandle> acquireInput();
	void releaseInput();

private:
	InputMethodManager();
	InputMethodHandle* inputMethod = nullptr;
};

class VirtualKeyboardHandle;

class VirtualKeyboardManager
    : public QWaylandClientExtensionTemplate<VirtualKeyboardManager>
    , private QtWayland::zwp_virtual_keyboard_manager_v1 {
	friend class QWaylandClientExtensionTemplate<VirtualKeyboardManager>;

public:
	~VirtualKeyboardManager() override = default;
	Q_DISABLE_COPY_MOVE(VirtualKeyboardManager);

	static VirtualKeyboardManager* instance();

	std::unique_ptr<VirtualKeyboardHandle> createVirtualKeyboard(const KeyMapState& keymap);

private:
	VirtualKeyboardManager();
};

} // namespace qs::wayland::input_method::impl
