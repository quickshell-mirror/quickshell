#pragma once

#include <qfilesystemwatcher.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtranslator.h>
#include <qurl.h>

#include "generation.hpp"

class RootWrapper: public QObject {
	Q_OBJECT;

public:
	explicit RootWrapper(QString rootPath, QString shellId);
	~RootWrapper() override;
	Q_DISABLE_COPY_MOVE(RootWrapper);

	void reloadGraph(bool hard);

private slots:
	void generationDestroyed();
	void onWatchFilesChanged();
	void onWatchedFilesChanged();
	void updateTooling();

private:
	void updateTranslations(QQmlEngine* engine);

	QString rootPath;
	QString shellId;
	EngineGeneration* generation = nullptr;
	QString originalWorkingDirectory;
	QFileSystemWatcher configDirWatcher;
	// Keep one catalog across overlapping engine generations. Qt translators are process-wide.
	QTranslator translator;
	QMetaObject::Connection translationLanguageConnection;
};
