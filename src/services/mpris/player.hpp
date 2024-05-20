#pragma once

#include <qdbusextratypes.h>
#include <qdbuspendingcall.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qtypes.h>

#include "../../dbus/properties.hpp"
#include "dbus_player.h"

Q_DECLARE_LOGGING_CATEGORY(logMprisPlayer);

namespace qs::service::mp {

class MprisPlayer;

class MprisPlayer: public QObject {
	Q_OBJECT;

public:
	explicit MprisPlayer(const QString& address, QObject* parent = nullptr);	
	QString watcherId; // TODO: maybe can be private CHECK

	void setPosition(QDBusObjectPath trackId, qlonglong position);
	void next();
	void previous();
	void pause();
	void playPause();
 	void stop(); 
 	void play();

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] bool isReady() const;
	
	// clang-format off
	dbus::DBusPropertyGroup properties;
	dbus::DBusProperty<bool> canControl {this->properties, "CanControl" };
	dbus::DBusProperty<bool> canGoNext {this->properties, "CanGoNext" };
	dbus::DBusProperty<bool> canGoPrevious {this->properties, "CanGoPrevious" };
	dbus::DBusProperty<bool> canPlay {this->properties, "CanPlay" };
	dbus::DBusProperty<bool> canPause {this->properties, "CanPause" };
	dbus::DBusProperty<QVariantMap> metadata {this->properties, "Metadata"};
	dbus::DBusProperty<QString> playbackStatus {this->properties, "PlaybackStatus" };	
	dbus::DBusProperty<qlonglong> position {this->properties, "Position" };
	dbus::DBusProperty<double> minimumRate {this->properties, "MinimumRate" };
	dbus::DBusProperty<double> maximumRate {this->properties, "MaximumRate" };

	dbus::DBusProperty<QString> loopStatus {this->properties, "LoopStatus" };
	dbus::DBusProperty<double> rate {this->properties, "Rate" };
	dbus::DBusProperty<bool> shuffle {this->properties, "Shuffle" };
	dbus::DBusProperty<double> volume {this->properties, "Volume" };
	// clang-format on

signals:
	void ready();

private slots:
	void onGetAllFinished();
	void updatePlayer();

private:
	DBusMprisPlayer* player = nullptr;
	bool mReady = false;
};

} // namespace qs::service::mp
