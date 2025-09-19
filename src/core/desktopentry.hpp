#pragma once

#include <utility>

#include <qcontainerfwd.h>
#include <qdir.h>
#include <qhash.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qrunnable.h>
#include <qtmetamacros.h>

#include "desktopentrymonitor.hpp"
#include "doc.hpp"
#include "model.hpp"

class DesktopAction;
class DesktopEntryMonitor;

struct DesktopActionData {
	QString id;
	QString name;
	QString icon;
	QString execString;
	QVector<QString> command;
	QHash<QString, QString> entries;
};

struct ParsedDesktopEntryData {
	QString id;
	QString name;
	QString genericName;
	QString startupClass;
	bool noDisplay = false;
	bool hidden = false;
	QString comment;
	QString icon;
	QString execString;
	QVector<QString> command;
	QString workingDirectory;
	bool terminal = false;
	QVector<QString> categories;
	QVector<QString> keywords;
	QHash<QString, QString> entries;
	QHash<QString, DesktopActionData> actions;
};

/// A desktop entry. See @@DesktopEntries for details.
class DesktopEntry: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QString id MEMBER mId CONSTANT);
	/// Name of the specific application, such as "Firefox".
	// clang-format off
	Q_PROPERTY(QString name READ default WRITE default NOTIFY nameChanged BINDABLE bindableName);
	/// Short description of the application, such as "Web Browser". May be empty.
	Q_PROPERTY(QString genericName READ default WRITE default NOTIFY genericNameChanged BINDABLE bindableGenericName);
	/// Initial class or app id the app intends to use. May be useful for matching running apps
	/// to desktop entries.
	Q_PROPERTY(QString startupClass READ default WRITE default NOTIFY startupClassChanged BINDABLE bindableStartupClass);
	/// If true, this application should not be displayed in menus and launchers.
	Q_PROPERTY(bool noDisplay READ default WRITE default NOTIFY noDisplayChanged BINDABLE bindableNoDisplay);
	/// Long description of the application, such as "View websites on the internet". May be empty.
	Q_PROPERTY(QString comment READ default WRITE default NOTIFY commentChanged BINDABLE bindableComment);
	/// Name of the icon associated with this application. May be empty.
	Q_PROPERTY(QString icon READ default WRITE default NOTIFY iconChanged BINDABLE bindableIcon);
	/// The raw `Exec` string from the desktop entry.
	///
	/// > [!WARNING] This cannot be reliably run as a command. See @@command for one you can run.
	Q_PROPERTY(QString execString READ default WRITE default NOTIFY execStringChanged BINDABLE bindableExecString);
	/// The parsed `Exec` command in the desktop entry.
	///
	/// The entry can be run with @@execute(), or by using this command in
	/// @@Quickshell.Quickshell.execDetached() or @@Quickshell.Io.Process.
	/// If used in `execDetached` or a `Process`, @@workingDirectory should also be passed to
	/// the invoked process. See @@execute() for details.
	///
	/// > [!NOTE]	The provided command does not invoke a terminal even if @@runInTerminal is true.
	Q_PROPERTY(QVector<QString> command READ default WRITE default NOTIFY commandChanged BINDABLE bindableCommand);
	/// The working directory to execute from.
	Q_PROPERTY(QString workingDirectory READ default WRITE default NOTIFY workingDirectoryChanged BINDABLE bindableWorkingDirectory);
	/// If the application should run in a terminal.
	Q_PROPERTY(bool runInTerminal READ default WRITE default NOTIFY runInTerminalChanged BINDABLE bindableRunInTerminal);
	Q_PROPERTY(QVector<QString> categories READ default WRITE default NOTIFY categoriesChanged BINDABLE bindableCategories);
	Q_PROPERTY(QVector<QString> keywords READ default WRITE default NOTIFY keywordsChanged BINDABLE bindableKeywords);
	// clang-format on
	Q_PROPERTY(QVector<DesktopAction*> actions READ actions CONSTANT);
	QML_ELEMENT;
	QML_UNCREATABLE("DesktopEntry instances must be retrieved from DesktopEntries");

