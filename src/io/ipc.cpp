#include "ipc.hpp"
#include <utility>

#include <qcolor.h>
#include <qmetatype.h>
#include <qobjectdefs.h>

namespace qs::io::ipc {

const VoidIpcType VoidIpcType::INSTANCE {};
const StringIpcType StringIpcType::INSTANCE {};
const IntIpcType IntIpcType::INSTANCE {};
const BoolIpcType BoolIpcType::INSTANCE {};
const DoubleIpcType DoubleIpcType::INSTANCE {};
const ColorIpcType ColorIpcType::INSTANCE {};

const IpcType* IpcType::ipcType(const QMetaType& metaType) {
	if (metaType.id() == QMetaType::Void) return &VoidIpcType::INSTANCE;
	if (metaType.id() == QMetaType::QString) return &StringIpcType::INSTANCE;
	if (metaType.id() == QMetaType::Int) return &IntIpcType::INSTANCE;
	if (metaType.id() == QMetaType::Bool) return &BoolIpcType::INSTANCE;
	if (metaType.id() == QMetaType::Double) return &DoubleIpcType::INSTANCE;
	if (metaType.id() == QMetaType::QColor) return &ColorIpcType::INSTANCE;
	return nullptr;
}

IpcTypeSlot::IpcTypeSlot(IpcTypeSlot&& other) noexcept { *this = std::move(other); }

IpcTypeSlot& IpcTypeSlot::operator=(IpcTypeSlot&& other) noexcept {
	this->mType = other.mType;
	this->storage = other.storage;
	other.mType = nullptr;
	other.storage = nullptr;
	return *this;
}

IpcTypeSlot::~IpcTypeSlot() { this->replace(nullptr); }

const IpcType* IpcTypeSlot::type() const { return this->mType; }

void* IpcTypeSlot::get() {
	if (this->storage == nullptr) {
		this->storage = this->mType->createStorage();
	}

	return this->storage;
}

QGenericArgument IpcTypeSlot::asGenericArgument() {
	if (this->mType) {
		return QGenericArgument(this->mType->genericArgumentName(), this->get());
	} else {
		return QGenericArgument();
	}
}

QGenericReturnArgument IpcTypeSlot::asGenericReturnArgument() {
	if (this->mType) {
		return QGenericReturnArgument(this->mType->genericArgumentName(), this->get());
	} else {
		return QGenericReturnArgument();
	}
}

void IpcTypeSlot::replace(void* value) {
	if (this->storage != nullptr) {
		this->mType->destroyStorage(this->storage);
	}

	this->storage = value;
}

const char* VoidIpcType::name() const { return "void"; }
const char* VoidIpcType::genericArgumentName() const { return "void"; }

// string
const char* StringIpcType::name() const { return "string"; }
const char* StringIpcType::genericArgumentName() const { return "QString"; }
void* StringIpcType::fromString(const QString& string) const { return new QString(string); }
QString StringIpcType::toString(void* slot) const { return *static_cast<QString*>(slot); }
void* StringIpcType::createStorage() const { return new QString(); }
void StringIpcType::destroyStorage(void* slot) const { delete static_cast<QString*>(slot); }

// int
const char* IntIpcType::name() const { return "int"; }
const char* IntIpcType::genericArgumentName() const { return "int"; }

void* IntIpcType::fromString(const QString& string) const {
	auto ok = false;
	auto v = string.toInt(&ok);

	return ok ? new int(v) : nullptr;
}

QString IntIpcType::toString(void* slot) const { return QString::number(*static_cast<int*>(slot)); }

void* IntIpcType::createStorage() const { return new int(); }
void IntIpcType::destroyStorage(void* slot) const { delete static_cast<int*>(slot); }

// bool
const char* BoolIpcType::name() const { return "bool"; }
const char* BoolIpcType::genericArgumentName() const { return "bool"; }

void* BoolIpcType::fromString(const QString& string) const {
	if (string == "true") return new bool(true);
	if (string == "false") return new bool(false);

	auto isInt = false;
	auto iv = string.toInt(&isInt);

	return isInt ? new bool(iv != 0) : nullptr;
}

QString BoolIpcType::toString(void* slot) const {
	return *static_cast<bool*>(slot) ? "true" : "false";
}

void* BoolIpcType::createStorage() const { return new bool(); }
void BoolIpcType::destroyStorage(void* slot) const { delete static_cast<bool*>(slot); }

// double
const char* DoubleIpcType::name() const { return "real"; }
const char* DoubleIpcType::genericArgumentName() const { return "double"; }

void* DoubleIpcType::fromString(const QString& string) const {
	auto ok = false;
	auto v = string.toDouble(&ok);

	return ok ? new double(v) : nullptr;
}

QString DoubleIpcType::toString(void* slot) const {
	return QString::number(*static_cast<double*>(slot));
}

void* DoubleIpcType::createStorage() const { return new double(); }
void DoubleIpcType::destroyStorage(void* slot) const { delete static_cast<double*>(slot); }

// color
const char* ColorIpcType::name() const { return "color"; }
const char* ColorIpcType::genericArgumentName() const { return "QColor"; }

void* ColorIpcType::fromString(const QString& string) const {
	auto color = QColor::fromString(string);

	if (!color.isValid() && !string.startsWith('#')) {
		color = QColor::fromString('#' % string);
	}

	return color.isValid() ? new QColor(color) : nullptr;
}

QString ColorIpcType::toString(void* slot) const {
	return static_cast<QColor*>(slot)->name(QColor::HexArgb);
}

void* ColorIpcType::createStorage() const { return new bool(); }
void ColorIpcType::destroyStorage(void* slot) const { delete static_cast<bool*>(slot); }

QString WireFunctionDefinition::toString() const {
	QString paramString;
	for (const auto& [name, type]: this->arguments) {
		if (!paramString.isEmpty()) paramString += ", ";
		paramString += name % ": " % type;
	}

	return "function " % this->name % '(' % paramString % "): " % this->returnType;
}

QString WireTargetDefinition::toString() const {
	QString accum = "target " % this->name;

	for (const auto& func: this->functions) {
		accum += "\n  " % func.toString();
	}

	return accum;
}

} // namespace qs::io::ipc
