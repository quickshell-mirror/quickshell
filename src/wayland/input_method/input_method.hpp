#pragma once

#include <qobject.h>
#include <qpointer.h>
#include <qtclasshelpermacros.h>
#include <qwayland-input-method-unstable-v2.h>

#include "types.hpp"

namespace qs::wayland::input_method::impl {

class InputMethodKeyboardGrab;

/// A lightweight handle for the wayland input_method protocol
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

	[[nodiscard]] const QString& surroundingText() const;
	[[nodiscard]] uint32_t surroundingTextCursor() const;
	[[nodiscard]] uint32_t surroundingTextAnchor() const;

	[[nodiscard]] ContentHint contentHint() const;
	[[nodiscard]] ContentPurpose contentPurpose() const;

signals:
	void activated();
	void deactivated();

	void surroundingTextChanged(TextChangeCause textChangeCause);

	void contentHintChanged();
	void contentPurposeChanged();

private:
	void zwp_input_method_v2_activate() override;
	void zwp_input_method_v2_deactivate() override;
	void zwp_input_method_v2_surrounding_text(const QString& text, uint32_t cursor, uint32_t anchor) override;
	void zwp_input_method_v2_text_change_cause(uint32_t cause) override;
	void zwp_input_method_v2_content_type(uint32_t hint, uint32_t purpose) override;
	void zwp_input_method_v2_done() override;
	void zwp_input_method_v2_unavailable() override;

	bool mAvailable = true;
	int serial = 0;

	struct State {
		bool activated = false;

		struct SurroundingText {
			QString text = "";
			uint32_t cursor = 0;
			uint32_t anchor = 0;
			TextChangeCause textChangeCause = TextChangeCause::INPUT_METHOD;
			bool operator==(const SurroundingText& other) const = default;
		} surroundingText;

		ContentHint contentHint = ContentHint::NONE;
		ContentPurpose contentPurpose = ContentPurpose::NORMAL;
	};

	State mState;
	State mNewState;

	InputMethodKeyboardGrab* keyboard = nullptr;
};

} // namespace qs::wayland::input_method::impl
