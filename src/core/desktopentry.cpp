#include "desktopentry.hpp"
#include <algorithm>
#include <utility>

#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qpair.h>
#include <qproperty.h>
#include <qscopeguard.h>
#include <qtenvironmentvariables.h>
#include <qthreadpool.h>
#include <qtmetamacros.h>
#include <ranges>

#include "../io/processcore.hpp"
#include "desktopentrymonitor.hpp"
#include "logcat.hpp"
#include "model.hpp"
#include "qmlglobal.hpp"

namespace {
QS_LOGGING_CATEGORY(logDesktopEntry, "quickshell.desktopentry", QtWarningMsg);
}

struct Locale {
	explicit Locale() = default;

	explicit Locale(const QString& string) {
		auto territoryIdx = string.indexOf('_');
		auto codesetIdx = string.indexOf('.');
		auto modifierIdx = string.indexOf('@');

		auto parseEnd = string.length();

		if (modifierIdx != -1) {
			this->modifier = string.sliced(modifierIdx + 1, parseEnd - modifierIdx - 1);
			parseEnd = modifierIdx;
		}

		if (codesetIdx != -1) {
			parseEnd = codesetIdx;
		}

		if (territoryIdx != -1) {
			this->territory = string.sliced(territoryIdx + 1, parseEnd - territoryIdx - 1);
			parseEnd = territoryIdx;
		}

		this->language = string.sliced(0, parseEnd);
	}

	[[nodiscard]] bool isValid() const { return !this->language.isEmpty(); }

	[[nodiscard]] int matchScore(const Locale& other) const {
		if (this->language != other.language) return 0;

		if (!other.modifier.isEmpty() && this->modifier != other.modifier) return 0;
		if (!other.territory.isEmpty() && this->territory != other.territory) return 0;

		auto score = 1;

		if (!other.territory.isEmpty()) score += 2;
		if (!other.modifier.isEmpty()) score += 1;

		return score;
	}

	static const Locale& system() {
		static Locale* locale = nullptr; // NOLINT

		if (locale == nullptr) {
			auto lstr = qEnvironmentVariable("LC_MESSAGES");
			if (lstr.isEmpty()) lstr = qEnvironmentVariable("LANG");
			locale = new Locale(lstr);
		}

		return *locale;
	}

	QString language;
	QString territory;
	QString modifier;
};

// NOLINTNEXTLINE(misc-use-internal-linkage)
QDebug operator<<(QDebug debug, const Locale& locale) {
	auto saver = QDebugStateSaver(debug);
	debug.nospace() << "Locale(language=" << locale.language << ", territory=" << locale.territory
	                << ", modifier=" << locale.modifier << ')';

	return debug;
}

ParsedDesktopEntryData DesktopEntry::parseText(const QString& id, const QString& text) {
	ParsedDesktopEntryData data;
	data.id = id;
	const auto& system = Locale::system();

	auto groupName = QString();
	auto entries = QHash<QString, QPair<Locale, QString>>();

	auto finishCategory = [&data, &groupName, &entries]() {
		if (groupName == "Desktop Entry") {
			if (entries.value("Type").second != "Application") return;

			for (const auto& [key, pair]: entries.asKeyValueRange()) {
				auto& [_, value] = pair;
				data.entries.insert(key, value);

				if (key == "Name") data.name = value;
				else if (key == "GenericName") data.genericName = value;
				else if (key == "StartupWMClass") data.startupClass = value;
				else if (key == "NoDisplay") data.noDisplay = value == "true";
				else if (key == "Hidden") data.hidden = value == "true";
				else if (key == "Comment") data.comment = value;
				else if (key == "Icon") data.icon = value;
				else if (key == "Exec") {
					data.execString = value;
					data.command = DesktopEntry::parseExecString(value);
				} else if (key == "Path") data.workingDirectory = value;
				else if (key == "Terminal") data.terminal = value == "true";
				else if (key == "Categories") data.categories = value.split(u';', Qt::SkipEmptyParts);
				else if (key == "Keywords") data.keywords = value.split(u';', Qt::SkipEmptyParts);
			}
		} else if (groupName.startsWith("Desktop Action ")) {
			auto actionName = groupName.sliced(16);
			DesktopActionData action;
			action.id = actionName;

			for (const auto& [key, pair]: entries.asKeyValueRange()) {
				const auto& [_, value] = pair;
				action.entries.insert(key, value);

				if (key == "Name") action.name = value;
				else if (key == "Icon") action.icon = value;
				else if (key == "Exec") {
					action.execString = value;
					action.command = DesktopEntry::parseExecString(value);
				}
			}

			data.actions.insert(actionName, action);
		}

		entries.clear();
	};

	for (auto& line: text.split(u'\n', Qt::SkipEmptyParts)) {
		if (line.startsWith(u'#')) continue;

		if (line.startsWith(u'[') && line.endsWith(u']')) {
			finishCategory();
			groupName = line.sliced(1, line.length() - 2);
			continue;
		}

		auto splitIdx = line.indexOf(u'=');
		if (splitIdx == -1) {
			qCWarning(logDesktopEntry) << "Encountered invalid line in desktop entry (no =)" << line;
			continue;
		}

		auto key = line.sliced(0, splitIdx);
		const auto& value = line.sliced(splitIdx + 1);

		auto localeIdx = key.indexOf('[');
		Locale locale;
		if (localeIdx != -1 && localeIdx != key.length() - 1) {
			locale = Locale(key.sliced(localeIdx + 1, key.length() - localeIdx - 2));
			key = key.sliced(0, localeIdx);
		}

		if (entries.contains(key)) {
			const auto& old = entries.value(key);

			auto oldScore = system.matchScore(old.first);
			auto newScore = system.matchScore(locale);

			if (newScore > oldScore || (oldScore == 0 && !locale.isValid())) {
				entries.insert(key, qMakePair(locale, value));
			}
		} else {
			entries.insert(key, qMakePair(locale, value));
		}
	}

	finishCategory();
	return data;
}