public:
	explicit DesktopEntry(QString id, QObject* parent): QObject(parent), mId(std::move(id)) {}

	static ParsedDesktopEntryData parseText(const QString& id, const QString& text);
	void updateState(const ParsedDesktopEntryData& newState);

	/// Run the application. Currently ignores @@runInTerminal and field codes.
	///
	/// This is equivalent to calling @@Quickshell.Quickshell.execDetached() with @@command
	/// and @@DesktopEntry.workingDirectory as shown below:
	///
	/// ```qml
	/// Quickshell.execDetached({
	///   command: desktopEntry.command,
	///   workingDirectory: desktopEntry.workingDirectory,
	/// });
	/// ```
	Q_INVOKABLE void execute() const;

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] QVector<DesktopAction*> actions() const;

	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; }
	[[nodiscard]] QBindable<QString> bindableGenericName() const { return &this->bGenericName; }
	[[nodiscard]] QBindable<QString> bindableStartupClass() const { return &this->bStartupClass; }
	[[nodiscard]] QBindable<bool> bindableNoDisplay() const { return &this->bNoDisplay; }
	[[nodiscard]] QBindable<QString> bindableComment() const { return &this->bComment; }
	[[nodiscard]] QBindable<QString> bindableIcon() const { return &this->bIcon; }
	[[nodiscard]] QBindable<QString> bindableExecString() const { return &this->bExecString; }
	[[nodiscard]] QBindable<QVector<QString>> bindableCommand() const { return &this->bCommand; }
	[[nodiscard]] QBindable<QString> bindableWorkingDirectory() const {
		return &this->bWorkingDirectory;
	}
	[[nodiscard]] QBindable<bool> bindableRunInTerminal() const { return &this->bRunInTerminal; }
	[[nodiscard]] QBindable<QVector<QString>> bindableCategories() const {
		return &this->bCategories;
	}
	[[nodiscard]] QBindable<QVector<QString>> bindableKeywords() const { return &this->bKeywords; }

	// currently ignores all field codes.
	static QVector<QString> parseExecString(const QString& execString);
	static void doExec(const QList<QString>& execString, const QString& workingDirectory);

signals:
	void nameChanged();
	void genericNameChanged();
	void startupClassChanged();
	void noDisplayChanged();
	void commentChanged();
	void iconChanged();
	void execStringChanged();
	void commandChanged();
	void workingDirectoryChanged();
	void runInTerminalChanged();
	void categoriesChanged();
	void keywordsChanged();

public:
	QString mId;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, QString, bName, &DesktopEntry::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, QString, bGenericName, &DesktopEntry::genericNameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, QString, bStartupClass, &DesktopEntry::startupClassChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, bool, bNoDisplay, &DesktopEntry::noDisplayChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, QString, bComment, &DesktopEntry::commentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, QString, bIcon, &DesktopEntry::iconChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, QString, bExecString, &DesktopEntry::execStringChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, QVector<QString>, bCommand, &DesktopEntry::commandChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, QString, bWorkingDirectory, &DesktopEntry::workingDirectoryChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, bool, bRunInTerminal, &DesktopEntry::runInTerminalChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, QVector<QString>, bCategories, &DesktopEntry::categoriesChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopEntry, QVector<QString>, bKeywords, &DesktopEntry::keywordsChanged);
	// clang-format on

private:
	void updateActions(const QHash<QString, DesktopActionData>& newActions);

	ParsedDesktopEntryData state;
	QHash<QString, DesktopAction*> mActions;

	friend class DesktopAction;
};

/// An action of a @@DesktopEntry$.
class DesktopAction: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QString id MEMBER mId CONSTANT);
	// clang-format off
	Q_PROPERTY(QString name READ default WRITE default NOTIFY nameChanged BINDABLE bindableName);
	Q_PROPERTY(QString icon READ default WRITE default NOTIFY iconChanged BINDABLE bindableIcon);
	/// The raw `Exec` string from the action.
	///
	/// > [!WARNING] This cannot be reliably run as a command. See @@command for one you can run.
	Q_PROPERTY(QString execString READ default WRITE default NOTIFY execStringChanged BINDABLE bindableExecString);
	/// The parsed `Exec` command in the action.
	///
	/// The entry can be run with @@execute(), or by using this command in
	/// @@Quickshell.Quickshell.execDetached() or @@Quickshell.Io.Process.
	/// If used in `execDetached` or a `Process`, @@DesktopEntry.workingDirectory should also be passed to
	/// the invoked process.
	///
	/// > [!NOTE]	The provided command does not invoke a terminal even if @@runInTerminal is true.
	Q_PROPERTY(QVector<QString> command READ default WRITE default NOTIFY commandChanged BINDABLE bindableCommand);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("DesktopAction instances must be retrieved from a DesktopEntry");

