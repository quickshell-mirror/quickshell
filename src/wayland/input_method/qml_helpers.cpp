#include "qml_helpers.hpp"
#include <algorithm>

#include <qjsvalue.h>
#include <qobject.h>
#include <qqmlinfo.h>
#include <qtmetamacros.h>

#include "qml.hpp"
#include "types.hpp"

namespace qs::wayland::input_method {

KeyboardTextEdit::KeyboardTextEdit(QObject* parent): InputMethod(parent) {
	QObject::connect(
		this,
		&InputMethod::hasKeyboardChanged,
		this,
		&KeyboardTextEdit::onHasKeyboardChanged
	);

	QObject::connect(
		this,
		&InputMethod::surroundingTextChanged,
		this,
		&KeyboardTextEdit::onSurroundingTextChanged
	);
}

void KeyboardTextEdit::onHasKeyboardChanged() {
	if(!this->hasKeyboard()) {
		return;
	}
	QObject::connect(this->keyboard(), &Keyboard::keyPress, this, &KeyboardTextEdit::onKeyPress);
	QObject::connect(this->keyboard(), &Keyboard::returnPress, this, &KeyboardTextEdit::onReturnPress);
	QObject::connect(this->keyboard(), &Keyboard::directionPress, this, &KeyboardTextEdit::onDirectionPress);
	QObject::connect(this->keyboard(), &Keyboard::backspacePress, this, &KeyboardTextEdit::onBackspacePress);
	QObject::connect(this->keyboard(), &Keyboard::deletePress, this, &KeyboardTextEdit::onDeletePress);
}

QJSValue KeyboardTextEdit::transform() const { return this->mTransform; }

void KeyboardTextEdit::setTransform(const QJSValue& callback) {
	if (!callback.isCallable()) {
		qmlInfo(this) << "Transform must be a callable function";
		return;
	}
	this->mTransform = callback;
}

int KeyboardTextEdit::cursor() const { return this->cursorBegin(); }
void KeyboardTextEdit::setCursor(int value) {
	value = std::max(value, 0);
	value = std::min(value, static_cast<int>(this->preeditString().size()));
	if (this->cursorBegin() == value) return;
	this->setCursorBegin(value);
}

void KeyboardTextEdit::onKeyPress(QChar character) {
	this->preeditString().insert(this->cursorBegin(), character);
	this->setPreeditString();
	this->commit();
	emit this->preeditStringChanged();
	this->setCursor(this->cursorBegin() + 1);
}
void KeyboardTextEdit::onBackspacePress() {
	if (this->cursorBegin() == 0) return;
	this->preeditString().remove(this->cursorBegin() - 1, 1);
	this->setPreeditString();
	this->commit();
	emit this->preeditStringChanged();
	this->setCursor(this->cursorBegin() - 1);
}
void KeyboardTextEdit::onDeletePress() {
	if (this->cursorBegin() == this->preeditString().size()) return;
	this->preeditString().remove(this->cursorBegin(), 1);
	this->setPreeditString();
	this->commit();
	emit this->preeditStringChanged();
}

void KeyboardTextEdit::onDirectionPress(QMLDirectionKey::Enum direction) {
	switch (direction) {
	case QMLDirectionKey::Left: {
		this->setCursor(this->cursorBegin() - 1);
		return;
	}
	case QMLDirectionKey::Right: {
		this->setCursor(this->cursorBegin() + 1);
		return;
	}
	default: return;
	}
}

void KeyboardTextEdit::onReturnPress() {
	QString text = "";
	if (this->mTransform.isCallable())
		text = this->mTransform.call(QJSValueList({this->preeditString()})).toString();
	this->commitString(text);
	this->setPreeditString("");
	this->releaseKeyboard();
}

void KeyboardTextEdit::onSurroundingTextChanged(QMLTextChangeCause::Enum textChangeCause) {
	if (textChangeCause == QMLTextChangeCause::Other) {
		this->setPreeditString("");
	}
}

} // namespace qs::wayland::input_method
