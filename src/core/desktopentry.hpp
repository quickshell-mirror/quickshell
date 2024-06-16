#pragma once

#include <utility>

#include <qcontainerfwd.h>
#include <qdir.h>
#include <qhash.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "model.hpp"

class DesktopAction;

/// A desktop entry. See [DesktopEntries](../desktopentries) for details.
class DesktopEntry: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QString id MEMBER mId CONSTANT);
	/// Name of the specific application, such as "Firefox".
	Q_PROPERTY(QString name MEMBER mName CONSTANT);
	/// Short description of the application, such as "Web Browser". May be empty.
	Q_PROPERTY(QString genericName MEMBER mGenericName CONSTANT);
	/// If true, this application should not be displayed in menus and launchers.
	Q_PROPERTY(bool noDisplay MEMBER mNoDisplay CONSTANT);
	/// Long description of the application, such as "View websites on the internet". May be empty.
	Q_PROPERTY(QString comment MEMBER mComment CONSTANT);
	/// Name of the icon associated with this application. May be empty.
	Q_PROPERTY(QString icon MEMBER mIcon CONSTANT);
	/// The raw `Exec` string from the desktop entry. You probably want `execute()`.
	Q_PROPERTY(QString execString MEMBER mExecString CONSTANT);
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

	/// Run the application. Currently ignores `runInTerminal` and field codes.
	Q_INVOKABLE void execute() const;

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] bool noDisplay() const;
	[[nodiscard]] QVector<DesktopAction*> actions() const;

	// currently ignores all field codes.
	static QVector<QString> parseExecString(const QString& execString);
	static void doExec(const QString& execString, const QString& workingDirectory);

private:
	QHash<QString, QString> mEntries;
	QHash<QString, DesktopAction*> mActions;

	QString mId;
	QString mName;
	QString mGenericName;
	bool mNoDisplay = false;
	QString mComment;
	QString mIcon;
	QString mExecString;
	QString mWorkingDirectory;
	bool mTerminal = false;
	QVector<QString> mCategories;
	QVector<QString> mKeywords;

	friend class DesktopAction;
};

/// An action of a [DesktopEntry](../desktopentry).
class DesktopAction: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QString id MEMBER mId CONSTANT);
	Q_PROPERTY(QString name MEMBER mName CONSTANT);
	Q_PROPERTY(QString icon MEMBER mIcon CONSTANT);
	/// The raw `Exec` string from the desktop entry. You probably want `execute()`.
	Q_PROPERTY(QString execString MEMBER mExecString CONSTANT);
	QML_ELEMENT;
	QML_UNCREATABLE("DesktopAction instances must be retrieved from a DesktopEntry");

public:
	explicit DesktopAction(QString id, DesktopEntry* entry)
	    : QObject(entry)
	    , entry(entry)
	    , mId(std::move(id)) {}

	/// Run the application. Currently ignores `runInTerminal` and field codes.
	Q_INVOKABLE void execute() const;

private:
	DesktopEntry* entry;
	QString mId;
	QString mName;
	QString mIcon;
	QString mExecString;
	QHash<QString, QString> mEntries;

	friend class DesktopEntry;
};

class DesktopEntryManager: public QObject {
	Q_OBJECT;

public:
	void scanDesktopEntries();

	[[nodiscard]] DesktopEntry* byId(const QString& id);

	[[nodiscard]] ObjectModel<DesktopEntry>* applications();

	static DesktopEntryManager* instance();

private:
	explicit DesktopEntryManager();

	void populateApplications();
	void scanPath(const QDir& dir, const QString& prefix = QString());

	QHash<QString, DesktopEntry*> desktopEntries;
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
	Q_PROPERTY(ObjectModel<DesktopEntry>* applications READ applications CONSTANT);
	QML_ELEMENT;
	QML_SINGLETON;

public:
	explicit DesktopEntries();

	/// Look up a desktop entry by name. Includes NoDisplay entries. May return null.
	Q_INVOKABLE [[nodiscard]] static DesktopEntry* byId(const QString& id);

	[[nodiscard]] static ObjectModel<DesktopEntry>* applications();
};
