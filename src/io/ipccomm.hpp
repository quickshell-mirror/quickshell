#pragma once

#include <qcontainerfwd.h>
#include <qflags.h>
#include <qobject.h>
#include <qtmetamacros.h>
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

struct SignalListenCommand {
	QString target;
	QString signal;

	void exec(qs::ipc::IpcServerConnection* conn);
};

DEFINE_SIMPLE_DATASTREAM_OPS(SignalListenCommand, data.target, data.signal);

int listenToSignal(
    qs::ipc::IpcClient* client,
    const QString& target,
    const QString& signal,
    bool once
);

struct NoCurrentGeneration: std::monostate {};
struct TargetNotFound: std::monostate {};
struct EntryNotFound: std::monostate {};

struct SignalResponse {
	QString response;
};

DEFINE_SIMPLE_DATASTREAM_OPS(SignalResponse, data.response);

using SignalListenResponse = std::
    variant<std::monostate, NoCurrentGeneration, TargetNotFound, EntryNotFound, SignalResponse>;

class RemoteSignalListener: public QObject {
	Q_OBJECT;

public:
	explicit RemoteSignalListener(qs::ipc::IpcServerConnection* conn, SignalListenCommand command);

	~RemoteSignalListener() override;

	Q_DISABLE_COPY_MOVE(RemoteSignalListener);

private slots:
	void onSignal(const QString& target, const QString& signal, const QString& value);
	void onConnDestroyed();

private:
	qs::ipc::IpcServerConnection* conn;
	SignalListenCommand command;
};

} // namespace qs::io::ipc::comm
