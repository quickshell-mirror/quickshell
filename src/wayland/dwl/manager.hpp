#pragma once

#include <qhash.h>
#include <qlist.h>
#include <qobject.h>
#include <qscreen.h>
#include <qstringlist.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwayland-dwl-ipc-unstable-v2.h>
#include <qwaylandclientextension.h>
#include <wayland-client-protocol.h>

#include "output.hpp"

namespace qs::dwl {

class DwlIpcManager
    : public QWaylandClientExtensionTemplate<DwlIpcManager>
    , public QtWayland::zdwl_ipc_manager_v2 {
	Q_OBJECT;

public:
	explicit DwlIpcManager();

	[[nodiscard]] quint32 tagCount() const;
	[[nodiscard]] QStringList layouts() const;
	[[nodiscard]] QList<DwlIpcOutput*> outputs() const;

	DwlIpcOutput* bindOutput(struct wl_output* wlOutput, const QString& name);
	void removeOutput(struct wl_output* wlOutput);

	static DwlIpcManager* instance();

signals:
	void tagCountChanged();
	void layoutsChanged();
	void outputAdded(DwlIpcOutput* output);
	void outputRemoved(DwlIpcOutput* output);

protected:
	void zdwl_ipc_manager_v2_tags(uint32_t amount) override;
	void zdwl_ipc_manager_v2_layout(const QString& name) override;

private slots:
	void onScreenAdded(QScreen* screen);
	void onScreenRemoved(QScreen* screen);

private:
	quint32 mTagCount = 0;
	QStringList mLayouts;
	QList<DwlIpcOutput*> mOutputs;
	QHash<struct wl_output*, DwlIpcOutput*> mOutputMap;
};

} // namespace qs::dwl
