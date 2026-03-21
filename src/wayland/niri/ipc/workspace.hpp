#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

class QJsonObject;

namespace qs::niri::ipc {

class NiriIpc;

///! Niri workspace.
/// Represents a workspace as reported by niri.
class NiriWorkspace: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(qint64 id READ default NOTIFY idChanged BINDABLE bindableId);
	Q_PROPERTY(qint32 idx READ default NOTIFY idxChanged BINDABLE bindableIdx);
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	Q_PROPERTY(QString output READ default NOTIFY outputChanged BINDABLE bindableOutput);
	Q_PROPERTY(bool focused READ default NOTIFY focusedChanged BINDABLE bindableFocused);
	Q_PROPERTY(bool active READ default NOTIFY activeChanged BINDABLE bindableActive);
	Q_PROPERTY(bool urgent READ default NOTIFY urgentChanged BINDABLE bindableUrgent);
	Q_PROPERTY(bool occupied READ default NOTIFY occupiedChanged BINDABLE bindableOccupied);
	Q_PROPERTY(qint64 activeWindowId READ default NOTIFY activeWindowIdChanged BINDABLE bindableActiveWindowId);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("NiriWorkspaces must be retrieved from the Niri object.");

public:
	explicit NiriWorkspace(QObject* parent): QObject(parent) {}

	void updateFromJson(const QJsonObject& object);
	void setFocused(bool focused);
	void setActive(bool active);
	void setActiveWindowId(qint64 id);

	[[nodiscard]] QBindable<qint64> bindableId() { return &this->bId; }
	[[nodiscard]] QBindable<qint32> bindableIdx() { return &this->bIdx; }
	[[nodiscard]] QBindable<QString> bindableName() { return &this->bName; }
	[[nodiscard]] QBindable<QString> bindableOutput() { return &this->bOutput; }
	[[nodiscard]] QBindable<bool> bindableFocused() { return &this->bFocused; }
	[[nodiscard]] QBindable<bool> bindableActive() { return &this->bActive; }
	[[nodiscard]] QBindable<bool> bindableUrgent() { return &this->bUrgent; }
	[[nodiscard]] QBindable<bool> bindableOccupied() { return &this->bOccupied; }
	[[nodiscard]] QBindable<qint64> bindableActiveWindowId() { return &this->bActiveWindowId; }

signals:
	void idChanged();
	void idxChanged();
	void nameChanged();
	void outputChanged();
	void focusedChanged();
	void activeChanged();
	void urgentChanged();
	void occupiedChanged();
	void activeWindowIdChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriWorkspace, qint64, bId, -1, &NiriWorkspace::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, qint32, bIdx, &NiriWorkspace::idxChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, QString, bName, &NiriWorkspace::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, QString, bOutput, &NiriWorkspace::outputChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, bool, bFocused, &NiriWorkspace::focusedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, bool, bActive, &NiriWorkspace::activeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, bool, bUrgent, &NiriWorkspace::urgentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWorkspace, bool, bOccupied, &NiriWorkspace::occupiedChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriWorkspace, qint64, bActiveWindowId, -1, &NiriWorkspace::activeWindowIdChanged);
	// clang-format on
};

} // namespace qs::niri::ipc
