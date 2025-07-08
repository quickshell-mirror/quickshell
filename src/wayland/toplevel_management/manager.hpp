#pragma once

#include <qcontainerfwd.h>
#include <qloggingcategory.h>
#include <qtmetamacros.h>
#include <qwayland-wlr-foreign-toplevel-management-unstable-v1.h>
#include <qwaylandclientextension.h>

#include "../../core/logcat.hpp"
#include "wayland-wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"

namespace qs::wayland::toplevel_management::impl {

class ToplevelHandle;

QS_DECLARE_LOGGING_CATEGORY(logToplevelManagement);

class ToplevelManager
    : public QWaylandClientExtensionTemplate<ToplevelManager>
    , public QtWayland::zwlr_foreign_toplevel_manager_v1 {
	Q_OBJECT;

public:
	[[nodiscard]] bool available() const;
	[[nodiscard]] const QVector<ToplevelHandle*>& readyToplevels() const;
	[[nodiscard]] ToplevelHandle* handleFor(::zwlr_foreign_toplevel_handle_v1* toplevel);

	static ToplevelManager* instance();

signals:
	void toplevelReady(ToplevelHandle* toplevel);

protected:
	explicit ToplevelManager();

	void zwlr_foreign_toplevel_manager_v1_toplevel(::zwlr_foreign_toplevel_handle_v1* toplevel
	) override;

private slots:
	void onToplevelReady();
	void onToplevelClosed();

private:
	QVector<ToplevelHandle*> mToplevels;
	QVector<ToplevelHandle*> mReadyToplevels;
};

} // namespace qs::wayland::toplevel_management::impl
