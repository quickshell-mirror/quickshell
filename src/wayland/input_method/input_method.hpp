#pragma once

#include <qobject.h>
#include <qpointer.h>
#include <qtclasshelpermacros.h>
#include <qwayland-input-method-unstable-v2.h>

namespace qs::wayland::input_method::impl {

class InputMethodKeyboardGrab;

class InputMethodHandle
    : public QObject
    , private QtWayland::zwp_input_method_v2 {
	Q_OBJECT;

public:
	explicit InputMethodHandle(QObject* parent, ::zwp_input_method_v2* inputMethod);
	~InputMethodHandle() override;
	Q_DISABLE_COPY_MOVE(InputMethodHandle);

	void commitString(const QString& text);
	// By default hides the cursor
	void sendPreeditString(const QString& text, int32_t cursorBegin = -1, int32_t cursorEnd = -1);
	void deleteText(int before, int after);
	void commit();

	[[nodiscard]] bool hasKeyboard() const;
	QPointer<InputMethodKeyboardGrab> grabKeyboard();
	void releaseKeyboard();

	[[nodiscard]] bool isActive() const;
	[[nodiscard]] bool isAvailable() const;

signals:
	void activated();
	void deactivated();

private:
	void zwp_input_method_v2_activate() override;
	void zwp_input_method_v2_deactivate() override;
	void zwp_input_method_v2_done() override;
	void zwp_input_method_v2_unavailable() override;

	bool mActivated = false;
	bool mNewActive = false;
	bool mAvailable = true;
	int serial = 0;

	InputMethodKeyboardGrab* keyboard = nullptr;
};

} // namespace qs::wayland::input_method::impl
