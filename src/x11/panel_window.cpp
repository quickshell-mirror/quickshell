#include "panel_window.hpp"
#include <array>
#include <map>

#include <qevent.h>
#include <qlist.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qqmlengine.h>
#include <qqmllist.h>
#include <qquickwindow.h>
#include <qscreen.h>
#include <qtenvironmentvariables.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <xcb/xproto.h>

#include "../core/generation.hpp"
#include "../core/qmlscreen.hpp"
#include "../core/types.hpp"
#include "../window/panelinterface.hpp"
#include "../window/proxywindow.hpp"
#include "util.hpp"

class XPanelStack {
public:
	static XPanelStack* instance() {
		static XPanelStack* stack = nullptr; // NOLINT

		if (stack == nullptr) {
			stack = new XPanelStack();
		}

		return stack;
	}

	[[nodiscard]] const QList<XPanelWindow*>& panels(XPanelWindow* panel) {
		return this->mPanels[EngineGeneration::findObjectGeneration(panel)];
	}

	void addPanel(XPanelWindow* panel) {
		panel->engineGeneration = EngineGeneration::findObjectGeneration(panel);
		auto& panels = this->mPanels[panel->engineGeneration];
		if (!panels.contains(panel)) {
			panels.push_back(panel);
		}
	}

	void removePanel(XPanelWindow* panel) {
		if (!panel->engineGeneration) return;

		auto& panels = this->mPanels[panel->engineGeneration];
		if (panels.removeOne(panel)) {
			if (panels.isEmpty()) {
				this->mPanels.erase(panel->engineGeneration);
			}

			// from the bottom up, update all panels
			for (auto* panel: panels) {
				panel->updateDimensions();
			}
		}
	}

	void updateLowerDimensions(XPanelWindow* exclude) {
		if (!exclude->engineGeneration) return;
		auto& panels = this->mPanels[exclude->engineGeneration];

		// update all panels lower than the one we start from
		auto found = false;
		for (auto* panel: panels) {
			if (panel == exclude) found = true;
			else if (found) panel->updateDimensions(false);
		}
	}

private:
	std::map<EngineGeneration*, QList<XPanelWindow*>> mPanels;
};

bool XPanelEventFilter::eventFilter(QObject* watched, QEvent* event) {
	if (event->type() == QEvent::PlatformSurface) {
		auto* surfaceEvent = static_cast<QPlatformSurfaceEvent*>(event); // NOLINT

		if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
			emit this->surfaceCreated();
		}
	}

	return this->QObject::eventFilter(watched, event);
}

XPanelWindow::XPanelWindow(QObject* parent): ProxyWindowBase(parent) {
	QObject::connect(
	    &this->eventFilter,
	    &XPanelEventFilter::surfaceCreated,
	    this,
	    &XPanelWindow::xInit
	);

	this->bcExclusiveZone.setBinding([this]() -> qint32 {
		switch (this->bExclusionMode.value()) {
		case ExclusionMode::Ignore: return 0;
		case ExclusionMode::Normal: return this->bExclusiveZone;
		case ExclusionMode::Auto:
			auto edge = this->bcExclusionEdge.value();
			auto margins = this->bMargins.value();

			if (edge == Qt::TopEdge || edge == Qt::BottomEdge) {
				return this->bImplicitHeight + margins.top + margins.bottom;
			} else if (edge == Qt::LeftEdge || edge == Qt::RightEdge) {
				return this->bImplicitWidth + margins.left + margins.right;
			} else {
				return 0;
			}
		}
	});

	this->bcExclusionEdge.setBinding([this] { return this->bAnchors.value().exclusionEdge(); });
}

XPanelWindow::~XPanelWindow() { XPanelStack::instance()->removePanel(this); }

