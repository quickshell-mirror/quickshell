#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "../core/generation.hpp"

namespace qs::ui {

class ReloadPopup: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(ReloadPopupInfo);
	QML_UNCREATABLE("")
	Q_PROPERTY(QString instanceId MEMBER instanceId CONSTANT);
	Q_PROPERTY(bool failed MEMBER failed CONSTANT);
	Q_PROPERTY(QString errorString MEMBER errorString CONSTANT);

public:
	Q_INVOKABLE void closed();

	static void spawnPopup(QString instanceId, bool failed, QString errorString);

private:
	ReloadPopup(QString instanceId, bool failed, QString errorString);

	EngineGeneration* generation;
	QObject* popup = nullptr;
	QString instanceId;
	bool failed = false;
	bool deleting = false;
	QString errorString;

	static ReloadPopup* activePopup;
};

} // namespace qs::ui
