#pragma once

#include <qdbusabstractadaptor.h>
#include <qobject.h>
#include <qtmetamacros.h>

namespace qs::dbus {

// DBus adaptor for exposing screen lock state to other applications
// Implements org.freedesktop.ScreenSaver interface
class ScreenSaverAdaptor: public QDBusAbstractAdaptor {
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.freedesktop.ScreenSaver")

public:
	explicit ScreenSaverAdaptor(QObject* parent);
	~ScreenSaverAdaptor() override = default;
	Q_DISABLE_COPY_MOVE(ScreenSaverAdaptor);

	// Update the active state and emit signal if changed
	void setActive(bool active);
	// Update the secure state (compositor has confirmed lock)
	void setSecure(bool secure);

public slots: // NOLINT
	// DBus method: Get current lock state
	[[nodiscard]] bool GetActive() const;
	// DBus method: Get whether lock is secure (compositor confirmed)
	[[nodiscard]] bool GetSecure() const;

signals: // NOLINT
	// DBus signal: Emitted when lock state changes
	void ActiveChanged(bool active);

private:
	bool mActive = false;
	bool mSecure = false;
};

} // namespace qs::dbus