void DesktopEntry::updateState(const ParsedDesktopEntryData& newState) {
	Qt::beginPropertyUpdateGroup();
	this->bName = newState.name;
	this->bGenericName = newState.genericName;
	this->bStartupClass = newState.startupClass;
	this->bNoDisplay = newState.noDisplay;
	this->bComment = newState.comment;
	this->bIcon = newState.icon;
	this->bExecString = newState.execString;
	this->bCommand = newState.command;
	this->bWorkingDirectory = newState.workingDirectory;
	this->bRunInTerminal = newState.terminal;
	this->bCategories = newState.categories;
	this->bKeywords = newState.keywords;
	Qt::endPropertyUpdateGroup();

	this->state = newState;
	this->updateActions(newState.actions);
}

void DesktopEntry::updateActions(const QHash<QString, DesktopActionData>& newActions) {
	auto old = this->mActions;

	for (const auto& [key, d]: newActions.asKeyValueRange()) {
		DesktopAction* act = nullptr;
		if (auto found = old.find(key); found != old.end()) {
			act = found.value();
			old.erase(found);
		} else {
			act = new DesktopAction(d.id, this);
			this->mActions.insert(key, act);
		}

		Qt::beginPropertyUpdateGroup();
		act->bName = d.name;
		act->bIcon = d.icon;
		act->bExecString = d.execString;
		act->bCommand = d.command;
		Qt::endPropertyUpdateGroup();

		act->mEntries = d.entries;
	}

	for (auto* leftover: old) {
		leftover->deleteLater();
	}
}

void DesktopEntry::execute() const {
	DesktopEntry::doExec(this->bCommand.value(), this->bWorkingDirectory.value());
}

bool DesktopEntry::isValid() const { return !this->bName.value().isEmpty(); }

QVector<DesktopAction*> DesktopEntry::actions() const { return this->mActions.values(); }

QVector<QString> DesktopEntry::parseExecString(const QString& execString) {
	QVector<QString> arguments;
	QString currentArgument;
	auto parsingString = false;
	auto escape = 0;
	auto percent = false;

	for (auto c: execString) {
		if (escape == 0 && c == u'\\') {
			escape = 1;
		} else if (parsingString) {
			if (c == '\\') {
				escape++;
				if (escape == 4) {
					currentArgument += '\\';
					escape = 0;
				}
			} else if (escape == 2) {
				currentArgument += c;
				escape = 0;
			} else if (escape != 0) {
				switch (c.unicode()) {
				case 's': currentArgument += u' '; break;
				case 'n': currentArgument += u'\n'; break;
				case 't': currentArgument += u'\t'; break;
				case 'r': currentArgument += u'\r'; break;
				case '\\': currentArgument += u'\\'; break;
				default:
					qCWarning(logDesktopEntry).noquote()
					    << "Illegal escape sequence in desktop entry exec string:" << execString;
					currentArgument += c;
					break;
				}
				escape = 0;
			} else if (c == u'"' || c == u'\'') {
				parsingString = false;
			} else {
				currentArgument += c;
			}
		} else if (escape != 0) {
			currentArgument += c;
			escape = 0;
		} else if (percent) {
			if (c == '%') {
				currentArgument += '%';
			} // else discard

			percent = false;
		} else if (c == '%') {
			percent = true;
		} else if (c == u'"' || c == u'\'') {
			parsingString = true;
		} else if (c == u' ') {
			if (!currentArgument.isEmpty()) {
				arguments.push_back(currentArgument);
				currentArgument.clear();
			}
		} else {
			currentArgument += c;
		}
	}

	if (!currentArgument.isEmpty()) {
		arguments.push_back(currentArgument);
		currentArgument.clear();
	}

	return arguments;
}

