#pragma once

#include <cstdint>

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QTimer>
#include <QVector>

namespace qs::service::oss {

///! Sound device mode for OSS devices
enum class DeviceMode : std::uint8_t { Playback = 0x01, Record = 0x02, PlaybackRecord = 0x03 };
///! Mixer control type
enum class MixerControlType : std::uint8_t { Volume, Switch, Unknown };

///! Individual OSS mixer control for a sound device
/// Represents a single mixer control such as volume, pcm, mic, etc.
class OSSMixerControl: public QObject {
	Q_OBJECT;
	/// Name of the mixer control (e.g. "vol", "pcm", "mic")
	Q_PROPERTY(QString name READ name CONSTANT);
	/// Left channel volume (0-100)
	Q_PROPERTY(int left READ left WRITE setLeft NOTIFY leftChanged);
	/// Right channel volume (0-100)
	Q_PROPERTY(int right READ right WRITE setRight NOTIFY rightChanged);
	/// Whether this control has separate left/right channels
	Q_PROPERTY(bool stereo READ isStereo CONSTANT);
	/// Whether this control is muted
	Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged);
	/// Type of this control
	Q_PROPERTY(MixerControlType type READ type CONSTANT);
	QML_ELEMENT;
	QML_UNCREATABLE("OSSMixerControl cannot be created");

public:
	explicit OSSMixerControl(
	    QString name,
	    int devId,
	    int mixerId,
	    MixerControlType type,
	    QObject* parent = nullptr
	);

	[[nodiscard]] QString name() const;
	[[nodiscard]] int left() const;
	[[nodiscard]] int right() const;
	[[nodiscard]] bool isStereo() const;
	[[nodiscard]] bool isMuted() const;
	[[nodiscard]] MixerControlType type() const;

	void setLeft(int value);
	void setRight(int value);
	void setMuted(bool muted);

	bool refresh();

signals:
	void leftChanged();
	void rightChanged();
	void mutedChanged();

private:
	bool updateValue();
	bool writeValue();

	QString mName;
	int mDeviceId;
	int mMixerId;
	MixerControlType mType;
	int mLeft = 0;
	int mRight = 0;
	bool mStereo = false;
	bool mMuted = false;
};

///! OSS sound device representation
/// Represents a single sound card or audio interface.
class OSSSoundDevice: public QObject {
	Q_OBJECT;
	/// PCM device number (0, 1, 2, etc.)
	Q_PROPERTY(int deviceId READ deviceId CONSTANT);
	/// Device name (e.g. "pcm0")
	Q_PROPERTY(QString name READ name CONSTANT);
	/// Human-readable device description
	Q_PROPERTY(QString description READ description CONSTANT);
	/// Playback, Record, or both
	Q_PROPERTY(DeviceMode mode READ mode CONSTANT);
	/// Whether this is the system default device
	Q_PROPERTY(bool isDefault READ isDefault NOTIFY isDefaultChanged);
	/// All mixer controls for this device
	Q_PROPERTY(QList<QObject*> controls READ controls NOTIFY controlsChanged);
	/// Master volume control (usually "vol" or "pcm")
	Q_PROPERTY(OSSMixerControl* master READ master NOTIFY masterChanged);
	QML_ELEMENT;
	QML_UNCREATABLE("OSSSoundDevice cannot be created");

public:
	explicit OSSSoundDevice(int deviceId, QObject* parent = nullptr);

	[[nodiscard]] int deviceId() const;
	[[nodiscard]] QString name() const;
	[[nodiscard]] QString description() const;
	[[nodiscard]] DeviceMode mode() const;
	[[nodiscard]] bool isDefault() const;
	[[nodiscard]] QList<QObject*> controls() const;
	[[nodiscard]] OSSMixerControl* master() const;

	void setIsDefault(bool isDefault);
	bool initialize();
	void refresh();

signals:
	void isDefaultChanged();
	void controlsChanged();
	void masterChanged();

private:
	void loadControls();

	int mDeviceId;
	QString mName;
	QString mDescription;
	DeviceMode mMode = DeviceMode::Playback;
	bool mIsDefault = false;
	QVector<OSSMixerControl*> mControls;
	OSSMixerControl* mMaster = nullptr;
};

///! OSS sound system integration (FreeBSD)
/// Provides access to OSS sound devices and mixer controls.
///
/// See @@OSSSoundDevice and @@OSSMixerControl for controlling individual devices.
class OSS: public QObject {
	Q_OBJECT;
	/// All available sound devices
	Q_PROPERTY(QList<QObject*> devices READ devices NOTIFY devicesChanged);
	/// The current system default device
	Q_PROPERTY(OSSSoundDevice* defaultDevice READ defaultDevice NOTIFY defaultDeviceChanged);
	/// Whether OSS is available on this system
	Q_PROPERTY(bool available READ isAvailable CONSTANT);
	QML_ELEMENT;
	QML_SINGLETON;

public:
	explicit OSS(QObject* parent = nullptr);

	[[nodiscard]] QList<QObject*> devices() const;
	[[nodiscard]] OSSSoundDevice* defaultDevice() const;
	[[nodiscard]] bool isAvailable() const;

	/// Set the system default sound device
	Q_INVOKABLE bool setDefaultDevice(int deviceId);
	/// Manually refresh all devices and their states
	Q_INVOKABLE void refresh();

signals:
	void devicesChanged();
	void defaultDeviceChanged();

private:
	void scanDevices();
	void updateDefault();
	void startPolling();

	QVector<OSSSoundDevice*> mDevices;
	OSSSoundDevice* mDefaultDevice = nullptr;
	bool mAvailable = false;
	QTimer* mPollTimer;
};

} // namespace qs::service::oss

Q_DECLARE_METATYPE(qs::service::oss::DeviceMode);
Q_DECLARE_METATYPE(qs::service::oss::MixerControlType);
