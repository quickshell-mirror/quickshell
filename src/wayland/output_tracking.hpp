#pragma once

#include <qlist.h>
#include <qobject.h>
#include <qscreen.h>
#include <qtmetamacros.h>

struct wl_output;

namespace qs::wayland {

class WlOutputTracker: public QObject {
	Q_OBJECT;

public:
	[[nodiscard]] const QList<QScreen*>& screens() const { return this->mScreens; }

signals:
	void screenAdded(QScreen* screen);
	void screenRemoved(QScreen* screen);

public slots:
	void addOutput(::wl_output* output);
	void removeOutput(::wl_output* output);

private slots:
	void onQScreenAdded(QScreen* screen);

private:
	QList<QScreen*> mScreens;
	QList<::wl_output*> mOutputs;
};

} // namespace qs::wayland
