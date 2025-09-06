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

	bool first = true;

	if (contentHint & Completion) {
		if (!first) {
			string += " | ";
		}
		string += "Completion";
		first = false;
	}
	if (contentHint & Spellcheck) {
		if (!first) {
			string += " | ";
		}
		string += "Spellcheck";
		first = false;
	}
	if (contentHint & AutoCapitalization) {
		if (!first) {
			string += " | ";
		}
		string += "AutoCapitalization";
		first = false;
	}
	if (contentHint & Lowercase) {
		if (!first) {
			string += " | ";
		}
		string += "Lowercase";
		first = false;
	}
	if (contentHint & Uppercase) {
		if (!first) {
			string += " | ";
		}
		string += "Uppercase";
		first = false;
	}
	if (contentHint & Titlecase) {
		if (!first) {
			string += " | ";
		}
		string += "Titlecase";
		first = false;
	}
	if (contentHint & HiddenText) {
		if (!first) {
			string += " | ";
		}
		string += "HiddenText";
		first = false;
	}
	if (contentHint & SensitiveData) {
		if (!first) {
			string += " | ";
		}
		string += "SensitiveData";
		first = false;
	}
	if (contentHint & Latin) {
		if (!first) {
			string += " | ";
		}
		string += "Latin";
		first = false;
	}
	if (contentHint & Multiline) {
		if (!first) {
			string += " | ";
		}
		string += "Multiline";
		first = false;
	}

	if (string == "") string = "None";
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
