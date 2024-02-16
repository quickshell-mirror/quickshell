#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "qmlscreen.hpp"

class QuickShellGlobal: public QObject {
	Q_OBJECT;
	/// All currently connected screens.
	///
	/// This property updates as connected screens change.
	///
	/// #### Reusing a window on every screen
	/// ```qml
	/// ShellRoot {
	///   Variants {
	///     ProxyShellWindow {
	///       // ...
	///     }
	///
	///     // see Variants for details
	///     variants: QuickShell.screens.map(screen => ({ screen }))
	///   }
	/// }
	/// ```
	///
	/// This creates an instance of your window once on every screen.
	/// As screens are added or removed your window will be created or destroyed on those screens.
	Q_PROPERTY(QQmlListProperty<QuickShellScreenInfo> screens READ screens NOTIFY screensChanged);
	QML_SINGLETON;
	QML_NAMED_ELEMENT(QuickShell);

public:
	QuickShellGlobal(QObject* parent = nullptr);

	QQmlListProperty<QuickShellScreenInfo> screens();

	/// Reload the shell from the [ShellRoot].
	///
	/// `hard` - perform a hard reload. If this is false, QuickShell will attempt to reuse windows
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
	static qsizetype screensCount(QQmlListProperty<QuickShellScreenInfo>* prop);
	static QuickShellScreenInfo* screenAt(QQmlListProperty<QuickShellScreenInfo>* prop, qsizetype i);

	QVector<QuickShellScreenInfo*> mScreens;
};
