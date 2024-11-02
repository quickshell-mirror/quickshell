#include "monitor.hpp"

#include <qcontainerfwd.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "workspace.hpp"

namespace qs::i3::ipc {

qint32 I3Monitor::id() const { return this->mId; };
QString I3Monitor::name() const { return this->mName; };
bool I3Monitor::power() const { return this->mPower; };
I3Workspace* I3Monitor::focusedWorkspace() const { return this->mFocusedWorkspace; };
qint32 I3Monitor::x() const { return this->mX; };
qint32 I3Monitor::y() const { return this->mY; };
qint32 I3Monitor::width() const { return this->mWidth; };
qint32 I3Monitor::height() const { return this->mHeight; };
qreal I3Monitor::scale() const { return this->mScale; };
bool I3Monitor::focused() const { return this->mFocused; };
QVariantMap I3Monitor::lastIpcObject() const { return this->mLastIpcObject; };

void I3Monitor::updateFromObject(const QVariantMap& obj) {
	auto id = obj.value("id").value<qint32>();
	auto name = obj.value("name").value<QString>();
	auto power = obj.value("power").value<bool>();
	auto activeWorkspaceId = obj.value("current_workspace").value<QString>();
	auto rect = obj.value("rect").toMap();
	auto x = rect.value("x").value<qint32>();
	auto y = rect.value("y").value<qint32>();
	auto width = rect.value("width").value<qint32>();
	auto height = rect.value("height").value<qint32>();
	auto scale = obj.value("scale").value<qreal>();
	auto focused = obj.value("focused").value<bool>();

	if (id != this->mId) {
		this->mId = id;
		emit this->idChanged();
	}

	if (name != this->mName) {
		this->mName = name;
		emit this->nameChanged();
	}

	if (power != this->mPower) {
		this->mPower = power;
		this->powerChanged();
	}

	if (activeWorkspaceId != this->mFocusedWorkspaceName) {
		auto* workspace = this->ipc->findWorkspaceByName(activeWorkspaceId);
		if (activeWorkspaceId.isEmpty() || workspace == nullptr) { // is null when output is disabled
			this->mFocusedWorkspace = nullptr;
			this->mFocusedWorkspaceName = "";
		} else {
			this->mFocusedWorkspaceName = activeWorkspaceId;
			this->mFocusedWorkspace = workspace;
		}
		emit this->focusedWorkspaceChanged();
	};

	if (x != this->mX) {
		this->mX = x;
		emit this->xChanged();
	}

	if (y != this->mY) {
		this->mY = y;
		emit this->yChanged();
	}

	if (width != this->mWidth) {
		this->mWidth = width;
		emit this->widthChanged();
	}

	if (height != this->mHeight) {
		this->mHeight = height;
		emit this->heightChanged();
	}

	if (scale != this->mScale) {
		this->mScale = scale;
		emit this->scaleChanged();
	}

	if (focused != this->mFocused) {
		this->mFocused = focused;
		emit this->focusedChanged();
	}

	if (obj != this->mLastIpcObject) {
		this->mLastIpcObject = obj;
		emit this->lastIpcObjectChanged();
	}
}

void I3Monitor::setFocus(bool focused) {
	this->mFocused = focused;
	emit this->focusedChanged();
}

void I3Monitor::setFocusedWorkspace(I3Workspace* workspace) {
	this->mFocusedWorkspace = workspace;
	this->mFocusedWorkspaceName = workspace->name();
	emit this->focusedWorkspaceChanged();
};

} // namespace qs::i3::ipc
