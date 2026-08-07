#include "ipc.hpp"
#include <utility>

#include <qcolor.h>
#include <qmetatype.h>
#include <qobjectdefs.h>
#include <qvariant.h>

namespace qs::io::ipc {

bool IpcValueSlot::isSupported(const QMetaType& type) { return IpcValueSlot::typeName(type); }

const char* IpcValueSlot::typeName(const QMetaType& type) {
	switch (type.id()) {
	case QMetaType::Void: return "void";
	case QMetaType::QString: return "string";
	case QMetaType::Int: return "int";
	case QMetaType::Bool: return "bool";
	case QMetaType::Double: return "real";
	case QMetaType::QColor: return "color";
	default: return nullptr;
	}
}

const char* IpcValueSlot::name() const {
	return this->mValue.isValid() ? IpcValueSlot::typeName(this->mValue.metaType()) : "void";
}

bool IpcValueSlot::isVoid() const { return !this->mValue.isValid(); }

const QVariant& IpcValueSlot::value() const { return this->mValue; }

bool IpcValueSlot::setString(const QString& string) {
	switch (this->mValue.typeId()) {
	case QMetaType::QString: this->mValue = QVariant::fromValue(string); return true;
	case QMetaType::Int: {
		auto ok = false;
		auto value = string.toInt(&ok);
		if (ok) this->mValue = QVariant::fromValue(value);
		return ok;
	}
	case QMetaType::Bool: {
		if (string == "true" || string == "false") {
			this->mValue = QVariant::fromValue(string == "true");
			return true;
		}

		auto ok = false;
		auto value = string.toInt(&ok);
		if (ok) this->mValue = QVariant::fromValue(value != 0);
		return ok;
	}
	case QMetaType::Double: {
		auto ok = false;
		auto value = string.toDouble(&ok);
		if (ok) this->mValue = QVariant::fromValue(value);
		return ok;
	}
	case QMetaType::QColor: {
		auto value = QColor::fromString(string);
		if (!value.isValid() && !string.startsWith('#')) value = QColor::fromString('#' % string);
		if (value.isValid()) this->mValue = QVariant::fromValue(value);
		return value.isValid();
	}
	default: return false;
	}
}

void IpcValueSlot::setValue(QVariant value) { this->mValue = std::move(value); }

QString IpcValueSlot::toString() const {
	switch (this->mValue.typeId()) {
	case QMetaType::UnknownType: return "";
	case QMetaType::QString: return this->mValue.value<QString>();
	case QMetaType::Int: return QString::number(this->mValue.value<int>());
	case QMetaType::Bool: return this->mValue.value<bool>() ? "true" : "false";
	case QMetaType::Double: return QString::number(this->mValue.value<double>());
	case QMetaType::QColor: return this->mValue.value<QColor>().name(QColor::HexArgb);
	default: return "";
	}
}

QGenericArgument IpcValueSlot::asGenericArgument() const {
	return QGenericArgument(this->mValue.metaType().name(), this->mValue.constData());
}

QGenericReturnArgument IpcValueSlot::asGenericReturnArgument() {
	if (!this->mValue.isValid()) return QGenericReturnArgument();
	return QGenericReturnArgument(this->mValue.metaType().name(), this->mValue.data());
}

QString WireFunctionDefinition::toString() const {
	QString paramString;
	for (const auto& [name, type]: this->arguments) {
		if (!paramString.isEmpty()) paramString += ", ";
		paramString += name % ": " % type;
	}

	return "function " % this->name % '(' % paramString % "): " % this->returnType;
}

QString WirePropertyDefinition::toString() const {
	return "property " % this->name % ": " % this->type;
}

QString WireSignalDefinition::toString() const {
	if (this->rettype.isEmpty()) {
		return "signal " % this->name % "()";
	} else {
		return "signal " % this->name % "(" % this->retname % ": " % this->rettype % ')';
	}
}

QString WireTargetDefinition::toString() const {
	QString accum = "target " % this->name;

	for (const auto& func: this->functions) {
		accum += "\n  " % func.toString();
	}

	for (const auto& prop: this->properties) {
		accum += "\n  " % prop.toString();
	}

	for (const auto& sig: this->signalFunctions) {
		accum += "\n  " % sig.toString();
	}

	return accum;
}

} // namespace qs::io::ipc
