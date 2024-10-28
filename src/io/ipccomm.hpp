#pragma once

#include <qcontainerfwd.h>
#include <qflags.h>

#include "../ipc/ipc.hpp"

namespace qs::io::ipc::comm {

struct QueryMetadataCommand {
	QString target;
	QString function;

	void exec(qs::ipc::IpcServerConnection* conn) const;
};

DEFINE_SIMPLE_DATASTREAM_OPS(QueryMetadataCommand, data.target, data.function);

struct StringCallCommand {
	QString target;
	QString function;
	QVector<QString> arguments;

	void exec(qs::ipc::IpcServerConnection* conn) const;
};

DEFINE_SIMPLE_DATASTREAM_OPS(StringCallCommand, data.target, data.function, data.arguments);

void handleMsg(qs::ipc::IpcServerConnection* conn);
int queryMetadata(qs::ipc::IpcClient* client, const QString& target, const QString& function);

int callFunction(
    qs::ipc::IpcClient* client,
    const QString& target,
    const QString& function,
    const QVector<QString>& arguments
);

} // namespace qs::io::ipc::comm
