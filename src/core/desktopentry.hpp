#pragma once

#include <utility>

#include <qcontainerfwd.h>
#include <qdir.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "doc.hpp"
#include "model.hpp"

class DesktopAction;

/// A desktop entry. See @@DesktopEntries for details.
class DesktopEntry: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QString id MEMBER mId CONSTANT);
	/// Name of the specific application, such as "Firefox".
	Q_PROPERTY(QString name MEMBER mName CONSTANT);
	/// Short description of the application, such as "Web Browser". May be empty.
	Q_PROPERTY(QString genericName MEMBER mGenericName CONSTANT);
	/// Initial class or app id the app intends to use. May be useful for matching running apps
	/// to desktop entries.
	Q_PROPERTY(QString startupClass MEMBER mStartupClass CONSTANT);
	/// If true, this application should not be displayed in menus and launchers.
	Q_PROPERTY(bool noDisplay MEMBER mNoDisplay CONSTANT);
	/// Long description of the application, such as "View websites on the internet". May be empty.
	Q_PROPERTY(QString comment MEMBER mComment CONSTANT);
	/// Name of the icon associated with this application. May be empty.
	Q_PROPERTY(QString icon MEMBER mIcon CONSTANT);
	/// The raw `Exec` string from the desktop entry.
	///
	/// > [!WARNING] This cannot be reliably run as a command. See @@command for one you can run.
	Q_PROPERTY(QString execString MEMBER mExecString CONSTANT);
	/// The parsed `Exec` command in the desktop entry.
	///
	/// The entry can be run with @@execute(), or by using this command in
	/// @@Quickshell.Quickshell.execDetached() or @@Quickshell.Io.Process.
	/// If used in `execDetached` or a `Process`, @@workingDirectory should also be passed to
	/// the invoked process. See @@execute() for details.
	///
	/// > [!NOTE]	The provided command does not invoke a terminal even if @@runInTerminal is true.
	Q_PROPERTY(QVector<QString> command MEMBER mCommand CONSTANT);
	/// The working directory to execute from.
	Q_PROPERTY(QString workingDirectory MEMBER mWorkingDirectory CONSTANT);
	/// If the application should run in a terminal.
	Q_PROPERTY(bool runInTerminal MEMBER mTerminal CONSTANT);
	Q_PROPERTY(QVector<QString> categories MEMBER mCategories CONSTANT);
	Q_PROPERTY(QVector<QString> keywords MEMBER mKeywords CONSTANT);
	Q_PROPERTY(QVector<DesktopAction*> actions READ actions CONSTANT);
	QML_ELEMENT;
	QML_UNCREATABLE("DesktopEntry instances must be retrieved from DesktopEntries");

public:
	explicit DesktopEntry(QString id, QObject* parent): QObject(parent), mId(std::move(id)) {}

	void parseEntry(const QString& text);

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
	[[nodiscard]] bool noDisplay() const;
	[[nodiscard]] QVector<DesktopAction*> actions() const;

	// currently ignores all field codes.
	static QVector<QString> parseExecString(const QString& execString);
	static void doExec(const QList<QString>& execString, const QString& workingDirectory);

public:
	QString mId;
	QString mName;
	QString mGenericName;
	QString mStartupClass;
	bool mNoDisplay = false;
	QString mComment;
	QString mIcon;
	QString mExecString;
	QVector<QString> mCommand;
	QString mWorkingDirectory;
	bool mTerminal = false;
	QVector<QString> mCategories;
	QVector<QString> mKeywords;

private:
	QHash<QString, QString> mEntries;
	QHash<QString, DesktopAction*> mActions;

	friend class DesktopAction;
};

/// An action of a @@DesktopEntry$.
class DesktopAction: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QString id MEMBER mId CONSTANT);
	Q_PROPERTY(QString name MEMBER mName CONSTANT);
	Q_PROPERTY(QString icon MEMBER mIcon CONSTANT);
	/// The raw `Exec` string from the action.
	///
	/// > [!WARNING] This cannot be reliably run as a command. See @@command for one you can run.
	Q_PROPERTY(QString execString MEMBER mExecString CONSTANT);
	/// The parsed `Exec` command in the action.
	///
	/// The entry can be run with @@execute(), or by using this command in
	/// @@Quickshell.Quickshell.execDetached() or @@Quickshell.Io.Process.
	/// If used in `execDetached` or a `Process`, @@DesktopEntry.workingDirectory should also be passed to
	/// the invoked process.
	///
	/// > [!NOTE]	The provided command does not invoke a terminal even if @@runInTerminal is true.
	Q_PROPERTY(QVector<QString> command MEMBER mCommand CONSTANT);
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

private:
	DesktopEntry* entry;
	QString mId;
	QString mName;
	QString mIcon;
	QString mExecString;
	QVector<QString> mCommand;
	QHash<QString, QString> mEntries;

	friend class DesktopEntry;
};

class DesktopEntryManager: public QObject {
	Q_OBJECT;

public:
	void scanDesktopEntries();

	[[nodiscard]] DesktopEntry* byId(const QString& id);
	[[nodiscard]] DesktopEntry* heuristicLookup(const QString& name);

	[[nodiscard]] ObjectModel<DesktopEntry>* applications();

	static DesktopEntryManager* instance();

private:
	explicit DesktopEntryManager();

	void populateApplications();
	void scanPath(const QDir& dir, const QString& prefix = QString());

	QHash<QString, DesktopEntry*> desktopEntries;
	QHash<QString, DesktopEntry*> lowercaseDesktopEntries;
	ObjectModel<DesktopEntry> mApplications {this};
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
};
