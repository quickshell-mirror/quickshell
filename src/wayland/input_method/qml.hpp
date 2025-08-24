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

class KeyboardDirectionKey: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(DirectionKey);
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		UP = static_cast<quint8>(impl::DirectionKey::UP),
		DOWN = static_cast<quint8>(impl::DirectionKey::DOWN),
		LEFT = static_cast<quint8>(impl::DirectionKey::LEFT),
		RIGHT = static_cast<quint8>(impl::DirectionKey::RIGHT),
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(Enum direction);
	static Enum fromDirection(impl::DirectionKey direction);
};

class Keyboard: public QObject {
	Q_OBJECT;
	/// The surface that will be created for the keyboard. Must create a @@KeyboardSurface$.
	// Q_PROPERTY(QQmlComponent* surface READ surfaceComponent WRITE setSurfaceComponent NOTIFY surfaceComponentChanged);
	// Q_CLASSINFO("DefaultProperty", "surface");
	QML_ELEMENT;

public:
	explicit Keyboard(QObject* parent = nullptr);

	void setKeyboard(QPointer<impl::InputMethodKeyboardGrab> keyboard);

signals:
	void keyPress(QChar character);
	void returnPress();
	void escapePress();
	void directionPress(KeyboardDirectionKey::Enum);
	void backspacePress();
	void deletePress();

private slots:
	void onDirectionPress(impl::DirectionKey direction);

private:
	QPointer<impl::InputMethodKeyboardGrab> mKeyboard = nullptr;
	// QQmlComponent* mSurfaceComponent = nullptr;
};

class InputMethod: public QObject {
	Q_OBJECT;
	Q_PROPERTY(bool active READ isActive NOTIFY activeChanged);
	Q_PROPERTY(bool hasInput READ hasInput NOTIFY hasInputChanged);
	Q_PROPERTY(bool hasKeyboard READ hasKeyboard NOTIFY hasKeyboardChanged);
	Q_PROPERTY(
	    bool clearPreeditOnKeyboardRelease MEMBER mClearPreeditOnKeyboardRelease NOTIFY
	        clearPreeditOnKeyboardReleaseChanged
	);
	// Q_PROPERTY(QString preedit)
	Q_PROPERTY(
	    QQmlComponent* keyboard READ keyboardComponent WRITE setKeyboardComponent NOTIFY
	        keyboardComponentChanged
	);
	Q_CLASSINFO("DefaultProperty", "keyboard");
	QML_NAMED_ELEMENT(QSInputMethod);

public:
	explicit InputMethod(QObject* parent = nullptr);

	Q_INVOKABLE void sendString(const QString& text);
	Q_INVOKABLE void
	sendPreeditString(const QString& text, int32_t cursorBegin = -1, int32_t cursorEnd = -1);
	Q_INVOKABLE void deleteText(int before = 1, int after = 0);

	Q_INVOKABLE [[nodiscard]] bool isActive() const;

	Q_INVOKABLE [[nodiscard]] QQmlComponent* keyboardComponent() const;
	Q_INVOKABLE void setKeyboardComponent(QQmlComponent* keyboardComponent);

	Q_INVOKABLE [[nodiscard]] bool hasInput() const;
	Q_INVOKABLE void getInput();
	Q_INVOKABLE void releaseInput();

	Q_INVOKABLE [[nodiscard]] bool hasKeyboard() const;
	Q_INVOKABLE void grabKeyboard();
	Q_INVOKABLE void releaseKeyboard();

signals:
	void activeChanged();
	void hasInputChanged();
	void hasKeyboardChanged();
	void clearPreeditOnKeyboardReleaseChanged();
	void keyboardComponentChanged();

private slots:
	void onHandleActiveChanged();

private:
	void handleKeyboardActive();

	QPointer<impl::InputMethodHandle> handle;
	Keyboard* keyboard = nullptr;
	QQmlComponent* mKeyboardComponent = nullptr;

	bool mClearPreeditOnKeyboardRelease = true;
};

} // namespace qs::wayland::input_method
