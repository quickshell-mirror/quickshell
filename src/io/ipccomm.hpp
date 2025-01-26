#pragma once

#include <qcontainerfwd.h>
#include <qflags.h>
#include <qtypes.h>

#include "../ipc/ipc.hpp"

namespace qs::io::ipc::comm {

struct QueryMetadataCommand {
	QString target;
	QString name;

	void exec(qs::ipc::IpcServerConnection* conn) const;
};

DEFINE_SIMPLE_DATASTREAM_OPS(QueryMetadataCommand, data.target, data.name);

struct StringCallCommand {
	QString target;
	QString function;
	QVector<QString> arguments;

	void exec(qs::ipc::IpcServerConnection* conn) const;
};

DEFINE_SIMPLE_DATASTREAM_OPS(StringCallCommand, data.target, data.function, data.arguments);

void handleMsg(qs::ipc::IpcServerConnection* conn);
int queryMetadata(qs::ipc::IpcClient* client, const QString& target, const QString& name);

int callFunction(
    qs::ipc::IpcClient* client,
    const QString& target,
    const QString& function,
    const QVector<QString>& arguments
);

struct StringPropReadCommand {
	QString target;
	QString property;

	void exec(qs::ipc::IpcServerConnection* conn) const;
};

DEFINE_SIMPLE_DATASTREAM_OPS(StringPropReadCommand, data.target, data.property);

int getProperty(qs::ipc::IpcClient* client, const QString& target, const QString& property);

} // namespace qs::io::ipc::comm
