#pragma once

#include <qlist.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qstring.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwayland-dwl-ipc-unstable-v2.h>

#include "tag.hpp"

namespace qs::dwl {

///! A DWL monitor/output with IPC state.
/// Exposes per-monitor compositor state: tag list, active layout, focused
/// window title, app ID, and fullscreen/floating flags.
///
/// Obtain instances via @@DwlIpc.outputs.
class DwlIpcOutput
    : public QObject
    , public QtWayland::zdwl_ipc_output_v2 {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("DwlIpcOutput instances are created by DwlIpc.");

	/// Whether this output is currently focused.
	Q_PROPERTY(bool active READ active NOTIFY activeChanged);
	/// Index into @@DwlIpc.layouts of the active layout.
	Q_PROPERTY(quint32 layoutIndex READ layoutIndex NOTIFY layoutIndexChanged);
	/// Current layout symbol string (e.g. "[]="). Use this for display.
	Q_PROPERTY(QString layoutSymbol READ layoutSymbol NOTIFY layoutSymbolChanged);
	/// Whether the focused client is floating.
	Q_PROPERTY(bool floating READ floating NOTIFY floatingChanged);
	/// Tag state list. Length equals @@DwlIpc.tagCount.
	Q_PROPERTY(QList<qs::dwl::DwlTag*> tags READ tags NOTIFY tagsChanged);
	/// Keyboard layout.
	Q_PROPERTY(QString kbLayout READ kbLayout NOTIFY kbLayoutChanged);

public:
	explicit DwlIpcOutput(::zdwl_ipc_output_v2* handle, QString name, QObject* parent = nullptr);

	~DwlIpcOutput() override;

	DwlIpcOutput(const DwlIpcOutput&) = delete;
	DwlIpcOutput& operator=(const DwlIpcOutput&) = delete;
	DwlIpcOutput(DwlIpcOutput&&) = delete;
	DwlIpcOutput& operator=(DwlIpcOutput&&) = delete;

	[[nodiscard]] bool active() const;
	[[nodiscard]] quint32 layoutIndex() const;
	[[nodiscard]] QString layoutSymbol() const;
	[[nodiscard]] bool floating() const;
	[[nodiscard]] QString kbLayout() const;
	[[nodiscard]] QList<DwlTag*> tags() const;
	[[nodiscard]] const QString& outputName() const;

	/// Set active tags.
	Q_INVOKABLE void setTags(quint32 tagmask, quint32 toggleTagset = 0);
	/// Set focused client tags.
	Q_INVOKABLE void setClientTags(quint32 andTags, quint32 xorTags);
	/// Select layout by index (from @@DwlIpc.layouts).
	Q_INVOKABLE void setLayout(quint32 index);

	void initTags(quint32 count);

signals:
	void activeChanged();
	void layoutIndexChanged();
	void layoutSymbolChanged();
	void fullscreenChanged();
	void floatingChanged();
	void toggleVisibility();
	void kbLayoutChanged();
	void tagsChanged();
	/// Emitted when all double-buffered state for this frame has been committed.
	void frame();

protected:
	void zdwl_ipc_output_v2_toggle_visibility() override;
	void zdwl_ipc_output_v2_active(uint32_t active) override;
	void zdwl_ipc_output_v2_layout(uint32_t layout) override;
	void zdwl_ipc_output_v2_layout_symbol(const QString& layout) override;
	void zdwl_ipc_output_v2_frame() override;
	void zdwl_ipc_output_v2_floating(uint32_t isFloating) override;
	void zdwl_ipc_output_v2_kb_layout(const QString& kbLayout) override;

	void
	zdwl_ipc_output_v2_tag(uint32_t tag, uint32_t state, uint32_t clients, uint32_t focused) override;

private:
	QString mOutputName;

	// Committed state, updated atomically on frame.
	bool mActive = false;
	QList<DwlTag*> mTags;
	quint32 mLayoutIndex = 0;
	QString mLayoutSymbol;
	QString mKbLayout;
	bool mFloating = false;

	// Pending state accumulated between frame events.
	struct {
		bool active = false;
		quint32 layoutIndex = 0;
		QString layoutSymbol;
		QString kbLayout;
		bool floating = false;
		bool hasActive : 1 = false;
		bool hasLayoutIndex : 1 = false;
		bool hasLayoutSymbol : 1 = false;
		bool hasFloating : 1 = false;
		bool hasKbLayout : 1 = false;
	} mPending;
};

} // namespace qs::dwl
