#pragma once
#include <qmenu.h>
#include <qpoint.h>

namespace qs::menu::platform {

class PlatformMenuQMenu: public QMenu {
public:
	explicit PlatformMenuQMenu() = default;
	~PlatformMenuQMenu() override;
	Q_DISABLE_COPY_MOVE(PlatformMenuQMenu);

	void setVisible(bool visible) override;

	PlatformMenuQMenu* containingMenu = nullptr;
	QPoint targetPosition;
};

} // namespace qs::menu::platform