void XPanelWindow::connectWindow() {
	this->ProxyWindowBase::connectWindow();

	this->window->installEventFilter(&this->eventFilter);
	this->updateScreen();

	QObject::connect(
	    this->window,
	    &QQuickWindow::visibleChanged,
	    this,
	    &XPanelWindow::updatePanelStack
	);

	// qt overwrites _NET_WM_STATE, so we have to use the qt api
	// QXcbWindow::WindowType::Dock in qplatformwindow_p.h
	// see QXcbWindow::setWindowFlags in qxcbwindow.cpp
	this->window->setProperty("_q_xcb_wm_window_type", 0x000004);

	// at least one flag needs to change for the above property to apply
	this->window->setFlag(Qt::FramelessWindowHint);
	this->updateAboveWindows();
	this->updateFocusable();

	if (this->window->handle() != nullptr) {
		this->xInit();
		this->updatePanelStack();
	}
}

void XPanelWindow::trySetWidth(qint32 implicitWidth) {
	// only update the actual size if not blocked by anchors
	if (!this->bAnchors.value().horizontalConstraint()) {
		this->ProxyWindowBase::trySetWidth(implicitWidth);
		this->updateDimensions();
	}
}

void XPanelWindow::trySetHeight(qint32 implicitHeight) {
	// only update the actual size if not blocked by anchors
	if (!this->bAnchors.value().verticalConstraint()) {
		this->ProxyWindowBase::trySetHeight(implicitHeight);
		this->updateDimensions();
	}
}

void XPanelWindow::setScreen(QuickshellScreenInfo* screen) {
	this->ProxyWindowBase::setScreen(screen);
	this->updateScreen();
}

void XPanelWindow::xInit() {
	if (this->window == nullptr || this->window->handle() == nullptr) return;
	this->updateDimensions();

	auto* conn = x11Connection();

	// Stick to every workspace
	auto desktop = 0xffffffff;
	xcb_change_property(
	    conn,
	    XCB_PROP_MODE_REPLACE,
	    this->window->winId(),
	    XAtom::_NET_WM_DESKTOP.atom(),
	    XCB_ATOM_CARDINAL,
	    32,
	    1,
	    &desktop
	);
}

void XPanelWindow::updateScreen() {
	auto* newScreen =
	    this->mScreen ? this->mScreen : (this->window ? this->window->screen() : nullptr);

	if (newScreen == this->mTrackedScreen) return;

	if (this->mTrackedScreen != nullptr) {
		QObject::disconnect(this->mTrackedScreen, nullptr, this, nullptr);
	}

	this->mTrackedScreen = newScreen;

	if (this->mTrackedScreen != nullptr) {
		QObject::connect(
		    this->mTrackedScreen,
		    &QScreen::geometryChanged,
		    this,
		    &XPanelWindow::updateDimensionsSlot
		);

		QObject::connect(
		    this->mTrackedScreen,
		    &QScreen::virtualGeometryChanged,
		    this,
		    &XPanelWindow::onScreenVirtualGeometryChanged
		);
	}

	this->updateDimensions();
}

// For some reason this gets sent multiple times with the same value.
void XPanelWindow::onScreenVirtualGeometryChanged() {
	auto geometry = this->mTrackedScreen->virtualGeometry();
	if (geometry != this->lastScreenVirtualGeometry) {
		this->lastScreenVirtualGeometry = geometry;
		this->updateStrut(false);
	}
}

void XPanelWindow::updateDimensionsSlot() { this->updateDimensions(); }

