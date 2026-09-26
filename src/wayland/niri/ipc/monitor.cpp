#include "monitor.hpp"

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "connection.hpp"
#include "workspace.hpp"

namespace qs::niri::ipc {

NiriMonitor::NiriMonitor(NiriIpc* ipc): QObject(ipc), ipc(ipc) {
	this->bFocused.setBinding([this]() {
		return this->ipc->bindableFocusedMonitor().value() == this;
	});
}

void NiriMonitor::updateInitial(const QString& name) { this->bName = name; }

void NiriMonitor::updateFromObject(const QVariantMap& object) {
	Qt::beginPropertyUpdateGroup();

	this->bName = object.value("name").value<QString>();
	this->bMake = object.value("make").value<QString>();
	this->bModel = object.value("model").value<QString>();

	// logical is null when the output is disabled.
	auto logical = object.value("logical").toMap();

	this->bX = logical.value("x").value<qint32>();
	this->bY = logical.value("y").value<qint32>();
	this->bWidth = logical.value("width").value<qint32>();
	this->bHeight = logical.value("height").value<qint32>();
	this->bScale = logical.value("scale").value<qreal>();

	this->mLastIpcObject = object;
	emit this->lastIpcObjectChanged();

	Qt::endPropertyUpdateGroup();
}

QVariantMap NiriMonitor::lastIpcObject() const { return this->mLastIpcObject; }

void NiriMonitor::setActiveWorkspace(NiriWorkspace* workspace) {
	auto* oldWorkspace = this->bActiveWorkspace.value();
	if (workspace == oldWorkspace) return;

	if (oldWorkspace != nullptr) {
		QObject::disconnect(oldWorkspace, nullptr, this, nullptr);
	}

	if (workspace != nullptr) {
		QObject::connect(
		    workspace,
		    &QObject::destroyed,
		    this,
		    &NiriMonitor::onActiveWorkspaceDestroyed
		);
	}

	this->bActiveWorkspace = workspace;
}

void NiriMonitor::onActiveWorkspaceDestroyed() { this->bActiveWorkspace = nullptr; }

} // namespace qs::niri::ipc
