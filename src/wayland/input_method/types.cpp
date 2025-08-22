#include "types.hpp"

#include "qstring.h"

namespace qs::wayland::input_method {

QString QMLDirectionKey::toString(Enum direction) {
	switch (direction) {
	case UP: return "UP";
	case DOWN: return "DOWN";
	case LEFT: return "LEFT";
	case RIGHT: return "RIGHT";
	}
	return "UNKNOWN";
}

QString QMLContentHint::toString(Enum contentHint) {
	QString string = "";

	bool first = true;

	if (contentHint & COMPLETION) {
		if (!first) {
			string += " | ";
		}
		string += "COMPLETION";
		first = false;
	}
	if (contentHint & SPELLCHECK) {
		if (!first) {
			string += " | ";
		}
		string += "SPELLCHECK";
		first = false;
	}
	if (contentHint & AUTO_CAPITALIZATION) {
		if (!first) {
			string += " | ";
		}
		string += "AUTO_CAPITALIZATION";
		first = false;
	}
	if (contentHint & LOWERCASE) {
		if (!first) {
			string += " | ";
		}
		string += "LOWERCASE";
		first = false;
	}
	if (contentHint & UPPERCASE) {
		if (!first) {
			string += " | ";
		}
		string += "UPPERCASE";
		first = false;
	}
	if (contentHint & TITLECASE) {
		if (!first) {
			string += " | ";
		}
		string += "TITLECASE";
		first = false;
	}
	if (contentHint & HIDDEN_TEXT) {
		if (!first) {
			string += " | ";
		}
		string += "HIDDEN_TEXT";
		first = false;
	}
	if (contentHint & SENSITIVE_DATA) {
		if (!first) {
			string += " | ";
		}
		string += "SENSITIVE_DATA";
		first = false;
	}
	if (contentHint & LATIN) {
		if (!first) {
			string += " | ";
		}
		string += "LATIN";
		first = false;
	}
	if (contentHint & MULTILINE) {
		if (!first) {
			string += " | ";
		}
		string += "MULTILINE";
		first = false;
	}

	if (string == "") string = "NONE";
	return string;
}

QString QMLContentPurpose::toString(Enum contentHint) {
	switch (contentHint) {
	case NORMAL: return "NORMAL";
	case ALPHA: return "ALPHA";
	case DIGITS: return "DIGITS";
	case NUMBER: return "NUMBER";
	case PHONE: return "PHONE";
	case URL: return "URL";
	case EMAIL: return "EMAIL";
	case NAME: return "NAME";
	case PASSWORD: return "PASSWORD";
	case PIN: return "PIN";
	case DATA: return "DATA";
	case TIME: return "TIME";
	case DATETIME: return "DATETIME";
	case TERMINAL: return "TERMINAL";
	}
	return "UNKNOWN";
}

QString QMLTextChangeCause::toString(Enum textChangeCause) {
	switch (textChangeCause) {
	case INPUT_METHOD: return "INPUT_METHOD";
	case OTHER: return "OTHER";
	}
	return "UNKNOWN";
}

} // namespace qs::wayland::input_method
