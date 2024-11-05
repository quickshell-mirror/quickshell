#pragma once

#include <qicon.h>
#include <qqmlengine.h>
#include <qurl.h>

QIcon getEngineImageAsIcon(QQmlEngine* engine, const QUrl& url);
QIcon getCurrentEngineImageAsIcon(const QUrl& url);
