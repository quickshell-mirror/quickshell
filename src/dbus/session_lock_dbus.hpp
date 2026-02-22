#pragma once

#include <qdbusabstractadaptor.h>
#include <qobject.h>
#include <qtmetamacros.h>

namespace qs::dbus {

class SessionLockAdaptor: public QDBusAbstractAdaptor {
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.quickshell.SessionLock")

	Q_PROPERTY(bool Locked READ locked NOTIFY lockedChanged) // NOLINT(readability-identifier-naming)
	Q_PROPERTY(bool Secure READ secure NOTIFY secureChanged) // NOLINT(readability-identifier-naming)

public:
	explicit SessionLockAdaptor(QObject* parent);
	~SessionLockAdaptor() override = default;
	Q_DISABLE_COPY_MOVE(SessionLockAdaptor);

	void setLocked(bool locked);
	void setSecure(bool secure);

	[[nodiscard]] bool locked() const { // NOLINT(readability-identifier-naming)
		return this->mLocked;
	}
	[[nodiscard]] bool secure() const { // NOLINT(readability-identifier-naming)
		return this->mSecure;
	}

public slots:
	[[nodiscard]] bool GetLocked() const; // NOLINT(readability-identifier-naming)
	[[nodiscard]] bool GetSecure() const; // NOLINT(readability-identifier-naming)

signals:
	void LockedChanged(bool locked); // NOLINT(readability-identifier-naming)
	void SecureChanged(bool secure); // NOLINT(readability-identifier-naming)

	void lockedChanged(bool locked); // NOLINT(readability-identifier-naming)
	void secureChanged(bool secure); // NOLINT(readability-identifier-naming)

private:
	bool mLocked = false;
	bool mSecure = false;
};

} // namespace qs::dbus