void DesktopEntry::doExec(const QList<QString>& execString, const QString& workingDirectory) {
	qs::io::process::ProcessContext ctx;
	ctx.setCommand(execString);
	ctx.setWorkingDirectory(workingDirectory);
	QuickshellGlobal::execDetached(ctx);
}

void DesktopAction::execute() const {
	DesktopEntry::doExec(this->bCommand.value(), this->entry->bWorkingDirectory.value());
}

DesktopEntryScanner::DesktopEntryScanner(DesktopEntryManager* manager): manager(manager) {
	this->setAutoDelete(true);
}

void DesktopEntryScanner::run() {
	const auto& desktopPaths = DesktopEntryManager::desktopPaths();
	auto scanResults = QList<ParsedDesktopEntryData>();

	for (const auto& path: desktopPaths | std::views::reverse) {
		auto file = QFileInfo(path);
		if (!file.isDir()) continue;

		this->scanDirectory(QDir(path), QString(), scanResults);
	}

	QMetaObject::invokeMethod(
	    this->manager,
	    "onScanCompleted",
	    Qt::QueuedConnection,
	    Q_ARG(QList<ParsedDesktopEntryData>, scanResults)
	);
}

void DesktopEntryScanner::scanDirectory(
    const QDir& dir,
    const QString& idPrefix,
    QList<ParsedDesktopEntryData>& entries
) {
	auto dirEntries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

	for (auto& entry: dirEntries) {
		if (entry.isDir()) {
			auto subdirPrefix = idPrefix.isEmpty() ? entry.fileName() : idPrefix + '-' + entry.fileName();
			this->scanDirectory(QDir(entry.absoluteFilePath()), subdirPrefix, entries);
		} else if (entry.isFile()) {
			auto path = entry.filePath();
			if (!path.endsWith(".desktop")) {
				qCDebug(logDesktopEntry) << "Skipping file" << path << "as it has no .desktop extension";
				continue;
			}

			auto file = QFile(path);
			if (!file.open(QFile::ReadOnly)) {
				qCDebug(logDesktopEntry) << "Could not open file" << path;
				continue;
			}

			auto basename = QFileInfo(entry.fileName()).completeBaseName();
			auto id = idPrefix.isEmpty() ? basename : idPrefix + '-' + basename;
			auto content = QString::fromUtf8(file.readAll());

			auto data = DesktopEntry::parseText(id, content);
			entries.append(std::move(data));
		}
	}
}

DesktopEntryManager::DesktopEntryManager(): monitor(new DesktopEntryMonitor(this)) {
	QObject::connect(
	    this->monitor,
	    &DesktopEntryMonitor::desktopEntriesChanged,
	    this,
	    &DesktopEntryManager::handleFileChanges
	);

	DesktopEntryScanner(this).run();
}

void DesktopEntryManager::scanDesktopEntries() {
	qCDebug(logDesktopEntry) << "Starting desktop entry scan";

	if (this->scanInProgress) {
		qCDebug(logDesktopEntry) << "Scan already in progress, queuing another scan";
		this->scanQueued = true;
		return;
	}

	this->scanInProgress = true;
	this->scanQueued = false;
	auto* scanner = new DesktopEntryScanner(this);
	QThreadPool::globalInstance()->start(scanner);
}

DesktopEntryManager* DesktopEntryManager::instance() {
	static auto* instance = new DesktopEntryManager(); // NOLINT
	return instance;
}

DesktopEntry* DesktopEntryManager::byId(const QString& id) {
	if (auto* entry = this->desktopEntries.value(id)) {
		return entry;
	} else if (auto* entry = this->lowercaseDesktopEntries.value(id.toLower())) {
		return entry;
	} else {
		return nullptr;
	}
}

DesktopEntry* DesktopEntryManager::heuristicLookup(const QString& name) {
	if (auto* entry = this->byId(name)) return entry;

	auto list = this->desktopEntries.values();

	auto iter = std::ranges::find_if(list, [&](DesktopEntry* entry) {
		return name == entry->bStartupClass.value();
	});

	if (iter != list.end()) return *iter;

	iter = std::ranges::find_if(list, [&](DesktopEntry* entry) {
		return name.toLower() == entry->bStartupClass.value().toLower();
	});

	if (iter != list.end()) return *iter;
	return nullptr;
}

ObjectModel<DesktopEntry>* DesktopEntryManager::applications() { return &this->mApplications; }

