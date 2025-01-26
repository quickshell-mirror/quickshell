#pragma once

#include <qcontainerfwd.h>
#include <qobjectdefs.h>
#include <qtclasshelpermacros.h>
#include <qtypes.h>

#include "../ipc/ipc.hpp"

namespace qs::io::ipc {

class IpcTypeSlot;

class IpcType {
public:
	IpcType() = default;
	virtual ~IpcType() = default;
	IpcType(const IpcType&) = default;
	IpcType(IpcType&&) = default;
	IpcType& operator=(const IpcType&) = default;
	IpcType& operator=(IpcType&&) = default;

	[[nodiscard]] virtual const char* name() const = 0;
	[[nodiscard]] virtual const char* genericArgumentName() const = 0;
	[[nodiscard]] virtual qsizetype size() const = 0;
	[[nodiscard]] virtual void* fromString(const QString& /*string*/) const { return nullptr; }
	[[nodiscard]] virtual QString toString(void* /*slot*/) const { return ""; }
	[[nodiscard]] virtual void* createStorage() const { return nullptr; }
	virtual void destroyStorage(void* /*slot*/) const {}
	void* copyStorage(const void* data) const;

	static const IpcType* ipcType(const QMetaType& metaType);
};

class IpcTypeSlot {
public:
	explicit IpcTypeSlot(const IpcType* type = nullptr): mType(type) {}
	~IpcTypeSlot();
	Q_DISABLE_COPY(IpcTypeSlot);
	IpcTypeSlot(IpcTypeSlot&& other) noexcept;
	IpcTypeSlot& operator=(IpcTypeSlot&& other) noexcept;

	[[nodiscard]] const IpcType* type() const;
	[[nodiscard]] void* get();
	[[nodiscard]] QGenericArgument asGenericArgument();
	[[nodiscard]] QGenericReturnArgument asGenericReturnArgument();

	void replace(void* value);
	void replace(const QVariant& value);

private:
	const IpcType* mType = nullptr;
	void* storage = nullptr;
};

class VoidIpcType: public IpcType {
public:
	[[nodiscard]] const char* name() const override;
	[[nodiscard]] const char* genericArgumentName() const override;
	[[nodiscard]] qsizetype size() const override;

	static const VoidIpcType INSTANCE;
};

class StringIpcType: public IpcType {
public:
	[[nodiscard]] const char* name() const override;
	[[nodiscard]] const char* genericArgumentName() const override;
	[[nodiscard]] qsizetype size() const override;
	[[nodiscard]] void* fromString(const QString& string) const override;
	[[nodiscard]] QString toString(void* slot) const override;
	[[nodiscard]] void* createStorage() const override;
	void destroyStorage(void* slot) const override;

	static const StringIpcType INSTANCE;
};

class IntIpcType: public IpcType {
public:
	[[nodiscard]] const char* name() const override;
	[[nodiscard]] const char* genericArgumentName() const override;
	[[nodiscard]] qsizetype size() const override;
	[[nodiscard]] void* fromString(const QString& string) const override;
	[[nodiscard]] QString toString(void* slot) const override;
	[[nodiscard]] void* createStorage() const override;
	void destroyStorage(void* slot) const override;

	static const IntIpcType INSTANCE;
};

class BoolIpcType: public IpcType {
public:
	[[nodiscard]] const char* name() const override;
	[[nodiscard]] const char* genericArgumentName() const override;
	[[nodiscard]] qsizetype size() const override;
	[[nodiscard]] void* fromString(const QString& string) const override;
	[[nodiscard]] QString toString(void* slot) const override;
	[[nodiscard]] void* createStorage() const override;
	void destroyStorage(void* slot) const override;

	static const BoolIpcType INSTANCE;
};

class DoubleIpcType: public IpcType {
public:
	[[nodiscard]] const char* name() const override;
	[[nodiscard]] const char* genericArgumentName() const override;
	[[nodiscard]] qsizetype size() const override;
	[[nodiscard]] void* fromString(const QString& string) const override;
	[[nodiscard]] QString toString(void* slot) const override;
	[[nodiscard]] void* createStorage() const override;
	void destroyStorage(void* slot) const override;

	static const DoubleIpcType INSTANCE;
};

class ColorIpcType: public IpcType {
public:
	[[nodiscard]] const char* name() const override;
	[[nodiscard]] const char* genericArgumentName() const override;
	[[nodiscard]] qsizetype size() const override;
	[[nodiscard]] void* fromString(const QString& string) const override;
	[[nodiscard]] QString toString(void* slot) const override;
	[[nodiscard]] void* createStorage() const override;
	void destroyStorage(void* slot) const override;

	static const ColorIpcType INSTANCE;
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

struct WireTargetDefinition {
	QString name;
	QVector<WireFunctionDefinition> functions;
	QVector<WirePropertyDefinition> properties;

	[[nodiscard]] QString toString() const;
};

DEFINE_SIMPLE_DATASTREAM_OPS(WireTargetDefinition, data.name, data.functions, data.properties);

} // namespace qs::io::ipc
