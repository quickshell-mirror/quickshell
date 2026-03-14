#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "wayland-dwl-ipc-unstable-v2-client-protocol.h"

namespace qs::dwl {

///! State of a single DWL tag.
/// Represents one tag slot on @@DwlIpcOutput.
class DwlTag: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("DwlTag instances are created by DwlIpcOutput.");

	/// Zero-based index of this tag.
	Q_PROPERTY(quint32 index READ index CONSTANT);
	/// Whether this tag is currently active on its output.
	Q_PROPERTY(bool active READ active NOTIFY activeChanged);
	/// Whether any client on this tag is urgent.
	Q_PROPERTY(bool urgent READ urgent NOTIFY urgentChanged);
	/// Number of clients assigned to this tag.
	Q_PROPERTY(quint32 clientCount READ clientCount NOTIFY clientCountChanged);
	/// Nonzero index of the focused client within this tag, 0 if none.
	Q_PROPERTY(quint32 focusedClient READ focusedClient NOTIFY focusedClientChanged);

public:
	explicit DwlTag(quint32 index, QObject* parent = nullptr);

	[[nodiscard]] quint32 index() const;
	[[nodiscard]] bool active() const;
	[[nodiscard]] bool urgent() const;
	[[nodiscard]] quint32 clientCount() const;
	[[nodiscard]] quint32 focusedClient() const;

	void updateState(quint32 state, quint32 clients, quint32 focused);

signals:
	void activeChanged();
	void urgentChanged();
	void clientCountChanged();
	void focusedClientChanged();

private:
	quint32 mIndex;
	bool mActive = false;
	bool mUrgent = false;
	quint32 mClientCount = 0;
	quint32 mFocusedClient = 0;
};

} // namespace qs::dwl
