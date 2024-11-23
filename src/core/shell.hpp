#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qqmlengine.h>
#include <qtmetamacros.h>

#include "qmlglobal.hpp"
#include "reload.hpp"

///! Optional root config element, allowing some settings to be specified inline.
class ShellRoot: public ReloadPropagator {
	Q_OBJECT;
	Q_PROPERTY(QuickshellSettings* settings READ settings CONSTANT);
	QML_ELEMENT;

public:
	explicit ShellRoot(QObject* parent = nullptr): ReloadPropagator(parent) {}

	[[nodiscard]] QuickshellSettings* settings() const;
};
