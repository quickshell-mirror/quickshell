#pragma once

#include <qfilesystemwatcher.h>
#include <qhash.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qtimer.h>

class DesktopEntryMonitor: public QObject {
	Q_OBJECT

public:
	enum class ChangeEvent { Added, Modified, Removed };

	explicit DesktopEntryMonitor(QObject* parent = nullptr);
	~DesktopEntryMonitor() = default;

signals:
	void desktopEntriesChanged(const QHash<QString, ChangeEvent>& changes);

private slots:
	void onDirectoryChanged(const QString& path);
	void processChanges();

private:
	void initializeDesktopPaths();
	void startMonitoring();
	void scanAndWatch(const QString& dirPath);
	void queueChange(ChangeEvent event, const QString& path);

	QFileSystemWatcher* watcher;
	QStringList desktopPaths;
	QTimer* debounceTimer;
	QHash<QString, ChangeEvent> pendingChanges;
};