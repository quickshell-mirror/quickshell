#include "types.hpp"

#include "qstring.h"

namespace qs::wayland::input_method {

QString QMLDirectionKey::toString(Enum direction) {
	switch (direction) {
	case Up: return "Up";
	case Down: return "Down";
	case Left: return "Left";
	case Right: return "Right";
	}
	return "UNKNOWN";
}

QString QMLContentHint::toString(Enum contentHint) {
	QString string = "";

	if (contentHint & Completion) {
		string += "Completion | ";
	}
	if (contentHint & Spellcheck) {
		string += "Spellcheck | ";
	}
	if (contentHint & AutoCapitalization) {
		string += "AutoCapitalization | ";
	}
	if (contentHint & Lowercase) {
		string += "Lowercase | ";
	}
	if (contentHint & Uppercase) {
		string += "Uppercase | ";
	}
	if (contentHint & Titlecase) {
		string += "Titlecase | ";
	}
	if (contentHint & HiddenText) {
		string += "HiddenText | ";
	}
	if (contentHint & SensitiveData) {
		string += "SensitiveData | ";
	}
	if (contentHint & Latin) {
		string += "Latin | ";
	}
	if (contentHint & Multiline) {
		string += "Multiline | ";
	}

	if (string == "") {
		string = "None";
	} else {
		// Erase the " | " from end of string
		string.resize(string.size() - 3);
	}
	return string;
}

QString QMLContentPurpose::toString(Enum contentHint) {
	switch (contentHint) {
	case Normal: return "Normal";
	case Alpha: return "Alpha";
	case Digits: return "Digits";
	case Number: return "Number";
	case Phone: return "Phone";
	case Url: return "Url";
	case Email: return "Email";
	case Name: return "Name";
	case Password: return "Password";
	case Pin: return "Pin";
	case Data: return "Data";
	case Time: return "Time";
	case Datetime: return "Datetime";
	case Terminal: return "Terminal";
	}
	return "Unknown";
}

QString QMLTextChangeCause::toString(Enum textChangeCause) {
	switch (textChangeCause) {
	case InputMethod: return "InputMethod";
	case Other: return "Other";
	}
	return "Unknown";
}

} // namespace qs::wayland::input_method
