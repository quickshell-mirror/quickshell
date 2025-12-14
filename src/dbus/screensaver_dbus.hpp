#pragma once

#include <qdbusabstractadaptor.h>
#include <qobject.h>
#include <qtmetamacros.h>

namespace qs::dbus {

class ScreenSaverAdaptor: public QDBusAbstractAdaptor {
	Q_OBJECT;
	Q_CLASSINFO("D-Bus Interface", "org.freedesktop.ScreenSaver")

public:
	explicit ScreenSaverAdaptor(QObject* parent);
	~ScreenSaverAdaptor() override = default;
	Q_DISABLE_COPY_MOVE(ScreenSaverAdaptor);

	void setActive(bool active);
	void setSecure(bool secure);

public slots:
	[[nodiscard]] bool GetActive() const; // NOLINT(readability-identifier-naming)
	[[nodiscard]] bool GetSecure() const; // NOLINT(readability-identifier-naming)

signals:
	void ActiveChanged(bool active); // NOLINT(readability-identifier-naming)

private:
	bool mActive = false;
	bool mSecure = false;
};

} // namespace qs::dbus
