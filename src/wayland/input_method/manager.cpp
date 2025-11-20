#include "manager.hpp"
#include <memory>

#include <qguiapplication_platform.h>
#include <qpointer.h>
#include <qwaylandclientextension.h>

#include "input_method.hpp"
#include "virtual_keyboard.hpp"

namespace qs::wayland::input_method::impl {

InputMethodManager::InputMethodManager(): QWaylandClientExtensionTemplate(1) { this->initialize(); }

InputMethodManager::~InputMethodManager() { this->destroy(); }

InputMethodManager* InputMethodManager::instance() {
	// The OS should free this memory when we exit
	static auto* instance = new InputMethodManager();
	return instance;
}

namespace {
wl_seat* getSeat() {
	return dynamic_cast<QGuiApplication*>(QGuiApplication::instance())
	    ->nativeInterface<QNativeInterface::QWaylandApplication>()
	    ->seat();
}
} // namespace

QPointer<InputMethodHandle> InputMethodManager::acquireInput() {
	if (this->inputMethod && this->inputMethod->isAvailable()) return this->inputMethod;

	this->inputMethod = new InputMethodHandle(this, this->get_input_method(getSeat()));

	return this->inputMethod;
}

void InputMethodManager::releaseInput() {
	this->inputMethod->deleteLater();
	this->inputMethod = nullptr;
}

VirtualKeyboardManager::VirtualKeyboardManager(): QWaylandClientExtensionTemplate(1) {
	this->initialize();
}

VirtualKeyboardManager* VirtualKeyboardManager::instance() {
	// The OS should free this memory when we exit
	static auto* instance = new VirtualKeyboardManager();
	return instance;
}

std::unique_ptr<VirtualKeyboardHandle>
VirtualKeyboardManager::createVirtualKeyboard(const KeyMapState& keymap) {
	return std::make_unique<VirtualKeyboardHandle>(this->create_virtual_keyboard(getSeat()), keymap);
}

} // namespace qs::wayland::input_method::impl
