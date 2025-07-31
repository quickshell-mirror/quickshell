#include "qmlglobal.hpp"
#include <utility>

#include <qclipboard.h>
#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qdir.h>
#include <qguiapplication.h>
#include <qicon.h>
#include <qjsengine.h>
#include <qlist.h>
#include <qlogging.h>
#include <qobject.h>
#include <qprocess.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qscreen.h>
#include <qtenvironmentvariables.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <qwindowdefs.h>
#include <unistd.h>

#include "../io/processcore.hpp"
#include "generation.hpp"
#include "iconimageprovider.hpp"
#include "paths.hpp"
#include "qmlscreen.hpp"
#include "rootwrapper.hpp"

QuickshellSettings::QuickshellSettings() {
	QObject::connect(
	    static_cast<QGuiApplication*>(QGuiApplication::instance()), // NOLINT
	    &QGuiApplication::lastWindowClosed,
	    this,
	    &QuickshellSettings::lastWindowClosed
	);
}

QuickshellSettings* QuickshellSettings::instance() {
	static QuickshellSettings* instance = nullptr; // NOLINT
	if (instance == nullptr) {
		QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
		instance = new QuickshellSettings();
	}
	return instance;
}

void QuickshellSettings::reset() { QuickshellSettings::instance()->mWatchFiles = true; }

QString QuickshellSettings::workingDirectory() const { // NOLINT
	return QDir::current().absolutePath();
}

void QuickshellSettings::setWorkingDirectory(QString workingDirectory) { // NOLINT
	QDir::setCurrent(workingDirectory);
	emit this->workingDirectoryChanged();
}

bool QuickshellSettings::watchFiles() const { return this->mWatchFiles; }

void QuickshellSettings::setWatchFiles(bool watchFiles) {
	if (watchFiles == this->mWatchFiles) return;
	this->mWatchFiles = watchFiles;
	emit this->watchFilesChanged();
}

QuickshellTracked::QuickshellTracked() {
	auto* app = QCoreApplication::instance();
	auto* guiApp = qobject_cast<QGuiApplication*>(app);

	if (guiApp != nullptr) {
		// clang-format off
		QObject::connect(guiApp, &QGuiApplication::primaryScreenChanged, this, &QuickshellTracked::updateScreens);
		QObject::connect(guiApp, &QGuiApplication::screenAdded, this, &QuickshellTracked::updateScreens);
		QObject::connect(guiApp, &QGuiApplication::screenRemoved, this, &QuickshellTracked::updateScreens);
		// clang-format on

		this->updateScreens();
	}
}

QuickshellScreenInfo* QuickshellTracked::screenInfo(QScreen* screen) const {
	for (auto* info: this->screens) {
		if (info->screen == screen) return info;
	}

	return nullptr;
}

QuickshellTracked* QuickshellTracked::instance() {
	static QuickshellTracked* instance = nullptr; // NOLINT
	if (instance == nullptr) {
		QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
		instance = new QuickshellTracked();
	}
	return instance;
}

void QuickshellTracked::updateScreens() {
	auto screens = QGuiApplication::screens();
	auto newScreens = QList<QuickshellScreenInfo*>();

	for (auto* newScreen: screens) {
		for (auto i = 0; i < this->screens.length(); i++) {
			auto* oldScreen = this->screens[i];
			if (newScreen == oldScreen->screen) {
				newScreens.push_back(oldScreen);
				this->screens.remove(i);
				goto next;
			}
		}

		{
			auto* si = new QuickshellScreenInfo(this, newScreen);
			QQmlEngine::setObjectOwnership(si, QQmlEngine::CppOwnership);
			newScreens.push_back(si);
		}
	next:;
	}

	for (auto* oldScreen: this->screens) {
		oldScreen->deleteLater();
	}

	this->screens = newScreens;
	emit this->screensChanged();
}

QuickshellGlobal::QuickshellGlobal(QObject* parent): QObject(parent) {
	// clang-format off
	QObject::connect(QuickshellSettings::instance(), &QuickshellSettings::workingDirectoryChanged, this, &QuickshellGlobal::workingDirectoryChanged);
	QObject::connect(QuickshellSettings::instance(), &QuickshellSettings::watchFilesChanged, this, &QuickshellGlobal::watchFilesChanged);
	QObject::connect(QuickshellSettings::instance(), &QuickshellSettings::lastWindowClosed, this, &QuickshellGlobal::lastWindowClosed);

	QObject::connect(QuickshellTracked::instance(), &QuickshellTracked::screensChanged, this, &QuickshellGlobal::screensChanged);
	// clang-format on

	QObject::connect(
	    static_cast<QGuiApplication*>(QGuiApplication::instance())->clipboard(), // NOLINT
	    &QClipboard::changed,
	    this,
	    &QuickshellGlobal::onClipboardChanged
	);
}

qint32 QuickshellGlobal::processId() const { // NOLINT
	return getpid();
}

qsizetype QuickshellGlobal::screensCount(QQmlListProperty<QuickshellScreenInfo>* /*unused*/) {
	return QuickshellTracked::instance()->screens.size();
}

QuickshellScreenInfo*
QuickshellGlobal::screenAt(QQmlListProperty<QuickshellScreenInfo>* /*unused*/, qsizetype i) {
	return QuickshellTracked::instance()->screens.at(i);
}

