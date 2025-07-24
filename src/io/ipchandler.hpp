#pragma once

#include <cstddef>
#include <vector>

#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qhash.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qqmlintegration.h>
#include <qqmlparserstatus.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../core/generation.hpp"
#include "../core/reload.hpp"
#include "ipc.hpp"

namespace qs::io::ipc {

class IpcCallStorage;

class IpcFunction {
public:
	explicit IpcFunction(QMetaMethod method): method(method) {}

	bool resolve(QString& error);
	void invoke(QObject* target, IpcCallStorage& storage) const;

	[[nodiscard]] QString toString() const;
	[[nodiscard]] WireFunctionDefinition wireDef() const;

	QMetaMethod method;
	QVector<const IpcType*> argumentTypes;
	const IpcType* returnType = nullptr;
};

class IpcCallStorage {
public:
	explicit IpcCallStorage(const IpcFunction& function);

	bool setArgumentStr(size_t i, const QString& value);
	[[nodiscard]] QString getReturnStr();

private:
	std::vector<IpcTypeSlot> argumentSlots;
	IpcTypeSlot returnSlot;

	friend class IpcFunction;
};

class IpcProperty {
public:
	explicit IpcProperty(QMetaProperty property): property(property) {}

	bool resolve(QString& error);
	void read(QObject* target, IpcTypeSlot& slot) const;

	[[nodiscard]] QString toString() const;
	[[nodiscard]] WirePropertyDefinition wireDef() const;

	QMetaProperty property;
	const IpcType* type = nullptr;
};

class IpcHandlerRegistry;

///! Handler for IPC message calls.
/// Each IpcHandler is registered into a per-instance map by its unique @@target.
/// Functions and properties defined on the IpcHandler can be accessed via `qs ipc`.
///
/// #### Handler Functions
/// IPC handler functions can be called by `qs ipc call` as long as they have at most 10
/// arguments, and all argument types along with the return type are listed below.
///
/// **Argument and return types must be explicitly specified or they will not
/// be registered.**
///
/// ##### Arguments
/// - `string` will be passed to the parameter as is.
/// - `int` will only accept parameters that can be parsed as an integer.
/// - `bool` will only accept parameters that are "true", "false", or an integer,
///   where 0 will be converted to false, and anything else to true.
/// - `real` will only accept parameters that can be parsed as a number with
///   or without a decimal.
/// - `color` will accept [named colors] or hex strings (RGB, RRGGBB, AARRGGBB) with
///   an optional `#` prefix.
///
/// [named colors]: https://doc.qt.io/qt-6/qml-color.html#svg-color-reference
///
/// ##### Return Type
/// - `void` will return nothing.
/// - `string` will be returned as is.
/// - `int` will be converted to a string and returned.
/// - `bool` will be converted to "true" or "false" and returned.
/// - `real` will be converted to a string and returned.
/// - `color` will be converted to a hex string in the form `#AARRGGBB` and returned.
///
/// #### Example
/// The following example creates ipc functions to control and retrieve the appearance
/// of a Rectangle.
///
/// ```qml
/// FloatingWindow {
///   Rectangle {
///     id: rect
///     anchors.centerIn: parent
///     width: 100
///     height: 100
///     color: "red"
///   }
///
///   IpcHandler {
///     target: "rect"
///
///     function setColor(color: color): void { rect.color = color; }
///     function getColor(): color { return rect.color; }
///     function setAngle(angle: real): void { rect.rotation = angle; }
///     function getAngle(): real { return rect.rotation; }
///     function setRadius(radius: int): void { rect.radius = radius; }
///     function getRadius(): int { return rect.radius; }
///   }
/// }
/// ```
/// The list of registered targets can be inspected using `qs ipc show`.
/// ```sh
/// $ qs ipc show
/// target rect
///   function setColor(color: color): void
///   function getColor(): color
///   function setAngle(angle: real): void
///   function getAngle(): real
///   function setRadius(radius: int): void
///   function getRadius(): int
/// ```
///
/// and then invoked using `qs ipc call`.
/// ```sh
/// $ qs ipc call rect setColor orange
/// $ qs ipc call rect setAngle 40.5
/// $ qs ipc call rect setRadius 30
/// $ qs ipc call rect getColor
/// #ffffa500
/// $ qs ipc call rect getAngle
/// 40.5
/// $ qs ipc call rect getRadius
/// 30
/// ```
///
/// #### Properties
/// Properties of an IpcHanlder can be read using `qs ipc prop get` as long as they are
/// of an IPC compatible type. See the table above for compatible types.
class IpcHandler: public PostReloadHook {
	Q_OBJECT;
	/// If the handler should be able to receive calls. Defaults to true.
	Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged);
	/// The target this handler should be accessible from.
	/// Required and must be unique. May be changed at runtime.
	Q_PROPERTY(QString target READ target WRITE setTarget NOTIFY targetChanged);
	QML_ELEMENT;

public:
	explicit IpcHandler(QObject* parent = nullptr): PostReloadHook(parent) {}
	~IpcHandler() override;
	Q_DISABLE_COPY_MOVE(IpcHandler);

	void onPostReload() override;

	[[nodiscard]] bool enabled() const;
	void setEnabled(bool enabled);

	[[nodiscard]] QString target() const;
	void setTarget(const QString& target);

	QString listMembers(qsizetype indent);
	[[nodiscard]] IpcFunction* findFunction(const QString& name);
	[[nodiscard]] IpcProperty* findProperty(const QString& name);
	[[nodiscard]] WireTargetDefinition wireDef() const;

signals:
	void enabledChanged();
	void targetChanged();

private slots:
	//void handleIpcPropertyChange();

private:
	void updateRegistration(bool destroying = false);

	struct RegistrationState {
		explicit RegistrationState(bool enabled = false): enabled(enabled) {}

		bool enabled = false;
		QString target;
	};

	RegistrationState registeredState;
	RegistrationState targetState {true};
	bool complete = false;

	QHash<QString, IpcFunction> functionMap;
	QHash<QString, IpcProperty> propertyMap;

	friend class IpcHandlerRegistry;
};

class IpcHandlerRegistry: public EngineGenerationExt {
public:
	static IpcHandlerRegistry* forGeneration(EngineGeneration* generation);

	void registerHandler(IpcHandler* handler);
	void deregisterHandler(IpcHandler* handler);

	QString listMembers(const QString& target, qsizetype indent);
	QString listTargets(qsizetype indent);

	IpcHandler* findHandler(const QString& target);

	[[nodiscard]] QVector<WireTargetDefinition> wireTargets() const;

private:
	QHash<QString, IpcHandler*> handlers;
	QHash<QString, QVector<IpcHandler*>> knownHandlers;
};

} // namespace qs::io::ipc
