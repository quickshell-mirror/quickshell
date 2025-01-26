#include "ipccomm.hpp"
#include <cstdio>
#include <variant>

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qtextstream.h>
#include <qtypes.h>

#include "../core/generation.hpp"
#include "../core/logging.hpp"
#include "../ipc/ipc.hpp"
#include "../ipc/ipccommand.hpp"
#include "ipc.hpp"
#include "ipchandler.hpp"

using namespace qs::ipc;

namespace qs::io::ipc::comm {

struct NoCurrentGeneration: std::monostate {};
struct TargetNotFound: std::monostate {};
struct EntryNotFound: std::monostate {};

using QueryResponse = std::variant<
    std::monostate,
    NoCurrentGeneration,
    TargetNotFound,
    EntryNotFound,
    QVector<WireTargetDefinition>,
    WireTargetDefinition,
    WireFunctionDefinition,
    WirePropertyDefinition>;

void QueryMetadataCommand::exec(qs::ipc::IpcServerConnection* conn) const {
	auto resp = conn->responseStream<QueryResponse>();

	if (auto* generation = EngineGeneration::currentGeneration()) {
		auto* registry = IpcHandlerRegistry::forGeneration(generation);

		if (this->target.isEmpty()) {
			resp << registry->wireTargets();
		} else {
			auto* handler = registry->findHandler(this->target);

			if (handler) {
				if (this->name.isEmpty()) {
					resp << handler->wireDef();
				} else {
					auto* func = handler->findFunction(this->name);

					if (func) {
						resp << func->wireDef();
						return;
					}

					auto* prop = handler->findProperty(this->name);

					if (prop) {
						resp << prop->wireDef();
						return;
					}

					resp << EntryNotFound();
				}
			} else {
				resp << TargetNotFound();
			}
		}
	} else {
		resp << NoCurrentGeneration();
	}
}

int queryMetadata(IpcClient* client, const QString& target, const QString& name) {
	client->sendMessage(IpcCommand(QueryMetadataCommand {.target = target, .name = name}));

	QueryResponse slot;
	if (!client->waitForResponse(slot)) return -1;

	if (std::holds_alternative<QVector<WireTargetDefinition>>(slot)) {
		const auto& targets = std::get<QVector<WireTargetDefinition>>(slot);

		for (const auto& target: targets) {
			qCInfo(logBare).noquote() << target.toString();
		}

		return 0;
	} else if (std::holds_alternative<WireTargetDefinition>(slot)) {
		qCInfo(logBare).noquote() << std::get<WireTargetDefinition>(slot).toString();
	} else if (std::holds_alternative<WireFunctionDefinition>(slot)) {
		qCInfo(logBare).noquote() << std::get<WireFunctionDefinition>(slot).toString();
	} else if (std::holds_alternative<WirePropertyDefinition>(slot)) {
		qCInfo(logBare).noquote() << std::get<WirePropertyDefinition>(slot).toString();
	} else if (std::holds_alternative<TargetNotFound>(slot)) {
		qCCritical(logBare) << "Target not found.";
	} else if (std::holds_alternative<EntryNotFound>(slot)) {
		qCCritical(logBare) << "Function not found.";
	} else if (std::holds_alternative<NoCurrentGeneration>(slot)) {
		qCCritical(logBare) << "Not ready to accept queries yet.";
	} else {
		qCCritical(logIpc) << "Received invalid IPC response from" << client;
	}

	return -1;
}

struct ArgParseFailed {
	WireFunctionDefinition definition;
	bool isCountMismatch = false;
	quint8 paramIndex = 0;
};

DEFINE_SIMPLE_DATASTREAM_OPS(
    ArgParseFailed,
    data.definition,
    data.isCountMismatch,
    data.paramIndex
);

struct Completed {
	bool isVoid = false;
	QString returnValue;
};

DEFINE_SIMPLE_DATASTREAM_OPS(Completed, data.isVoid, data.returnValue);

using StringCallResponse = std::variant<
    std::monostate,
    NoCurrentGeneration,
    TargetNotFound,
    EntryNotFound,
    ArgParseFailed,
    Completed>;

void StringCallCommand::exec(qs::ipc::IpcServerConnection* conn) const {
	auto resp = conn->responseStream<StringCallResponse>();

	if (auto* generation = EngineGeneration::currentGeneration()) {
		auto* registry = IpcHandlerRegistry::forGeneration(generation);

		auto* handler = registry->findHandler(this->target);
		if (!handler) {
			resp << TargetNotFound();
			return;
		}

		auto* func = handler->findFunction(this->function);
		if (!func) {
			resp << EntryNotFound();
			return;
		}

		if (func->argumentTypes.length() != this->arguments.length()) {
			resp << ArgParseFailed {
			    .definition = func->wireDef(),
			    .isCountMismatch = true,
			};

			return;
		}

		auto storage = IpcCallStorage(*func);
		for (auto i = 0; i < this->arguments.length(); i++) {
			if (!storage.setArgumentStr(i, this->arguments.value(i))) {
				resp << ArgParseFailed {
				    .definition = func->wireDef(),
				    .paramIndex = static_cast<quint8>(i),
				};

				return;
			}
		}

		func->invoke(handler, storage);

		resp << Completed {
		    .isVoid = func->returnType == &VoidIpcType::INSTANCE,
		    .returnValue = storage.getReturnStr(),
		};
	} else {
		conn->respond(StringCallResponse(NoCurrentGeneration()));
	}
}

int callFunction(
    IpcClient* client,
    const QString& target,
    const QString& function,
    const QVector<QString>& arguments
) {
	if (target.isEmpty()) {
		qCCritical(logBare) << "Target required to send message.";
		return -1;
	} else if (function.isEmpty()) {
		qCCritical(logBare) << "Function required to send message.";
		return -1;
	}

	client->sendMessage(
	    IpcCommand(StringCallCommand {.target = target, .function = function, .arguments = arguments})
	);

	StringCallResponse slot;
	if (!client->waitForResponse(slot)) return -1;

	if (std::holds_alternative<Completed>(slot)) {
		auto& result = std::get<Completed>(slot);
		if (!result.isVoid) {
			QTextStream(stdout) << result.returnValue << Qt::endl;
		}

		return 0;
	} else if (std::holds_alternative<ArgParseFailed>(slot)) {
		auto& error = std::get<ArgParseFailed>(slot);

		if (error.isCountMismatch) {
			auto correctCount = error.definition.arguments.length();

			qCCritical(logBare).nospace()
			    << "Too " << (correctCount < arguments.length() ? "many" : "few")
			    << " arguments provided (" << correctCount << " required but " << arguments.length()
			    << " were provided.)";
		} else {
			const auto& provided = arguments.at(error.paramIndex);
			const auto& definition = error.definition.arguments.at(error.paramIndex);

			qCCritical(logBare).nospace()
			    << "Unable to parse argument " << (error.paramIndex + 1) << " as " << definition.second
			    << ". Provided argument: " << provided;
		}

		qCCritical(logBare).noquote() << "Function definition:" << error.definition.toString();
	} else if (std::holds_alternative<TargetNotFound>(slot)) {
		qCCritical(logBare) << "Target not found.";
	} else if (std::holds_alternative<EntryNotFound>(slot)) {
		qCCritical(logBare) << "Function not found.";
	} else if (std::holds_alternative<NoCurrentGeneration>(slot)) {
		qCCritical(logBare) << "Not ready to accept queries yet.";
	} else {
		qCCritical(logIpc) << "Received invalid IPC response from" << client;
	}

	return -1;
}

struct PropertyValue {
	QString value;
};

DEFINE_SIMPLE_DATASTREAM_OPS(PropertyValue, data.value);

using StringPropReadResponse =
    std::variant<std::monostate, NoCurrentGeneration, TargetNotFound, EntryNotFound, PropertyValue>;

void StringPropReadCommand::exec(qs::ipc::IpcServerConnection* conn) const {
	auto resp = conn->responseStream<StringPropReadResponse>();

	if (auto* generation = EngineGeneration::currentGeneration()) {
		auto* registry = IpcHandlerRegistry::forGeneration(generation);

		auto* handler = registry->findHandler(this->target);
		if (!handler) {
			resp << TargetNotFound();
			return;
		}

		auto* prop = handler->findProperty(this->property);
		if (!prop) {
			resp << EntryNotFound();
			return;
		}

		auto slot = IpcTypeSlot(prop->type);
		prop->read(handler, slot);

		resp << PropertyValue {
		    .value = slot.type()->toString(slot.get()),
		};
	} else {
		conn->respond(StringCallResponse(NoCurrentGeneration()));
	}
}

int getProperty(IpcClient* client, const QString& target, const QString& property) {
	if (target.isEmpty()) {
		qCCritical(logBare) << "Target required to send message.";
		return -1;
	} else if (property.isEmpty()) {
		qCCritical(logBare) << "Property required to send message.";
		return -1;
	}

	client->sendMessage(IpcCommand(StringPropReadCommand {.target = target, .property = property}));

	StringPropReadResponse slot;
	if (!client->waitForResponse(slot)) return -1;

	if (std::holds_alternative<PropertyValue>(slot)) {
		auto& result = std::get<PropertyValue>(slot);
		QTextStream(stdout) << result.value << Qt::endl;
		return 0;
	} else if (std::holds_alternative<TargetNotFound>(slot)) {
		qCCritical(logBare) << "Target not found.";
	} else if (std::holds_alternative<EntryNotFound>(slot)) {
		qCCritical(logBare) << "Property not found.";
	} else if (std::holds_alternative<NoCurrentGeneration>(slot)) {
		qCCritical(logBare) << "Not ready to accept queries yet.";
	} else {
		qCCritical(logIpc) << "Received invalid IPC response from" << client;
	}

	return -1;
}

} // namespace qs::io::ipc::comm
