#pragma once

#include <memory>
#include <string>

#include <CLI/App.hpp>
#include <qcoreapplication.h>
#include <qstring.h>

namespace qs::launch {

extern int DAEMON_PIPE; // NOLINT

class QStringOption {
public:
	QStringOption() = default;
	QStringOption& operator=(const std::string& str) {
		this->str = QString::fromStdString(str);
		return *this;
	}

	QString& operator*() { return this->str; }
	QString* operator->() { return &this->str; }

private:
	QString str;
};

struct CommandState {
	std::unique_ptr<CLI::App> app;
	struct {
		int argc = 0;
		char** argv = nullptr;
	} exec;

	struct {
		bool timestamp = false;
		bool noColor = !qEnvironmentVariableIsEmpty("NO_COLOR");
		bool sparse = false;
		size_t verbosity = 0;
		int tail = 0;
		bool follow = false;
		QStringOption rules;
		QStringOption readoutRules;
		QStringOption file;
	} log;

	struct {
		QStringOption path;
		QStringOption manifest;
		QStringOption name;
		bool newest = false;
		bool anyDisplay = false;
	} config;

	struct {
		int port = -1;
		bool wait = false;
	} debug;

	struct {
		QStringOption id;
		pid_t pid = -1; // NOLINT (include)
		bool all = false;
		bool includeDead = false;
	} instance;

	struct {
		bool json = false;
	} output;

	struct {
		CLI::App* ipc = nullptr;
		CLI::App* show = nullptr;
		CLI::App* call = nullptr;
		CLI::App* getprop = nullptr;
		CLI::App* wait = nullptr;
		CLI::App* listen = nullptr;
		bool showOld = false;
		QStringOption target;
		QStringOption name;
		std::vector<QStringOption> arguments;
	} ipc;

	struct {
		CLI::App* log = nullptr;
		CLI::App* list = nullptr;
		CLI::App* kill = nullptr;
		CLI::App* msg = nullptr;
	} subcommand;

	struct {
		bool checkCompat = false;
		bool printVersion = false;
		bool killAll = false;
		bool noDuplicate = true;
		bool daemonize = false;
	} misc;
};

struct LaunchArgs {
	QString configPath;
	int debugPort = -1;
	bool waitForDebug = false;
};

void exitDaemon(int code);

int parseCommand(int argc, char** argv, CommandState& state);
int runCommand(int argc, char** argv, QCoreApplication* coreApplication);

QString getDisplayConnection();

int launch(const LaunchArgs& args, char** argv, QCoreApplication* coreApplication);

} // namespace qs::launch
