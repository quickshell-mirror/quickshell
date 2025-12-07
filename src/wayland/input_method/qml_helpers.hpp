#pragma once

#include <qqmlintegration.h>
#include <qstring.h>
#include <qtmetamacros.h>

#include "qml.hpp"

namespace qs::wayland::input_method {

/// A keyboard handler for @@InputMethod$'s keyboard grab protocol.
///
/// As text is typed on the keyboard this will be displayed in the input method as virtual/preedit text.
/// A cursor enables some control over the composition of the text.
/// When return is pressed the transform function is called and the returned string is sent to the input text
/// and the keyboard is released.
///
/// ```
///  KeyboardTextEdit {
///   id: keyboard_text_edit
///   transform: function (text: string): string {
///     return {
///         "cool": "hi"
///         // etc.
//      }[text];
///   }
/// }
/// IpcHandler {
///   target: "emoji"
///   function get(): void { keyboard_text_edit.grabKeyboard(); }
/// }
/// ```
///
/// If you need a lower level view of the key events use @@Keyboard$.
class KeyboardTextEdit: public InputMethod {
	Q_OBJECT;
	/// A function that has a string parameter and returns a string.
	Q_PROPERTY(QJSValue transform READ transform WRITE setTransform NOTIFY transformChanged FINAL)
	QML_ELEMENT;

public:
	KeyboardTextEdit(QObject* parent = nullptr);

	Q_INVOKABLE [[nodiscard]] QJSValue transform() const;
	Q_INVOKABLE void setTransform(const QJSValue& callback);

	Q_INVOKABLE [[nodiscard]] int cursor() const;
	Q_INVOKABLE void setCursor(int value);

signals:
	void transformChanged();

private slots:
	void onHasKeyboardChanged();
	void onKeyPress(QChar character);
	void onReturnPress();
	void onDirectionPress(QMLDirectionKey::Enum direction);
	void onBackspacePress();
	void onDeletePress();
	void onSurroundingTextChanged(QMLTextChangeCause::Enum textChangeCause);

private:
	QJSValue mTransform;
};

} // namespace qs::wayland::input_method