void XPanelWindow::updateDimensions(bool propagate) {
	if (this->window == nullptr || this->window->handle() == nullptr
	    || this->mTrackedScreen == nullptr)
		return;

	auto screenGeometry = this->mTrackedScreen->geometry();

	if (this->bExclusionMode != ExclusionMode::Ignore) {
		for (auto* panel: XPanelStack::instance()->panels(this)) {
			// we only care about windows below us
			if (panel == this) break;

			// we only care about windows in the same layer
			if (panel->bAboveWindows != this->bAboveWindows) continue;

			if (panel->mTrackedScreen != this->mTrackedScreen) continue;

			auto edge = panel->bcExclusionEdge.value();
			auto exclusiveZone = panel->bcExclusiveZone.value();

			screenGeometry.adjust(
			    edge == Qt::LeftEdge ? exclusiveZone : 0,
			    edge == Qt::TopEdge ? exclusiveZone : 0,
			    edge == Qt::RightEdge ? -exclusiveZone : 0,
			    edge == Qt::BottomEdge ? -exclusiveZone : 0
			);
		}
	}

	auto geometry = QRect();

	auto anchors = this->bAnchors.value();
	auto margins = this->bMargins.value();

	if (anchors.horizontalConstraint()) {
		geometry.setX(screenGeometry.x() + margins.left);
		geometry.setWidth(screenGeometry.width() - margins.left - margins.right);
	} else {
		if (anchors.mLeft) {
			geometry.setX(screenGeometry.x() + margins.left);
		} else if (anchors.mRight) {
			geometry.setX(
			    screenGeometry.x() + screenGeometry.width() - this->implicitWidth() - margins.right
			);
		} else {
			geometry.setX(screenGeometry.x() + screenGeometry.width() / 2 - this->implicitWidth() / 2);
		}

		geometry.setWidth(this->implicitWidth());
	}

	if (anchors.verticalConstraint()) {
		geometry.setY(screenGeometry.y() + margins.top);
		geometry.setHeight(screenGeometry.height() - margins.top - margins.bottom);
	} else {
		if (anchors.mTop) {
			geometry.setY(screenGeometry.y() + margins.top);
		} else if (anchors.mBottom) {
			geometry.setY(
			    screenGeometry.y() + screenGeometry.height() - this->implicitHeight() - margins.bottom
			);
		} else {
			geometry.setY(screenGeometry.y() + screenGeometry.height() / 2 - this->implicitHeight() / 2);
		}

		geometry.setHeight(this->implicitHeight());
	}

	this->window->setGeometry(geometry);
	this->updateStrut(propagate);

	// AwesomeWM incorrectly repositions the window without this.
	// See https://github.com/polybar/polybar/blob/f0f9563ecf39e78ba04cc433cb7b38a83efde473/src/components/bar.cpp#L666
	QTimer::singleShot(0, this, [this, geometry]() {
		// forces second call not to be discarded as duplicate
		this->window->setGeometry({0, 0, 0, 0});
		this->window->setGeometry(geometry);
	});
}

void XPanelWindow::updatePanelStack() {
	if (this->window->isVisible()) {
		XPanelStack::instance()->addPanel(this);
	} else {
		XPanelStack::instance()->removePanel(this);
	}
}

// Disable xinerama structs to break multi monitor configurations with bad WMs less.
// Usually this results in one monitor at the top left corner of the root window working
// perfectly and all others being broken semi randomly.
static bool XINERAMA_STRUTS = qEnvironmentVariableIsEmpty("QS_NO_XINERAMA_STRUTS"); // NOLINT

