#pragma once

#include <functional>

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "../core/model.hpp"
#include "workspace.hpp"

namespace qs::wm {

class WindowManager: public QObject {
	Q_OBJECT;

public:
	static void setProvider(std::function<WindowManager*()> provider);
	static WindowManager* instance();

	[[nodiscard]] virtual UntypedObjectModel* workspaces() const {
		return UntypedObjectModel::emptyInstance();
	}

	[[nodiscard]] virtual UntypedObjectModel* workspaceGroups() const {
		return UntypedObjectModel::emptyInstance();
	}

private:
	static std::function<WindowManager*()> provider;
};

class WindowManagerQml: public QObject {
	Q_OBJECT;
	QML_NAMED_ELEMENT(WindowManager);
	QML_SINGLETON;
	Q_PROPERTY(UntypedObjectModel* workspaces READ workspaces CONSTANT);
	Q_PROPERTY(UntypedObjectModel* workspaceGroups READ workspaceGroups CONSTANT);

public:
	[[nodiscard]] static UntypedObjectModel* workspaces() {
		return WindowManager::instance()->workspaces();
	}

	[[nodiscard]] static UntypedObjectModel* workspaceGroups() {
		return WindowManager::instance()->workspaceGroups();
	}
};

} // namespace qs::wm
