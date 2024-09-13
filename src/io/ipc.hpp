#pragma once

#include <qcontainerfwd.h>
#include <qobjectdefs.h>
#include <qtclasshelpermacros.h>

#include "../core/ipc.hpp"

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
	[[nodiscard]] virtual void* fromString(const QString& /*string*/) const { return nullptr; }
	[[nodiscard]] virtual QString toString(void* /*slot*/) const { return ""; }
	[[nodiscard]] virtual void* createStorage() const { return nullptr; }
	virtual void destroyStorage(void* /*slot*/) const {}

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

private:
	const IpcType* mType = nullptr;
	void* storage = nullptr;
};

class VoidIpcType: public IpcType {
public:
	[[nodiscard]] const char* name() const override;
	[[nodiscard]] const char* genericArgumentName() const override;

	static const VoidIpcType INSTANCE;
};

class StringIpcType: public IpcType {
public:
	[[nodiscard]] const char* name() const override;
	[[nodiscard]] const char* genericArgumentName() const override;
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

struct WireTargetDefinition {
	QString name;
	QVector<WireFunctionDefinition> functions;

	[[nodiscard]] QString toString() const;
};

DEFINE_SIMPLE_DATASTREAM_OPS(WireTargetDefinition, data.name, data.functions);

} // namespace qs::io::ipc