void XPanelWindow::updateStrut(bool propagate) {
	if (this->window == nullptr || this->window->handle() == nullptr) return;
	auto* conn = x11Connection();

	auto edge = this->bcExclusionEdge.value();
	auto exclusiveZone = this->bcExclusiveZone.value();

	if (edge == 0 || this->bExclusionMode == ExclusionMode::Ignore) {
		xcb_delete_property(conn, this->window->winId(), XAtom::_NET_WM_STRUT.atom());
		xcb_delete_property(conn, this->window->winId(), XAtom::_NET_WM_STRUT_PARTIAL.atom());
		return;
	}

	auto rootGeometry = this->window->screen()->virtualGeometry();
	auto screenGeometry = this->window->screen()->geometry();
	auto horizontal = edge == Qt::LeftEdge || edge == Qt::RightEdge;

	if (XINERAMA_STRUTS) {
		switch (edge) {
		case Qt::LeftEdge: exclusiveZone += screenGeometry.left(); break;
		case Qt::RightEdge: exclusiveZone += rootGeometry.right() - screenGeometry.right(); break;
		case Qt::TopEdge: exclusiveZone += screenGeometry.top(); break;
		case Qt::BottomEdge: exclusiveZone += rootGeometry.bottom() - screenGeometry.bottom(); break;
		default: break;
		}
	}

	quint32 side = -1;

	switch (edge) {
	case Qt::LeftEdge: side = 0; break;
	case Qt::RightEdge: side = 1; break;
	case Qt::TopEdge: side = 2; break;
	case Qt::BottomEdge: side = 3; break;
	}

	auto data = std::array<quint32, 12>();
	data[side] = exclusiveZone;

	auto start = horizontal ? this->window->y() : this->window->x();

	data[4 + side * 2] = start;
	data[5 + side * 2] = start + (horizontal ? this->window->height() : this->window->width());

	xcb_change_property(
	    conn,
	    XCB_PROP_MODE_REPLACE,
	    this->window->winId(),
	    XAtom::_NET_WM_STRUT.atom(),
	    XCB_ATOM_CARDINAL,
	    32,
	    4,
	    data.data()
	);

	xcb_change_property(
	    conn,
	    XCB_PROP_MODE_REPLACE,
	    this->window->winId(),
	    XAtom::_NET_WM_STRUT_PARTIAL.atom(),
	    XCB_ATOM_CARDINAL,
	    32,
	    12,
	    data.data()
	);

	if (propagate) XPanelStack::instance()->updateLowerDimensions(this);
}

void XPanelWindow::updateAboveWindows() {
	if (this->window == nullptr) return;

	auto above = this->bAboveWindows.value();
	this->window->setFlag(Qt::WindowStaysOnBottomHint, !above);
	this->window->setFlag(Qt::WindowStaysOnTopHint, above);
}

void XPanelWindow::updateFocusable() {
	if (this->window == nullptr) return;
	this->window->setFlag(Qt::WindowDoesNotAcceptFocus, !this->bFocusable);
}

// XPanelInterface

XPanelInterface::XPanelInterface(QObject* parent)
    : PanelWindowInterface(parent)
    , panel(new XPanelWindow(this)) {
	this->connectSignals();

	// clang-format off
	QObject::connect(this->panel, &XPanelWindow::anchorsChanged, this, &XPanelInterface::anchorsChanged);
	QObject::connect(this->panel, &XPanelWindow::marginsChanged, this, &XPanelInterface::marginsChanged);
	QObject::connect(this->panel, &XPanelWindow::exclusiveZoneChanged, this, &XPanelInterface::exclusiveZoneChanged);
	QObject::connect(this->panel, &XPanelWindow::exclusionModeChanged, this, &XPanelInterface::exclusionModeChanged);
	QObject::connect(this->panel, &XPanelWindow::aboveWindowsChanged, this, &XPanelInterface::aboveWindowsChanged);
	QObject::connect(this->panel, &XPanelWindow::focusableChanged, this, &XPanelInterface::focusableChanged);
	// clang-format on
}

void XPanelInterface::onReload(QObject* oldInstance) {
	QQmlEngine::setContextForObject(this->panel, QQmlEngine::contextForObject(this));

	auto* old = qobject_cast<XPanelInterface*>(oldInstance);
	this->panel->reload(old != nullptr ? old->panel : nullptr);
}

ProxyWindowBase* XPanelInterface::proxyWindow() const { return this->panel; }

// NOLINTBEGIN
#define proxyPair(type, get, set)                                                                  \
	type XPanelInterface::get() const { return this->panel->get(); }                                 \
	void XPanelInterface::set(type value) { this->panel->set(value); }

proxyPair(Anchors, anchors, setAnchors);
proxyPair(Margins, margins, setMargins);
proxyPair(qint32, exclusiveZone, setExclusiveZone);
proxyPair(ExclusionMode::Enum, exclusionMode, setExclusionMode);
proxyPair(bool, focusable, setFocusable);
proxyPair(bool, aboveWindows, setAboveWindows);

#undef proxyPair
// NOLINTEND