void DesktopEntryManager::handleFileChanges() {
	qCDebug(logDesktopEntry) << "Directory change detected, performing full rescan";

	if (this->scanInProgress) {
		qCDebug(logDesktopEntry) << "Scan already in progress, queuing another scan";
		this->scanQueued = true;
		return;
	}

	this->scanInProgress = true;
	this->scanQueued = false;
	auto* scanner = new DesktopEntryScanner(this);
	QThreadPool::globalInstance()->start(scanner);
}

const QStringList& DesktopEntryManager::desktopPaths() {
	static const auto paths = []() {
		auto dataPaths = QStringList();

		auto dataHome = qEnvironmentVariable("XDG_DATA_HOME");
		if (dataHome.isEmpty() && qEnvironmentVariableIsSet("HOME"))
			dataHome = qEnvironmentVariable("HOME") + "/.local/share";
		if (!dataHome.isEmpty()) dataPaths.append(dataHome + "/applications");

		auto dataDirs = qEnvironmentVariable("XDG_DATA_DIRS");
		if (dataDirs.isEmpty()) dataDirs = "/usr/local/share:/usr/share";

		for (const auto& dir: dataDirs.split(':', Qt::SkipEmptyParts)) {
			dataPaths.append(dir + "/applications");
		}

		return dataPaths;
	}();

	return paths;
}

void DesktopEntryManager::onScanCompleted(const QList<ParsedDesktopEntryData>& scanResults) {
	auto guard = qScopeGuard([this] {
		this->scanInProgress = false;
		if (this->scanQueued) {
			this->scanQueued = false;
			this->scanDesktopEntries();
		}
	});

	auto oldEntries = this->desktopEntries;
	auto newEntries = QHash<QString, DesktopEntry*>();
	auto newLowercaseEntries = QHash<QString, DesktopEntry*>();

	for (const auto& data: scanResults) {
		auto lowerId = data.id.toLower();

		if (data.hidden) {
			if (auto* victim = newEntries.take(data.id)) victim->deleteLater();
			newLowercaseEntries.remove(lowerId);

			if (auto it = oldEntries.find(data.id); it != oldEntries.end()) {
				it.value()->deleteLater();
				oldEntries.erase(it);
			}

			qCDebug(logDesktopEntry) << "Masking hidden desktop entry" << data.id;
			continue;
		}

		DesktopEntry* dentry = nullptr;

		if (auto it = oldEntries.find(data.id); it != oldEntries.end()) {
			dentry = it.value();
			oldEntries.erase(it);
			dentry->updateState(data);
		} else {
			dentry = new DesktopEntry(data.id, this);
			dentry->updateState(data);
		}

		if (!dentry->isValid()) {
			qCDebug(logDesktopEntry) << "Skipping desktop entry" << data.id;
			if (!oldEntries.contains(data.id)) {
				dentry->deleteLater();
			}
			continue;
		}

		qCDebug(logDesktopEntry) << "Found desktop entry" << data.id;

		auto conflictingId = newEntries.contains(data.id);

		if (conflictingId) {
			qCDebug(logDesktopEntry) << "Replacing old entry for" << data.id;
			if (auto* victim = newEntries.take(data.id)) victim->deleteLater();
			newLowercaseEntries.remove(lowerId);
		}

		newEntries.insert(data.id, dentry);

		if (newLowercaseEntries.contains(lowerId)) {
			qCInfo(logDesktopEntry).nospace()
			    << "Multiple desktop entries have the same lowercased id " << lowerId
			    << ". This can cause ambiguity when byId requests are not made with the correct case "
			       "already.";

			newLowercaseEntries.remove(lowerId);
		}

		newLowercaseEntries.insert(lowerId, dentry);
	}

	this->desktopEntries = newEntries;
	this->lowercaseDesktopEntries = newLowercaseEntries;

	auto newApplications = QVector<DesktopEntry*>();
	for (auto* entry: this->desktopEntries.values())
		if (!entry->bNoDisplay) newApplications.append(entry);

	this->mApplications.diffUpdate(newApplications);

	emit this->applicationsChanged();

	for (auto* e: oldEntries) e->deleteLater();
}

DesktopEntries::DesktopEntries() {
	QObject::connect(
	    DesktopEntryManager::instance(),
	    &DesktopEntryManager::applicationsChanged,
	    this,
	    &DesktopEntries::applicationsChanged
	);
}

DesktopEntry* DesktopEntries::byId(const QString& id) {
	return DesktopEntryManager::instance()->byId(id);
}

DesktopEntry* DesktopEntries::heuristicLookup(const QString& name) {
	return DesktopEntryManager::instance()->heuristicLookup(name);
}

ObjectModel<DesktopEntry>* DesktopEntries::applications() {
	return DesktopEntryManager::instance()->applications();
}
