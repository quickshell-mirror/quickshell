#pragma once

#include <qcontainerfwd.h>
#include <qjsengine.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "qmlscreen.hpp"

///! Accessor for some options under the Quickshell type.
class QuickshellSettings: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Quickshell's working directory. Defaults to whereever quickshell was launched from.
	Q_PROPERTY(QString workingDirectory READ workingDirectory WRITE setWorkingDirectory NOTIFY workingDirectoryChanged);
	/// If true then the configuration will be reloaded whenever any files change.
	/// Defaults to true.
	Q_PROPERTY(bool watchFiles READ watchFiles WRITE setWatchFiles NOTIFY watchFilesChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("singleton");

public:
	[[nodiscard]] QString workingDirectory() const;
	void setWorkingDirectory(QString workingDirectory);

	[[nodiscard]] bool watchFiles() const;
	void setWatchFiles(bool watchFiles);

	static QuickshellSettings* instance();
	static void reset();

signals:
	void workingDirectoryChanged();
	void watchFilesChanged();

private:
	bool mWatchFiles = true;
};

class QuickshellGlobal: public QObject {
	Q_OBJECT;
	// clang-format off
	/// All currently connected screens.
	///
	/// This property updates as connected screens change.
	///
	/// #### Reusing a window on every screen
	/// ```qml
	/// ShellRoot {
	///   Variants {
	///     ShellWindow {
	///       // ...
	///     }
	///
	///     // see Variants for details
	///     variants: Quickshell.screens.map(screen => ({ screen }))
	///   }
	/// }
	/// ```
	///
	/// This creates an instance of your window once on every screen.
	/// As screens are added or removed your window will be created or destroyed on those screens.
	Q_PROPERTY(QQmlListProperty<QuickshellScreenInfo> screens READ screens NOTIFY screensChanged);
	/// Quickshell's working directory. Defaults to whereever quickshell was launched from.
	Q_PROPERTY(QString workingDirectory READ workingDirectory WRITE setWorkingDirectory NOTIFY workingDirectoryChanged);
	/// If true then the configuration will be reloaded whenever any files change.
	/// Defaults to true.
	Q_PROPERTY(bool watchFiles READ watchFiles WRITE setWatchFiles NOTIFY watchFilesChanged);
	// clang-format on
	QML_SINGLETON;
	QML_NAMED_ELEMENT(Quickshell);

public:
	QuickshellGlobal(QObject* parent = nullptr);

	QQmlListProperty<QuickshellScreenInfo> screens();

	/// Reload the shell from the [ShellRoot].
	///
	/// `hard` - perform a hard reload. If this is false, Quickshell will attempt to reuse windows
	/// that already exist. If true windows will be recreated.
	///
	/// See [Reloadable] for more information on what can be reloaded and how.
	///
	/// [Reloadable]: ../reloadable
	Q_INVOKABLE void reload(bool hard);

	/// Returns the string value of an environment variable or null if it is not set.
	Q_INVOKABLE QVariant env(const QString& variable);

	[[nodiscard]] QString workingDirectory() const;
	void setWorkingDirectory(QString workingDirectory);

	[[nodiscard]] bool watchFiles() const;
	void setWatchFiles(bool watchFiles);

signals:
	void screensChanged();
	void workingDirectoryChanged();
	void watchFilesChanged();

public slots:
	void updateScreens();

private:
	static qsizetype screensCount(QQmlListProperty<QuickshellScreenInfo>* prop);
	static QuickshellScreenInfo* screenAt(QQmlListProperty<QuickshellScreenInfo>* prop, qsizetype i);

	QVector<QuickshellScreenInfo*> mScreens;
};
