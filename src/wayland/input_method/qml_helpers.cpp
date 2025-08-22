#include "qml_helpers.hpp"

#include <qjsvalue.h>
#include <qobject.h>
#include <qqmlinfo.h>

#include "qml.hpp"

namespace qs::wayland::input_method {

KeyboardTextEdit::KeyboardTextEdit(QObject* parent): Keyboard(parent) {
	QObject::connect(this, &Keyboard::keyPress, this, &KeyboardTextEdit::onKeyPress);
	QObject::connect(this, &Keyboard::returnPress, this, &KeyboardTextEdit::onReturnPress);
	QObject::connect(this, &Keyboard::directionPress, this, &KeyboardTextEdit::onDirectionPress);
	QObject::connect(this, &Keyboard::backspacePress, this, &KeyboardTextEdit::onBackspacePress);
	QObject::connect(this, &Keyboard::deletePress, this, &KeyboardTextEdit::onDeletePress);

	auto* inputMethod = dynamic_cast<InputMethod*>(parent);
	if (inputMethod)
		QObject::connect(inputMethod, &InputMethod::surroundingTextChanged, this, &KeyboardTextEdit::onSurroundingTextChanged);
}

QJSValue KeyboardTextEdit::transform() const { return this->mTransform; }

void KeyboardTextEdit::setTransform(const QJSValue& callback) {
	if (!callback.isCallable()) {
		qmlInfo(this) << "Transform must be a callable function";
		return;
	}
	this->mTransform = callback;
}

int KeyboardTextEdit::cursor() const { return this->mCursor; }
void KeyboardTextEdit::setCursor(int value) {
	value = std::max(value, 0);
	value = std::min(value, static_cast<int>(this->mEditText.size()));
	if (this->mCursor == value) return;
	this->mCursor = value;
	this->updatePreedit();
	emit cursorChanged();
}

QString KeyboardTextEdit::editText() const { return this->mEditText; }
void KeyboardTextEdit::setEditText(const QString& value) {
	if (this->mEditText == value) return;
	this->mEditText = value;
	this->updatePreedit();
	emit editTextChanged();
}

void KeyboardTextEdit::updatePreedit() {
	auto* inputMethod = dynamic_cast<InputMethod*>(parent());
	if (!inputMethod) {
		return;
	}
	inputMethod->sendPreeditString(this->mEditText, this->mCursor, this->mCursor);
}

void KeyboardTextEdit::onKeyPress(QChar character) {
	this->mEditText.insert(this->mCursor, character);
	this->updatePreedit();
	emit editTextChanged();
	this->setCursor(this->mCursor + 1);
}
void KeyboardTextEdit::onBackspacePress() {
	if (this->mCursor == 0) return;
	this->mEditText.remove(this->mCursor - 1, 1);
	this->updatePreedit();
	emit editTextChanged();
	this->setCursor(this->mCursor - 1);
}
void KeyboardTextEdit::onDeletePress() {
	if (this->mCursor == this->mEditText.size()) return;
	this->mEditText.remove(this->mCursor, 1);
	this->updatePreedit();
	emit editTextChanged();
}

void KeyboardTextEdit::onDirectionPress(QMLDirectionKey::Enum direction) {
	switch (direction) {
	case QMLDirectionKey::LEFT: {
		this->setCursor(this->mCursor - 1);
		return;
	}
	case QMLDirectionKey::RIGHT: {
		this->setCursor(this->mCursor + 1);
		return;
	}
	default: return;
	}
}

void KeyboardTextEdit::onReturnPress() {
	auto* inputMethod = dynamic_cast<InputMethod*>(parent());
	if (!inputMethod) return;
	QString text = "";
	if (this->mTransform.isCallable())
		text = this->mTransform.call(QJSValueList({this->mEditText})).toString();
	inputMethod->sendString(text);
	this->setEditText("");
	inputMethod->releaseKeyboard();
}

void KeyboardTextEdit::onSurroundingTextChanged(QMLTextChangeCause::Enum textChangeCause) {
	if (textChangeCause == QMLTextChangeCause::OTHER) {
		this->setEditText("");
	}
}

} // namespace qs::wayland::input_method
