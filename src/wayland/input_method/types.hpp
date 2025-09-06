#pragma once

#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <xkbcommon/xkbcommon.h>
#include <wayland-text-input-unstable-v3-client-protocol.h>
#include <qobject.h>

namespace qs::wayland::input_method {

class QMLDirectionKey: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(DirectionKey);
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		Up, Down, Left, Right
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(Enum direction);
};

class QMLContentHint: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(ContentHint);
	QML_SINGLETON;

public:
	enum Enum : quint16 {
		None = ZWP_TEXT_INPUT_V3_CONTENT_HINT_NONE,
		Completion = ZWP_TEXT_INPUT_V3_CONTENT_HINT_COMPLETION,
		Spellcheck = ZWP_TEXT_INPUT_V3_CONTENT_HINT_SPELLCHECK,
		AutoCapitalization = ZWP_TEXT_INPUT_V3_CONTENT_HINT_AUTO_CAPITALIZATION,
		Lowercase = ZWP_TEXT_INPUT_V3_CONTENT_HINT_LOWERCASE,
		Uppercase = ZWP_TEXT_INPUT_V3_CONTENT_HINT_UPPERCASE,
		Titlecase = ZWP_TEXT_INPUT_V3_CONTENT_HINT_TITLECASE,
		HiddenText = ZWP_TEXT_INPUT_V3_CONTENT_HINT_HIDDEN_TEXT,
		SensitiveData = ZWP_TEXT_INPUT_V3_CONTENT_HINT_SENSITIVE_DATA,
		Latin = ZWP_TEXT_INPUT_V3_CONTENT_HINT_LATIN,
		Multiline = ZWP_TEXT_INPUT_V3_CONTENT_HINT_MULTILINE,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(Enum contentHint);
};

class QMLContentPurpose: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(ContentPurpose);
	QML_SINGLETON;

public:
	enum Enum : quint8{
		Normal = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NORMAL,
		Alpha = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_ALPHA,
		Digits = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DIGITS,
		Number = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NUMBER,
		Phone = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PHONE,
		Url = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_URL,
		Email = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_EMAIL,
		Name = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NAME,
		Password = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PASSWORD,
		Pin = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PIN,
		Data = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DATE,
		Time = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_TIME,
		Datetime = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DATETIME,
		Terminal = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_TERMINAL,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(Enum contentHint);
};

class QMLTextChangeCause: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(TextChangeCause);
	QML_SINGLETON;

public:
	enum Enum : bool {
		InputMethod = ZWP_TEXT_INPUT_V3_CHANGE_CAUSE_INPUT_METHOD,
		Other = ZWP_TEXT_INPUT_V3_CHANGE_CAUSE_OTHER,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(Enum textChangeCause);
};

namespace impl {
using DirectionKey = QMLDirectionKey::Enum;

using ContentHint = QMLContentHint::Enum;
using ContentPurpose = QMLContentPurpose::Enum;
using TextChangeCause = QMLTextChangeCause::Enum;

inline constexpr xkb_keycode_t WAYLAND_KEY_OFFSET = 8;
}
} // namespace qs::wayland::input_method::impl
