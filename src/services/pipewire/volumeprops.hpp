namespace qs::service::pipewire {
struct PwVolumeProps {
	QVector<PwAudioChannel::Enum> channels;
	QVector<float> volumes;
	bool mute = false;
	float volumeStep = -1;

	static PwVolumeProps parseSpaPod(const spa_pod* param);
};
}
