#pragma once

#include <qobject.h>
#include <qqmlintegration.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvector.h>

#include "node.hpp"

namespace qs::service::pipewire {

class PwNodeIface;
class PwPeakStream;

} // namespace qs::service::pipewire

Q_DECLARE_OPAQUE_POINTER(qs::service::pipewire::PwNodeIface*);

namespace qs::service::pipewire {

///! Monitors peak levels of an audio node.
/// Tracks volume peaks for a node across all its channels.
///
/// The peak monitor binds nodes similarly to @@PwObjectTracker when enabled.
class PwNodePeakMonitor: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The node to monitor. Must be an audio node.
	Q_PROPERTY(qs::service::pipewire::PwNodeIface* node READ node WRITE setNode NOTIFY nodeChanged);
	/// If true, the monitor is actively capturing and computing peaks. Defaults to true.
	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged);
	/// Per-channel peak noise levels (0.0-1.0). Length matches @@channels.
	///
  /// The channel's volume does not affect this property.
	Q_PROPERTY(QVector<float> peaks READ peaks NOTIFY peaksChanged);
	/// Maximum value of @@peaks.
	Q_PROPERTY(float peak READ peak NOTIFY peakChanged);
	/// Channel positions for the captured format. Length matches @@peaks.
	Q_PROPERTY(QVector<qs::service::pipewire::PwAudioChannel::Enum> channels READ channels NOTIFY channelsChanged);
	// clang-format on
	QML_ELEMENT;

public:
	explicit PwNodePeakMonitor(QObject* parent = nullptr);
	~PwNodePeakMonitor() override;
	Q_DISABLE_COPY_MOVE(PwNodePeakMonitor);

	[[nodiscard]] PwNodeIface* node() const;
	void setNode(PwNodeIface* node);

	[[nodiscard]] bool isEnabled() const;
	void setEnabled(bool enabled);

	[[nodiscard]] QVector<float> peaks() const { return this->mPeaks; }
	[[nodiscard]] float peak() const { return this->mPeak; }
	[[nodiscard]] QVector<PwAudioChannel::Enum> channels() const { return this->mChannels; }

signals:
	void nodeChanged();
	void enabledChanged();
	void peaksChanged();
	void peakChanged();
	void channelsChanged();

private slots:
	void onNodeDestroyed();

private:
	friend class PwPeakStream;

	void updatePeaks(const QVector<float>& peaks, float peak);
	void updateChannels(const QVector<PwAudioChannel::Enum>& channels);
	void clearPeaks();
	void rebuildStream();

	PwNodeIface* mNode = nullptr;
	PwBindableRef<PwNode> mNodeRef;
	bool mEnabled = true;
	QVector<float> mPeaks;
	float mPeak = 0.0f;
	QVector<PwAudioChannel::Enum> mChannels;
	PwPeakStream* mStream = nullptr;
};

} // namespace qs::service::pipewire
