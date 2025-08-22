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
		UP, DOWN, LEFT, RIGHT
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
		NONE = ZWP_TEXT_INPUT_V3_CONTENT_HINT_NONE,
		COMPLETION = ZWP_TEXT_INPUT_V3_CONTENT_HINT_COMPLETION,
		SPELLCHECK = ZWP_TEXT_INPUT_V3_CONTENT_HINT_SPELLCHECK,
		AUTO_CAPITALIZATION = ZWP_TEXT_INPUT_V3_CONTENT_HINT_AUTO_CAPITALIZATION,
		LOWERCASE = ZWP_TEXT_INPUT_V3_CONTENT_HINT_LOWERCASE,
		UPPERCASE = ZWP_TEXT_INPUT_V3_CONTENT_HINT_UPPERCASE,
		TITLECASE = ZWP_TEXT_INPUT_V3_CONTENT_HINT_TITLECASE,
		HIDDEN_TEXT = ZWP_TEXT_INPUT_V3_CONTENT_HINT_HIDDEN_TEXT,
		SENSITIVE_DATA = ZWP_TEXT_INPUT_V3_CONTENT_HINT_SENSITIVE_DATA,
		LATIN = ZWP_TEXT_INPUT_V3_CONTENT_HINT_LATIN,
		MULTILINE = ZWP_TEXT_INPUT_V3_CONTENT_HINT_MULTILINE,
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
		NORMAL = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NORMAL,
		ALPHA = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_ALPHA,
		DIGITS = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DIGITS,
		NUMBER = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NUMBER,
		PHONE = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PHONE,
		URL = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_URL,
		EMAIL = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_EMAIL,
		NAME = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NAME,
		PASSWORD = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PASSWORD,
		PIN = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PIN,
		DATA = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DATE,
		TIME = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_TIME,
		DATETIME = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DATETIME,
		TERMINAL = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_TERMINAL,
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
		INPUT_METHOD = ZWP_TEXT_INPUT_V3_CHANGE_CAUSE_INPUT_METHOD,
		OTHER = ZWP_TEXT_INPUT_V3_CHANGE_CAUSE_OTHER,
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
