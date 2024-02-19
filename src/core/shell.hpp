#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qqmlengine.h>
#include <qtmetamacros.h>

#include "reload.hpp"

class ShellConfig {
	Q_GADGET;
	Q_PROPERTY(bool watchFiles MEMBER mWatchFiles);

public:
	bool mWatchFiles = true;
};

///! Root config element
class ShellRoot: public ReloadPropagator {
	Q_OBJECT;
	/// If `config.watchFiles` is true the configuration will be reloaded whenever it changes.
	/// Defaults to true.
	Q_PROPERTY(ShellConfig config READ config WRITE setConfig);
	QML_ELEMENT;

public:
	explicit ShellRoot(QObject* parent = nullptr): ReloadPropagator(parent) {}

	void setConfig(ShellConfig config);
	[[nodiscard]] ShellConfig config() const;

signals:
	void configChanged();

private:
	ShellConfig mConfig;
};
