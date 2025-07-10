#include "ipchandler.hpp"
#include <cstddef>

#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qpair.h>
#include <qqmlinfo.h>
#include <qstringbuilder.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/generation.hpp"
#include "../core/logcat.hpp"
#include "ipc.hpp"

namespace qs::io::ipc {

namespace {
QS_LOGGING_CATEGORY(logIpcHandler, "quickshell.ipchandler", QtWarningMsg)
}

bool IpcFunction::resolve(QString& error) {
	if (this->method.parameterCount() > 10) {
		error = "Due to technical limitations, IPC functions can only have 10 arguments.";
		return false;
	}

	for (auto i = 0; i < this->method.parameterCount(); i++) {
		const auto& metaType = this->method.parameterMetaType(i);
		const auto* type = IpcType::ipcType(metaType);

		if (type == nullptr) {
			error = QString("Type of argument %1 (%2: %3) cannot be used across IPC.")
			            .arg(i + 1)
			            .arg(this->method.parameterNames().value(i))
			            .arg(metaType.name());

			return false;
		}

		this->argumentTypes.append(type);
	}

	const auto& metaType = this->method.returnMetaType();
	const auto* type = IpcType::ipcType(metaType);

	if (type == nullptr) {
		// void and var get mixed by qml engine in return types
		if (metaType.id() == QMetaType::QVariant) type = &VoidIpcType::INSTANCE;

		if (type == nullptr) {
			error = QString("Return type (%1) cannot be used across IPC.").arg(metaType.name());
			return false;
		}
	}

	this->returnType = type;

	return true;
}

void IpcFunction::invoke(QObject* target, IpcCallStorage& storage) const {
	auto getArg = [&](size_t i) {
		return i < storage.argumentSlots.size() ? storage.argumentSlots.at(i).asGenericArgument()
		                                        : QGenericArgument();
	};

	this->method.invoke(
	    target,
	    storage.returnSlot.asGenericReturnArgument(),
	    getArg(0),
	    getArg(1),
	    getArg(2),
	    getArg(3),
	    getArg(4),
	    getArg(5),
	    getArg(6),
	    getArg(7),
	    getArg(8),
	    getArg(9)
	);
}

QString IpcFunction::toString() const {
	QString paramString;
	auto paramNames = this->method.parameterNames();
	for (auto i = 0; i < this->argumentTypes.length(); i++) {
		paramString += paramNames.value(i) % ": " % this->argumentTypes.value(i)->name();

		if (i + 1 != this->argumentTypes.length()) {
			paramString += ", ";
		}
	}

	return "function " % this->method.name() % '(' % paramString % "): " % this->returnType->name();
}

WireFunctionDefinition IpcFunction::wireDef() const {
	WireFunctionDefinition wire;
	wire.name = this->method.name();
	wire.returnType = this->returnType->name();

	auto paramNames = this->method.parameterNames();
	for (auto i = 0; i < this->argumentTypes.length(); i++) {
		wire.arguments += qMakePair(paramNames.value(i), this->argumentTypes.value(i)->name());
	}

	return wire;
}

bool IpcProperty::resolve(QString& error) {
	this->type = IpcType::ipcType(this->property.metaType());

	if (!this->type) {
		error = QString("Type %1 cannot be used across IPC.").arg(this->property.metaType().name());
		return false;
	}

	return true;
}

void IpcProperty::read(QObject* target, IpcTypeSlot& slot) const {
	slot.replace(this->property.read(target));
}

QString IpcProperty::toString() const {
	return QString("property ") % this->property.name() % ": " % this->type->name();
}

WirePropertyDefinition IpcProperty::wireDef() const {
	WirePropertyDefinition wire;
	wire.name = this->property.name();
	wire.type = this->type->name();
	return wire;
}

IpcCallStorage::IpcCallStorage(const IpcFunction& function): returnSlot(function.returnType) {
	for (const auto& arg: function.argumentTypes) {
		this->argumentSlots.emplace_back(arg);
	}
}

bool IpcCallStorage::setArgumentStr(size_t i, const QString& value) {
	auto& slot = this->argumentSlots.at(i);

	auto* data = slot.type()->fromString(value);
	slot.replace(data);
	return data != nullptr;
}

QString IpcCallStorage::getReturnStr() {
	return this->returnSlot.type()->toString(this->returnSlot.get());
}

IpcHandler::~IpcHandler() {
	if (this->registeredState.enabled) {
		this->targetState.enabled = false;
		this->updateRegistration(true);
	}
}

void IpcHandler::onPostReload() {
	const auto& smeta = IpcHandler::staticMetaObject;
	const auto* meta = this->metaObject();

	// Start at the first function following IpcHandler's slots,
	// which should handle inheritance on the qml side.
	for (auto i = smeta.methodCount(); i != meta->methodCount(); i++) {
		const auto& method = meta->method(i);
		if (method.methodType() != QMetaMethod::Slot) continue;

		auto ipcFunc = IpcFunction(method);
		QString error;

		if (!ipcFunc.resolve(error)) {
			qmlWarning(this).nospace().noquote()
			    << "Error parsing function \"" << method.name() << "\": " << error;
		} else {
			this->functionMap.insert(method.name(), ipcFunc);
		}
	}

	for (auto i = smeta.propertyCount(); i != meta->propertyCount(); i++) {
		const auto& property = meta->property(i);
		if (!property.isReadable() || !property.hasNotifySignal()) continue;

		auto ipcProp = IpcProperty(property);
		QString error;

		if (!ipcProp.resolve(error)) {
			qmlWarning(this).nospace().noquote()
			    << "Error parsing property \"" << property.name() << "\": " << error;
		} else {
			this->propertyMap.insert(property.name(), ipcProp);
		}
	}

	this->complete = true;
	this->updateRegistration();

	if (this->targetState.enabled && this->targetState.target.isEmpty()) {
		qmlWarning(this) << "This IPC handler is enabled but no target is set. This means it is "
		                    "effectively inoperable.";
	}
}

IpcHandlerRegistry* IpcHandlerRegistry::forGeneration(EngineGeneration* generation) {
	static const int key = 0;
	auto* ext = generation->findExtension(&key);

	if (!ext) {
		ext = new IpcHandlerRegistry();
		generation->registerExtension(&key, ext);
		qCDebug(logIpcHandler) << "Created new IPC handler registry" << ext << "for" << generation;
	}

	return dynamic_cast<IpcHandlerRegistry*>(ext);
}

void IpcHandler::updateRegistration(bool destroying) {
	if (!this->complete) return;

	auto* generation = EngineGeneration::findObjectGeneration(this);

	if (!generation) {
		if (!destroying) {
			qmlWarning(this) << "Unable to identify engine generation, cannot register.";
		}

		return;
	}

	auto* registry = IpcHandlerRegistry::forGeneration(generation);

	if (this->registeredState.enabled) {
		registry->deregisterHandler(this);
		qCDebug(logIpcHandler) << "Deregistered" << this << "from registry" << registry;
	}

	if (this->targetState.enabled && !this->targetState.target.isEmpty()) {
		registry->registerHandler(this);
		qCDebug(logIpcHandler) << "Registered" << this << "to registry" << registry;
	}
}

bool IpcHandler::enabled() const { return this->targetState.enabled; }

void IpcHandler::setEnabled(bool enabled) {
	if (enabled != this->targetState.enabled) {
		this->targetState.enabled = enabled;
		emit this->enabledChanged();
		this->updateRegistration();
	}
}

QString IpcHandler::target() const { return this->targetState.target; }

void IpcHandler::setTarget(const QString& target) {
	if (target != this->targetState.target) {
		this->targetState.target = target;
		emit this->targetChanged();
		this->updateRegistration();
	}
}

void IpcHandlerRegistry::registerHandler(IpcHandler* handler) {
	// inserting a new vec if not present is the desired behavior
	auto& targetVec = this->knownHandlers[handler->targetState.target];
	targetVec.append(handler);

	if (this->handlers.contains(handler->targetState.target)) {
		qmlWarning(handler) << "Handler was registered but will not be used because another handler "
		                       "is registered for target "
		                    << handler->targetState.target;
	} else {
		this->handlers.insert(handler->targetState.target, handler);
	}

	handler->registeredState = handler->targetState;
	handler->registeredState.enabled = true;
}

void IpcHandlerRegistry::deregisterHandler(IpcHandler* handler) {
	auto& targetVec = this->knownHandlers[handler->registeredState.target];
	targetVec.removeOne(handler);

	if (this->handlers.value(handler->registeredState.target) == handler) {
		if (targetVec.isEmpty()) {
			this->handlers.remove(handler->registeredState.target);
		} else {
			this->handlers.insert(handler->registeredState.target, targetVec.first());
		}
	}

	handler->registeredState = IpcHandler::RegistrationState(false);
}

QString IpcHandler::listMembers(qsizetype indent) {
	auto indentStr = QString(indent, ' ');
	QString accum;

	for (const auto& func: this->functionMap.values()) {
		if (!accum.isEmpty()) accum += '\n';
		accum += indentStr % func.toString();
	}

	return accum;
}

WireTargetDefinition IpcHandler::wireDef() const {
	WireTargetDefinition wire;
	wire.name = this->registeredState.target;

	for (const auto& func: this->functionMap.values()) {
		wire.functions += func.wireDef();
	}

	for (const auto& prop: this->propertyMap.values()) {
		wire.properties += prop.wireDef();
	}

	return wire;
}

QString IpcHandlerRegistry::listMembers(const QString& target, qsizetype indent) {
	if (auto* handler = this->handlers.value(target)) {
		return handler->listMembers(indent);
	} else {
		QString accum;

		for (auto* handler: this->knownHandlers.value(target)) {
			if (!accum.isEmpty()) accum += '\n';
			accum += handler->listMembers(indent);
		}

		return accum;
	}
}

QString IpcHandlerRegistry::listTargets(qsizetype indent) {
	auto indentStr = QString(indent, ' ');
	QString accum;

	for (const auto& target: this->knownHandlers.keys()) {
		if (!accum.isEmpty()) accum += '\n';
		accum += indentStr % "Target " % target % '\n' % this->listMembers(target, indent + 2);
	}

	return accum;
}

IpcFunction* IpcHandler::findFunction(const QString& name) {
	auto itr = this->functionMap.find(name);

	if (itr == this->functionMap.end()) return nullptr;
	else return &*itr;
}

IpcProperty* IpcHandler::findProperty(const QString& name) {
	auto itr = this->propertyMap.find(name);

	if (itr == this->propertyMap.end()) return nullptr;
	else return &*itr;
}

IpcHandler* IpcHandlerRegistry::findHandler(const QString& target) {
	return this->handlers.value(target);
}

QVector<WireTargetDefinition> IpcHandlerRegistry::wireTargets() const {
	QVector<WireTargetDefinition> wire;

	for (const auto* handler: this->handlers.values()) {
		wire += handler->wireDef();
	}

	return wire;
}

} // namespace qs::io::ipc
