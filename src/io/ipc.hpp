#pragma once

#include <qcontainerfwd.h>
#include <qmetatype.h>
#include <qobjectdefs.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../ipc/ipc.hpp"

namespace qs::io::ipc {

class IpcValueSlot {
public:
	IpcValueSlot() = default;
	explicit IpcValueSlot(QMetaType type): mValue(type) {}

	[[nodiscard]] static bool isSupported(const QMetaType& type);
	[[nodiscard]] static const char* typeName(const QMetaType& type);
	[[nodiscard]] const char* name() const;
	[[nodiscard]] bool isVoid() const;
	[[nodiscard]] const QVariant& value() const;

	bool setString(const QString& string);
	void setValue(QVariant value);
	[[nodiscard]] QString toString() const;

	[[nodiscard]] QGenericArgument asGenericArgument() const;
	[[nodiscard]] QGenericReturnArgument asGenericReturnArgument();

private:
	QVariant mValue;
};

struct WireFunctionDefinition {
	QString name;
	QString returnType;
	QVector<QPair<QString, QString>> arguments;

	[[nodiscard]] QString toString() const;
};

DEFINE_SIMPLE_DATASTREAM_OPS(WireFunctionDefinition, data.name, data.returnType, data.arguments);

struct WirePropertyDefinition {
	QString name;
	QString type;

	[[nodiscard]] QString toString() const;
};

DEFINE_SIMPLE_DATASTREAM_OPS(WirePropertyDefinition, data.name, data.type);

struct WireSignalDefinition {
	QString name;
	QString retname;
	QString rettype;

	[[nodiscard]] QString toString() const;
};

DEFINE_SIMPLE_DATASTREAM_OPS(WireSignalDefinition, data.name, data.retname, data.rettype);

struct WireTargetDefinition {
	QString name;
	QVector<WireFunctionDefinition> functions;
	QVector<WirePropertyDefinition> properties;
	QVector<WireSignalDefinition> signalFunctions;

	[[nodiscard]] QString toString() const;
};

DEFINE_SIMPLE_DATASTREAM_OPS(
    WireTargetDefinition,
    data.name,
    data.functions,
    data.properties,
    data.signalFunctions
);

} // namespace qs::io::ipc
