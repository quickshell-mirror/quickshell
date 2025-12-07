#pragma once

#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

#include "types.hpp"

namespace qs::wayland::input_method {

namespace impl {
class InputMethodManager;
class InputMethodHandle;
class InputMethodKeyboardGrab;
} // namespace impl

/// Provides keyboard handling logic for an @@InputMethod$'s grab protocol.
/// Use @@KeyboardTextEdit for a higher level and easier to use version.
class Keyboard: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("Input Method Keyboard must be retrieved from an Input Method object.");

public:
	explicit Keyboard(QObject* parent = nullptr);

	void setKeyboard(QPointer<impl::InputMethodKeyboardGrab> keyboard);

signals:
	void keyPress(QChar character);
	void returnPress();
	/// Note that internally Quickshell will release the keyboard when escape is pressed.
	void escapePress();
	void directionPress(QMLDirectionKey::Enum);
	void backspacePress();
	void deletePress();

private:
	QPointer<impl::InputMethodKeyboardGrab> mKeyboard;
};

/// Provides the ability to send text input to the compositor
///
/// A simple example that sends text to the currently focused input method:
/// ```
/// QSInputMethod {
///   id: input_method
/// }
/// IpcHandler {
///   target: "input_method"
///   function hi(): void { input_method.sendString("hi"); }
/// }
/// ```
/// We can now call the ipc handler and see that `hi` is printed to the terminal input.
/// `$ qs ipc call input_method hi`
class InputMethod: public QObject {
	Q_OBJECT;
	/// If a text input is focused
	Q_PROPERTY(bool active READ isActive NOTIFY activeChanged);
	/// Is false if another input method is already registered to the compositor, otherwise is true
	Q_PROPERTY(bool hasInput READ hasInput NOTIFY hasInputChanged);
	/// If the input method has grabbed the keyboard
	Q_PROPERTY(bool hasKeyboard READ hasKeyboard NOTIFY hasKeyboardChanged);
	/// Provides virtual text in the text input so the user can visualise what they write
	Q_PROPERTY(QString preeditString READ preeditString WRITE setPreeditString NOTIFY preeditStringChanged);
	/// If `cursorBegin == cursorEnd == -1` the text input will not show a cursor
	/// If `cursorBegin == cursorEnd == n` or `cursorBegin == n && cursorEnd == -1` the text input will show a cursor at n
	/// If `cursorBegin == n` and `cursorEnd == m` the text from n to m will be highlighted
	Q_PROPERTY(int32_t cursorBegin READ cursorBegin WRITE setCursorBegin NOTIFY cursorBeginChanged);
	Q_PROPERTY(int32_t cursorEnd READ cursorEnd WRITE setCursorEnd NOTIFY cursorEndChanged);
	/// The text around the where we will insert
	Q_PROPERTY(QString surroundingText READ surroundingText NOTIFY surroundingTextChanged);
	Q_PROPERTY(uint32_t surroundingTextCurosr READ surroundingTextCursor NOTIFY surroundingTextChanged);
	Q_PROPERTY(uint32_t surroundingTextAnchor READ surroundingTextAnchor NOTIFY surroundingTextChanged);
	/// The content_hint of the text input see https://wayland.app/protocols/text-input-unstable-v3#zwp_text_input_v3:enum:content_hint
	Q_PROPERTY(QMLContentHint::Enum contentHint READ contentHint NOTIFY contentHintChanged);
	/// The content_purpose of the text input see https://wayland.app/protocols/text-input-unstable-v3#zwp_text_input_v3:enum:content_purpose
	Q_PROPERTY(QMLContentPurpose::Enum contentPurpose READ contentPurpose NOTIFY contentPurposeChanged);
	/// Clear the preedit text when the keyboard grab is released
	Q_PROPERTY(
	    bool clearPreeditOnKeyboardRelease MEMBER mClearPreeditOnKeyboardRelease NOTIFY
	        clearPreeditOnKeyboardReleaseChanged
	);
	/// The @@Keyboard that will handle the grabbed keyboard.
	/// Use @@KeyboardTextEdit for most cases.
	Q_PROPERTY(QPointer<Keyboard> keyboard READ keyboard NOTIFY keyboardChanged);
	// Q_PROPERTY(QQmlComponent* surface READ surfaceComponent WRITE setSurfaceComponent NOTIFY surfaceComponentChanged);
	// Q_CLASSINFO("DefaultProperty", "surface");
	Q_CLASSINFO("DefaultProperty", "keyboard");
	QML_NAMED_ELEMENT(QSInputMethod);

public:
	explicit InputMethod(QObject* parent = nullptr);

	/// Sends a string to the text input currently focused. Will be updated after $$commit is called.
	Q_INVOKABLE void commitString(const QString& text);
	/// Removes text before the cursor by `before` and after by `after`.
	/// If preedit text is present, then text will be deleted before the preedit text and after the preedit text instead of the cursor.
	/// Will be updated after $$commit is called.
	Q_INVOKABLE void deleteSuroundingText(int before = 1, int after = 0);

	/// Apply state changes from commitString, setPreeditString and deleteSurroundingText requests.
	Q_INVOKABLE void commit();

	/// If there is a focused text input that we can write to
	[[nodiscard]] bool isActive() const;

	/// @@keyboard
	Q_INVOKABLE [[nodiscard]] QPointer<Keyboard> keyboard() const;

	/// @@hasInput
	[[nodiscard]] bool hasInput() const;
	/// Retries getting the input method.
	Q_INVOKABLE void getInput();
	/// Releases the input method so another program can use it.
	Q_INVOKABLE void releaseInput();

	/// @@hasKeyboard
	[[nodiscard]] bool hasKeyboard() const;
	/// Grabs the current keyboard so the input can be intercepted by the @@keyboard object
	Q_INVOKABLE void grabKeyboard();
	/// Releases the grabbed keyboard so it can be used normally.
	Q_INVOKABLE void releaseKeyboard();

	[[nodiscard]] const QString& preeditString() const;
	Q_INVOKABLE void setPreeditString(const QString& string);
	[[nodiscard]] int32_t cursorBegin() const;
	Q_INVOKABLE void setCursorBegin(int32_t position);
	[[nodiscard]] int32_t cursorEnd() const;
	Q_INVOKABLE void setCursorEnd(int32_t position);

	[[nodiscard]] const QString& surroundingText() const;
	[[nodiscard]] uint32_t surroundingTextCursor() const;
	[[nodiscard]] uint32_t surroundingTextAnchor() const;

	[[nodiscard]] QMLContentHint::Enum contentHint() const;
	[[nodiscard]] QMLContentPurpose::Enum contentPurpose() const;

signals:
	void activeChanged();
	void hasInputChanged();
	void hasKeyboardChanged();
	void preeditStringChanged();
	void cursorBeginChanged();
	void cursorEndChanged();
	void clearPreeditOnKeyboardReleaseChanged();
	void keyboardChanged();
	void contentHintChanged();
	void contentPurposeChanged();

	void surroundingTextChanged(QMLTextChangeCause::Enum textChangeCause);

protected:
	/// Sends the preedit string to compositor
	void setPreeditString();
	/// Make sure to call $$setPreeditString if you mutate
	[[nodiscard]] QString& preeditString();

private slots:
	void onHandleActiveChanged();

private:
	void handleKeyboardActive();

	QPointer<impl::InputMethodHandle> handle;
	Keyboard* mKeyboard = nullptr;

	bool mClearPreeditOnKeyboardRelease = true;

	QString mPreeditString;
	int32_t mCursorBegin = 0;
	int32_t mCursorEnd = 0;
};

} // namespace qs::wayland::input_method
