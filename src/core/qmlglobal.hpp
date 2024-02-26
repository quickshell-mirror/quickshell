#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "qmlscreen.hpp"

class QuickshellGlobal: public QObject {
	Q_OBJECT;
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

signals:
	void screensChanged();

public slots:
	void updateScreens();

private:
	static qsizetype screensCount(QQmlListProperty<QuickshellScreenInfo>* prop);
	static QuickshellScreenInfo* screenAt(QQmlListProperty<QuickshellScreenInfo>* prop, qsizetype i);

	QVector<QuickshellScreenInfo*> mScreens;
};
