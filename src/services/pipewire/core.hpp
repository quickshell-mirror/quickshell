#pragma once

#include <pipewire/context.h>
#include <pipewire/core.h>
#include <pipewire/loop.h>
#include <pipewire/proxy.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qsocketnotifier.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <spa/utils/hook.h>

namespace qs::service::pipewire {

class SpaHook {
public:
	explicit SpaHook();

	void remove();
	spa_hook hook;
};

class PwCore: public QObject {
	Q_OBJECT;

public:
	explicit PwCore(QObject* parent = nullptr);
	~PwCore() override;
	Q_DISABLE_COPY_MOVE(PwCore);

	[[nodiscard]] bool isValid() const;
	[[nodiscard]] qint32 sync(quint32 id) const;

	pw_loop* loop = nullptr;
	pw_context* context = nullptr;
	pw_core* core = nullptr;

signals:
	void polled();
	void synced(quint32 id, qint32 seq);

private slots:
	void poll();

private:
	static const pw_core_events EVENTS;

	static void onSync(void* data, quint32 id, qint32 seq);

	QSocketNotifier notifier;
	SpaHook listener;
};

template <typename T>
class PwObject {
public:
	explicit PwObject(T* object = nullptr): object(object) {}
	~PwObject() { pw_proxy_destroy(reinterpret_cast<pw_proxy*>(this->object)); }

	Q_DISABLE_COPY_MOVE(PwObject);

	T* object;
};

} // namespace qs::service::pipewire
