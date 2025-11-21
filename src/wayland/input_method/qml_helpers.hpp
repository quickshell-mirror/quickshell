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
/// QSInputMethod {
///   id: input_method
///   KeyboardTextEdit {
///     transform: function (text: string): string {
///       return {
///           "cool": "hi"
///           // etc.
///       }[text];
///     }
///   }
/// }
/// IpcHandler {
///   target: "emoji"
///   function get(): void { input_method.grabKeyboard(); }
/// }
/// ```
///
/// If you need a lower level view of the key events use @@Keyboard$.
class KeyboardTextEdit: public Keyboard {
	Q_OBJECT;
	/// A function that has a string parameter and returns a string.
	Q_PROPERTY(QJSValue transform READ transform WRITE setTransform NOTIFY transformChanged FINAL)
	/// The position of the cursor within the @@editText$.
	Q_PROPERTY(int cursor MEMBER mCursor READ cursor WRITE setCursor NOTIFY cursorChanged);
	/// The text that is currently being edited and displayed.
	Q_PROPERTY(
	    QString editText MEMBER mEditText READ editText WRITE setEditText NOTIFY editTextChanged
	);
	QML_ELEMENT;

public:
	KeyboardTextEdit(QObject* parent = nullptr);

	Q_INVOKABLE [[nodiscard]] QJSValue transform() const;
	Q_INVOKABLE void setTransform(const QJSValue& callback);

	Q_INVOKABLE [[nodiscard]] int cursor() const;
	Q_INVOKABLE void setCursor(int value);

	Q_INVOKABLE [[nodiscard]] QString editText() const;
	Q_INVOKABLE void setEditText(const QString& value);

signals:
	void transformChanged();
	void cursorChanged();
	void editTextChanged();

private slots:
	void onKeyPress(QChar character);
	void onReturnPress();
	void onDirectionPress(QMLDirectionKey::Enum direction);
	void onBackspacePress();
	void onDeletePress();
	void onSurroundingTextChanged(QMLTextChangeCause::Enum textChangeCause);

private:
	void updatePreedit();

	QJSValue mTransform;
	int mCursor = 0;
	QString mEditText = "";
};

} // namespace qs::wayland::input_method