public:
	explicit DesktopAction(QString id, DesktopEntry* entry)
	    : QObject(entry)
	    , entry(entry)
	    , mId(std::move(id)) {}

	/// Run the application. Currently ignores @@DesktopEntry.runInTerminal and field codes.
	///
	/// This is equivalent to calling @@Quickshell.Quickshell.execDetached() with @@command
	/// and @@DesktopEntry.workingDirectory.
	Q_INVOKABLE void execute() const;

	[[nodiscard]] QBindable<QString> bindableName() const { return &this->bName; }
	[[nodiscard]] QBindable<QString> bindableIcon() const { return &this->bIcon; }
	[[nodiscard]] QBindable<QString> bindableExecString() const { return &this->bExecString; }
	[[nodiscard]] QBindable<QVector<QString>> bindableCommand() const { return &this->bCommand; }

signals:
	void nameChanged();
	void iconChanged();
	void execStringChanged();
	void commandChanged();

private:
	DesktopEntry* entry;
	QString mId;
	QHash<QString, QString> mEntries;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(DesktopAction, QString, bName, &DesktopAction::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopAction, QString, bIcon, &DesktopAction::iconChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopAction, QString, bExecString, &DesktopAction::execStringChanged);
	Q_OBJECT_BINDABLE_PROPERTY(DesktopAction, QVector<QString>, bCommand, &DesktopAction::commandChanged);
	// clang-format on

	friend class DesktopEntry;
};

class DesktopEntryManager;

class DesktopEntryScanner: public QRunnable {
public:
	explicit DesktopEntryScanner(DesktopEntryManager* manager);

	void run() override;
	// clang-format off
	void scanDirectory(const QDir& dir, const QString& idPrefix, QList<ParsedDesktopEntryData>& entries);
	// clang-format on

private:
	DesktopEntryManager* manager;
};

class DesktopEntryManager: public QObject {
	Q_OBJECT;

public:
	void scanDesktopEntries();

	[[nodiscard]] DesktopEntry* byId(const QString& id);
	[[nodiscard]] DesktopEntry* heuristicLookup(const QString& name);

	[[nodiscard]] ObjectModel<DesktopEntry>* applications();

	static DesktopEntryManager* instance();

	static const QStringList& desktopPaths();

signals:
	void applicationsChanged();

private slots:
	void handleFileChanges();
	void onScanCompleted(const QList<ParsedDesktopEntryData>& scanResults);

private:
	explicit DesktopEntryManager();

	QHash<QString, DesktopEntry*> desktopEntries;
	QHash<QString, DesktopEntry*> lowercaseDesktopEntries;
	ObjectModel<DesktopEntry> mApplications {this};
	DesktopEntryMonitor* monitor = nullptr;
	bool scanInProgress = false;
	bool scanQueued = false;

	friend class DesktopEntryScanner;
};

///! Desktop entry index.
/// Index of desktop entries according to the [desktop entry specification].
///
/// Primarily useful for looking up icons and metadata from an id, as there is
/// currently no mechanism for usage based sorting of entries and other launcher niceties.
///
/// [desktop entry specification]: https://specifications.freedesktop.org/desktop-entry-spec/latest/
class DesktopEntries: public QObject {
	Q_OBJECT;
	/// All desktop entries of type Application that are not Hidden or NoDisplay.
	QSDOC_TYPE_OVERRIDE(ObjectModel<DesktopEntry>*);
	Q_PROPERTY(UntypedObjectModel* applications READ applications CONSTANT);
	QML_ELEMENT;
	QML_SINGLETON;

public:
	explicit DesktopEntries();

	/// Look up a desktop entry by name. Includes NoDisplay entries. May return null.
	///
	/// While this function requires an exact match, @@heuristicLookup() will correctly
	/// find an entry more often and is generally more useful.
	Q_INVOKABLE [[nodiscard]] static DesktopEntry* byId(const QString& id);
	/// Look up a desktop entry by name using heuristics. Unlike @@byId(),
	/// if no exact matches are found this function will try to guess - potentially incorrectly.
	/// May return null.
	Q_INVOKABLE [[nodiscard]] static DesktopEntry* heuristicLookup(const QString& name);

	[[nodiscard]] static ObjectModel<DesktopEntry>* applications();

signals:
	void applicationsChanged();
};
