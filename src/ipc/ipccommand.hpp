#pragma once

#include <variant>

#include "../io/ipccomm.hpp"
#include "ipc.hpp"

namespace qs::ipc {

struct IpcKillCommand: std::monostate {
	static void exec(IpcServerConnection* /*unused*/);
};

using IpcCommand = std::variant<
    std::monostate,
    IpcKillCommand,
    qs::io::ipc::comm::QueryMetadataCommand,
    qs::io::ipc::comm::StringCallCommand,
    qs::io::ipc::comm::StringPropReadCommand>;

} // namespace qs::ipc