QQmlListProperty<QuickshellScreenInfo> QuickshellGlobal::screens() {
	return QQmlListProperty<QuickshellScreenInfo>(
	    this,
	    nullptr,
	    &QuickshellGlobal::screensCount,
	    &QuickshellGlobal::screenAt
	);
}

void QuickshellGlobal::reload(bool hard) {
	auto* generation = EngineGeneration::findObjectGeneration(this);
	auto* root = generation == nullptr ? nullptr : generation->wrapper;

	if (root == nullptr) {
		qWarning() << "cannot find RootWrapper for reload, ignoring request";
		return;
	}

	root->reloadGraph(hard);
}

QString QuickshellGlobal::workingDirectory() const { // NOLINT
	return QuickshellSettings::instance()->workingDirectory();
}

void QuickshellGlobal::setWorkingDirectory(QString workingDirectory) { // NOLINT
	QuickshellSettings::instance()->setWorkingDirectory(std::move(workingDirectory));
}

bool QuickshellGlobal::watchFiles() const { // NOLINT
	return QuickshellSettings::instance()->watchFiles();
}

void QuickshellGlobal::setWatchFiles(bool watchFiles) { // NOLINT
	QuickshellSettings::instance()->setWatchFiles(watchFiles);
}

QString QuickshellGlobal::clipboardText() {
	return static_cast<QGuiApplication*>(QGuiApplication::instance())->clipboard()->text(); // NOLINT
}

void QuickshellGlobal::setClipboardText(const QString& text) {
	return static_cast<QGuiApplication*>(QGuiApplication::instance()) // NOLINT
	    ->clipboard()
	    ->setText(text);
}

void QuickshellGlobal::onClipboardChanged(QClipboard::Mode mode) {
	if (mode == QClipboard::Clipboard) emit this->clipboardTextChanged();
}

QString QuickshellGlobal::shellDir() const {
	return EngineGeneration::findObjectGeneration(this)->rootPath.path();
}

QString QuickshellGlobal::configDir() const {
	qWarning() << "Quickshell.configDir is deprecated and may be removed in a future release. Use "
	              "Quickshell.shellDir.";
	return this->shellDir();
}

QString QuickshellGlobal::shellRoot() const {
	qWarning() << "Quickshell.shellRoot is deprecated and may be removed in a future release. Use "
	              "Quickshell.shellDir.";
	return this->shellDir();
}

QString QuickshellGlobal::dataDir() const { // NOLINT
	return QsPaths::instance()->shellDataDir().path();
}

QString QuickshellGlobal::stateDir() const { // NOLINT
	return QsPaths::instance()->shellStateDir().path();
}

QString QuickshellGlobal::cacheDir() const { // NOLINT
	return QsPaths::instance()->shellCacheDir().path();
}

QString QuickshellGlobal::shellPath(const QString& path) const {
	return this->shellDir() % '/' % path;
}

QString QuickshellGlobal::configPath(const QString& path) const {
	qWarning() << "Quickshell.configPath() is deprecated and may be removed in a future release. Use "
	              "Quickshell.shellPath().";
	return this->shellPath(path);
}

QString QuickshellGlobal::dataPath(const QString& path) const {
	return this->dataDir() % '/' % path;
}

QString QuickshellGlobal::statePath(const QString& path) const {
	return this->stateDir() % '/' % path;
}

QString QuickshellGlobal::cachePath(const QString& path) const {
	return this->cacheDir() % '/' % path;
}

QVariant QuickshellGlobal::env(const QString& variable) { // NOLINT
	auto vstr = variable.toStdString();
	if (!qEnvironmentVariableIsSet(vstr.data())) return QVariant::fromValue(nullptr);

	return qEnvironmentVariable(vstr.data());
}

void QuickshellGlobal::execDetached(QList<QString> command) {
	QuickshellGlobal::execDetached(qs::io::process::ProcessContext(std::move(command)));
}

void QuickshellGlobal::execDetached(const qs::io::process::ProcessContext& context) {
	if (context.command.isEmpty()) {
		qWarning() << "Cannot start process as command is empty.";
		return;
	}

	const auto& cmd = context.command.first();
	auto args = context.command.sliced(1);

	QProcess process;
	qs::io::process::setupProcessEnvironment(&process, context.clearEnvironment, context.environment);

	if (!context.workingDirectory.isEmpty()) {
		process.setWorkingDirectory(context.workingDirectory);
	}

	process.setProgram(cmd);
	process.setArguments(args);

	process.setStandardInputFile(QProcess::nullDevice());

	if (context.unbindStdout) {
		process.setStandardOutputFile(QProcess::nullDevice());
		process.setStandardErrorFile(QProcess::nullDevice());
	}

	process.startDetached();
}

QString QuickshellGlobal::iconPath(const QString& icon) {
	return IconImageProvider::requestString(icon);
}

QString QuickshellGlobal::iconPath(const QString& icon, bool check) {
	if (check && QIcon::fromTheme(icon).isNull()) return "";
	return IconImageProvider::requestString(icon);
}

QString QuickshellGlobal::iconPath(const QString& icon, const QString& fallback) {
	return IconImageProvider::requestString(icon, "", fallback);
}

QuickshellGlobal* QuickshellGlobal::create(QQmlEngine* engine, QJSEngine* /*unused*/) {
	auto* qsg = new QuickshellGlobal();
	auto* generation = EngineGeneration::findEngineGeneration(engine);

	if (generation->qsgInstance == nullptr) {
		generation->qsgInstance = qsg;
	}

	return qsg;
}
