#include "desktopentry.hpp"
#include <algorithm>

#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qhash.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpair.h>
#include <qstringview.h>
#include <qtenvironmentvariables.h>
#include <ranges>

#include "../io/processcore.hpp"
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
		auto territoryMatches = !this->territory.isEmpty() && this->territory == other.territory;
		auto modifierMatches = !this->modifier.isEmpty() && this->modifier == other.modifier;

		auto score = 1;
		if (territoryMatches) score += 2;
		if (modifierMatches) score += 1;

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
	                << ", modifier" << locale.modifier << ')';

	return debug;
}

void DesktopEntry::parseEntry(const QString& text) {
	const auto& system = Locale::system();

	auto groupName = QString();
	auto entries = QHash<QString, QPair<Locale, QString>>();

	auto finishCategory = [this, &groupName, &entries]() {
		if (groupName == "Desktop Entry") {
			if (entries["Type"].second != "Application") return;
			if (entries.contains("Hidden") && entries["Hidden"].second == "true") return;

			for (const auto& [key, pair]: entries.asKeyValueRange()) {
				auto& [_, value] = pair;
				this->mEntries.insert(key, value);

				if (key == "Name") this->mName = value;
				else if (key == "GenericName") this->mGenericName = value;
				else if (key == "StartupWMClass") this->mStartupClass = value;
				else if (key == "NoDisplay") this->mNoDisplay = value == "true";
				else if (key == "Comment") this->mComment = value;
				else if (key == "Icon") this->mIcon = value;
				else if (key == "Exec") {
					this->mExecString = value;
					this->mCommand = DesktopEntry::parseExecString(value);
				} else if (key == "Path") this->mWorkingDirectory = value;
				else if (key == "Terminal") this->mTerminal = value == "true";
				else if (key == "Categories") this->mCategories = value.split(u';', Qt::SkipEmptyParts);
				else if (key == "Keywords") this->mKeywords = value.split(u';', Qt::SkipEmptyParts);
			}
		} else if (groupName.startsWith("Desktop Action ")) {
			auto actionName = groupName.sliced(16);
			auto* action = new DesktopAction(actionName, this);

			for (const auto& [key, pair]: entries.asKeyValueRange()) {
				const auto& [_, value] = pair;
				action->mEntries.insert(key, value);

				if (key == "Name") action->mName = value;
				else if (key == "Icon") action->mIcon = value;
				else if (key == "Exec") {
					action->mExecString = value;
					action->mCommand = DesktopEntry::parseExecString(value);
				}
			}

			this->mActions.insert(actionName, action);
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
}

void DesktopEntry::execute() const {
	DesktopEntry::doExec(this->mCommand, this->mWorkingDirectory);
}

bool DesktopEntry::isValid() const { return !this->mName.isEmpty(); }
bool DesktopEntry::noDisplay() const { return this->mNoDisplay; }

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
			} else if (escape != 0) {
				if (escape != 2) {
					// Technically this is an illegal state, but the spec has a terrible double escape
					// rule in strings for no discernable reason. Assuming someone might understandably
					// misunderstand it, treat it as a normal escape and log it.
					qCWarning(logDesktopEntry).noquote()
					    << "Illegal escape sequence in desktop entry exec string:" << execString;
				}

				currentArgument += c;
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
	DesktopEntry::doExec(this->mCommand, this->entry->mWorkingDirectory);
}

DesktopEntryManager::DesktopEntryManager() {
	this->scanDesktopEntries();
	this->populateApplications();
}

void DesktopEntryManager::scanDesktopEntries() {
	QList<QString> dataPaths;

	if (qEnvironmentVariableIsSet("XDG_DATA_HOME")) {
		dataPaths.push_back(qEnvironmentVariable("XDG_DATA_HOME"));
	} else if (qEnvironmentVariableIsSet("HOME")) {
		dataPaths.push_back(qEnvironmentVariable("HOME") + "/.local/share");
	}

	if (qEnvironmentVariableIsSet("XDG_DATA_DIRS")) {
		auto var = qEnvironmentVariable("XDG_DATA_DIRS");
		dataPaths += var.split(u':', Qt::SkipEmptyParts);
	} else {
		dataPaths.push_back("/usr/local/share");
		dataPaths.push_back("/usr/share");
	}

	qCDebug(logDesktopEntry) << "Creating desktop entry scanners";

	for (auto& path: std::ranges::reverse_view(dataPaths)) {
		auto p = QDir(path).filePath("applications");
		auto file = QFileInfo(p);

		if (!file.isDir()) {
			qCDebug(logDesktopEntry) << "Not scanning path" << p << "as it is not a directory";
			continue;
		}

		qCDebug(logDesktopEntry) << "Scanning path" << p;
		this->scanPath(p);
	}
}

void DesktopEntryManager::populateApplications() {
	for (auto& entry: this->desktopEntries.values()) {
		if (!entry->noDisplay()) this->mApplications.insertObject(entry);
	}
}

void DesktopEntryManager::scanPath(const QDir& dir, const QString& prefix) {
	auto entries = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

	for (auto& entry: entries) {
		if (entry.isDir()) this->scanPath(entry.absoluteFilePath(), prefix + dir.dirName() + "-");
		else if (entry.isFile()) {
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

			auto id = prefix + entry.fileName().sliced(0, entry.fileName().length() - 8);
			auto lowerId = id.toLower();

			auto text = QString::fromUtf8(file.readAll());
			auto* dentry = new DesktopEntry(id, this);
			dentry->parseEntry(text);

			if (!dentry->isValid()) {
				qCDebug(logDesktopEntry) << "Skipping desktop entry" << path;
				delete dentry;
				continue;
			}

			qCDebug(logDesktopEntry) << "Found desktop entry" << id << "at" << path;

			auto conflictingId = this->desktopEntries.contains(id);

			if (conflictingId) {
				qCDebug(logDesktopEntry) << "Replacing old entry for" << id;
				delete this->desktopEntries.value(id);
				this->desktopEntries.remove(id);
				this->lowercaseDesktopEntries.remove(lowerId);
			}

			this->desktopEntries.insert(id, dentry);

			if (this->lowercaseDesktopEntries.contains(lowerId)) {
				qCInfo(logDesktopEntry).nospace()
				    << "Multiple desktop entries have the same lowercased id " << lowerId
				    << ". This can cause ambiguity when byId requests are not made with the correct case "
				       "already.";

				this->lowercaseDesktopEntries.remove(lowerId);
			}

			this->lowercaseDesktopEntries.insert(lowerId, dentry);
		}
	}
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

	auto iter = std::ranges::find_if(list, [&](const DesktopEntry* entry) {
		return name == entry->mStartupClass;
	});

	if (iter != list.end()) return *iter;

	iter = std::ranges::find_if(list, [&](const DesktopEntry* entry) {
		return name.toLower() == entry->mStartupClass.toLower();
	});

	if (iter != list.end()) return *iter;
	return nullptr;
}

ObjectModel<DesktopEntry>* DesktopEntryManager::applications() { return &this->mApplications; }

DesktopEntries::DesktopEntries() { DesktopEntryManager::instance(); }

DesktopEntry* DesktopEntries::byId(const QString& id) {
	return DesktopEntryManager::instance()->byId(id);
}

DesktopEntry* DesktopEntries::heuristicLookup(const QString& name) {
	return DesktopEntryManager::instance()->heuristicLookup(name);
}

ObjectModel<DesktopEntry>* DesktopEntries::applications() {
	return DesktopEntryManager::instance()->applications();
}
