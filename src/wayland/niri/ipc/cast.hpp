#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "connection.hpp"

namespace qs::niri::ipc {

class NiriCast: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Stream id of the screencast. Uniquely identifies it.
	Q_PROPERTY(qint64 streamId READ default NOTIFY streamIdChanged BINDABLE bindableStreamId);
	/// Session id of the screencast. A session can have multiple streams.
	Q_PROPERTY(qint64 sessionId READ default NOTIFY sessionIdChanged BINDABLE bindableSessionId);
	/// Kind of the screencast: "PipeWire" or "WlrScreencopy".
	Q_PROPERTY(QString kind READ default NOTIFY kindChanged BINDABLE bindableKind);
	/// Name of the screencasted output, or empty if the cast does not target an output.
	Q_PROPERTY(QString targetOutput READ default NOTIFY targetOutputChanged BINDABLE bindableTargetOutput);
	/// Id of the screencasted window, or 0 if the cast does not target a window.
	Q_PROPERTY(qint64 targetWindowId READ default NOTIFY targetWindowIdChanged BINDABLE bindableTargetWindowId);
	/// Whether this cast's target can be changed by actions like SetDynamicCastWindow.
	Q_PROPERTY(bool dynamicTarget READ default NOTIFY dynamicTargetChanged BINDABLE bindableDynamicTarget);
	/// Whether the cast is currently streaming frames.
	Q_PROPERTY(bool active READ default NOTIFY activeChanged BINDABLE bindableActive);
	/// Process ID of the screencast consumer, or 0 if unknown.
	Q_PROPERTY(qint32 pid READ default NOTIFY pidChanged BINDABLE bindablePid);
	/// PipeWire node ID of the screencast stream, or -1 if none.
	Q_PROPERTY(qint32 pwNodeId READ default NOTIFY pwNodeIdChanged BINDABLE bindablePwNodeId);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("NiriCasts must be retrieved from the Niri singleton.");

public:
	explicit NiriCast(NiriIpc* ipc);

	void updateFromObject(const QVariantMap& object);

	[[nodiscard]] QBindable<qint64> bindableStreamId() { return &this->bStreamId; }
	[[nodiscard]] QBindable<qint64> bindableSessionId() { return &this->bSessionId; }
	[[nodiscard]] QBindable<QString> bindableKind() { return &this->bKind; }
	[[nodiscard]] QBindable<QString> bindableTargetOutput() { return &this->bTargetOutput; }
	[[nodiscard]] QBindable<qint64> bindableTargetWindowId() { return &this->bTargetWindowId; }
	[[nodiscard]] QBindable<bool> bindableDynamicTarget() { return &this->bDynamicTarget; }
	[[nodiscard]] QBindable<bool> bindableActive() { return &this->bActive; }
	[[nodiscard]] QBindable<qint32> bindablePid() { return &this->bPid; }
	[[nodiscard]] QBindable<qint32> bindablePwNodeId() { return &this->bPwNodeId; }

signals:
	void streamIdChanged();
	void sessionIdChanged();
	void kindChanged();
	void targetOutputChanged();
	void targetWindowIdChanged();
	void dynamicTargetChanged();
	void activeChanged();
	void pidChanged();
	void pwNodeIdChanged();

private:
	NiriIpc* ipc;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriCast, qint64, bStreamId, 0, &NiriCast::streamIdChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriCast, qint64, bSessionId, &NiriCast::sessionIdChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriCast, QString, bKind, &NiriCast::kindChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriCast, QString, bTargetOutput, &NiriCast::targetOutputChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriCast, qint64, bTargetWindowId, &NiriCast::targetWindowIdChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriCast, bool, bDynamicTarget, &NiriCast::dynamicTargetChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriCast, bool, bActive, &NiriCast::activeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriCast, qint32, bPid, &NiriCast::pidChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriCast, qint32, bPwNodeId, -1, &NiriCast::pwNodeIdChanged);
	// clang-format on
};

} // namespace qs::niri::ipc
