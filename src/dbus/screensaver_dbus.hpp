#pragma once

#include <QDBusAbstractAdaptor>
#include <QObject>
#include <QtGlobal> // QtWarningMsg

namespace qs::dbus {

class ScreenSaverAdaptor: public QDBusAbstractAdaptor {
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.freedesktop.ScreenSaver")

public:
	explicit ScreenSaverAdaptor(QObject* parent);
	~ScreenSaverAdaptor() override = default;
	Q_DISABLE_COPY_MOVE(ScreenSaverAdaptor);

	void setActive(bool active);
	void setSecure(bool secure);

public slots:
	[[nodiscard]] bool getActive() const;
	[[nodiscard]] bool getSecure() const;

signals:
	void activeChanged(bool active);

private:
	bool mActive = false;
	bool mSecure = false;
};

} // namespace qs::dbus
