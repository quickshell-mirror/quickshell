#pragma once

#include <qdbusextratypes.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtypes.h>

#include "player.hpp"



///! Mpris implementation for quickshell 
/// mpris service, get useful information from apps that implement media player fucntionality [mpris spec]
/// (Beware of misuse of spec, it is just a suggestion for most)
///
///
/// [mpris spec]: https://specifications.freedesktop.org/mpris-spec 
class Player: public QObject {
	Q_OBJECT;
	// READ-ONLY 
	Q_PROPERTY(bool canControl READ canControl NOTIFY canControlChanged);
	Q_PROPERTY(bool canGoNext READ canGoNext NOTIFY canGoNextChanged);
	Q_PROPERTY(bool canGoPrevious READ canGoPrevious NOTIFY canGoPreviousChanged);
	Q_PROPERTY(bool canPlay READ canPlay NOTIFY canPlayChanged);
	Q_PROPERTY(bool canPause READ canPause NOTIFY canPauseChanged);	
	Q_PROPERTY(QVariantMap metadata READ metadata NOTIFY metadataChanged);
	Q_PROPERTY(QString playbackStatus READ playbackStatus NOTIFY playbackStatusChanged);
	Q_PROPERTY(qlonglong position READ position NOTIFY positionChanged);
	Q_PROPERTY(double minimumRate READ minimumRate NOTIFY minimumRateChanged);
	Q_PROPERTY(double maximumRate READ maximumRate NOTIFY maximumRateChanged);
	
	// READ/WRITE - Write isn't implemented thus this need to fixed when that happens. 
	Q_PROPERTY(QString loopStatus READ loopStatus NOTIFY loopStatusChanged);
	Q_PROPERTY(double rate READ rate NOTIFY rateChanged);
	Q_PROPERTY(bool shuffle READ shuffle NOTIFY shuffleChanged);
	Q_PROPERTY(double volume READ volume NOTIFY volumeChanged);
	
	QML_ELEMENT;
	QML_UNCREATABLE("MprisPlayers can only be acquired from Mpris");

public:
	explicit Player(qs::service::mp::MprisPlayer* player, QObject* parent = nullptr);
	
	// These are all self-explanatory.
	Q_INVOKABLE void setPosition(QDBusObjectPath trackId, qlonglong position) const; 
	Q_INVOKABLE void next() const; 
	Q_INVOKABLE void previous() const;
	Q_INVOKABLE void pause() const; 
	Q_INVOKABLE void playPause() const;
	Q_INVOKABLE void stop() const; 
	Q_INVOKABLE void play() const; 

	[[nodiscard]] bool canControl() const; 
	[[nodiscard]] bool canGoNext() const;
	[[nodiscard]] bool canGoPrevious() const; 
	[[nodiscard]] bool canPlay() const; 
	[[nodiscard]] bool canPause() const;
	[[nodiscard]] QVariantMap metadata() const; 
	[[nodiscard]] QString playbackStatus() const;	
	[[nodiscard]] qlonglong position() const;  
	[[nodiscard]] double minimumRate() const;
	[[nodiscard]] double maximumRate() const;

	[[nodiscard]] QString loopStatus() const; 
	[[nodiscard]] double rate() const; 
	[[nodiscard]] bool shuffle() const; 
	[[nodiscard]] double volume() const;

	qs::service::mp::MprisPlayer* player = nullptr;

signals:
	void canControlChanged();
	void canGoNextChanged(); 
	void canGoPreviousChanged();
	void canPlayChanged();
	void canPauseChanged();
	void metadataChanged();
	void playbackStatusChanged();	
	void positionChanged();
	void minimumRateChanged();
	void maximumRateChanged();

	void loopStatusChanged();
	void rateChanged();
	void shuffleChanged();
	void volumeChanged();
};

class Mpris: public QObject {
	Q_OBJECT;
	Q_PROPERTY(QQmlListProperty<Player> players READ players NOTIFY playersChanged);
	QML_ELEMENT;
	QML_SINGLETON;

public:
	explicit Mpris(QObject* parent = nullptr);
	
	[[nodiscard]] QQmlListProperty<Player> players();

signals:
	void playersChanged();

private slots:
	void onPlayerRegistered(const QString& service);
	void onPlayerUnregistered(const QString& service);

private:
	static qsizetype playersCount(QQmlListProperty<Player>* property);
	static Player* playerAt(QQmlListProperty<Player>* property, qsizetype index);
	static Player* playerWithAddress(QQmlListProperty<Player> property, const QString& address);  

	QList<Player*> mPlayers;
};
