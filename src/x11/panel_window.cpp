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
#include <qtimer.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <xcb/xproto.h>

#include "../core/generation.hpp"
#include "../core/panelinterface.hpp"
#include "../core/proxywindow.hpp"
#include "../core/qmlscreen.hpp"
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
		auto& panels = this->mPanels[EngineGeneration::findObjectGeneration(panel)];
		if (!panels.contains(panel)) {
			panels.push_back(panel);
		}
	}

	void removePanel(XPanelWindow* panel) {
		auto& panels = this->mPanels[EngineGeneration::findObjectGeneration(panel)];
		if (panels.removeOne(panel)) {
			if (panels.isEmpty()) {
				this->mPanels.erase(EngineGeneration::findObjectGeneration(panel));
			}

			// from the bottom up, update all panels
			for (auto* panel: panels) {
				panel->updateDimensions();
			}
		}
	}

	void updateLowerDimensions(XPanelWindow* exclude) {
		auto& panels = this->mPanels[EngineGeneration::findObjectGeneration(exclude)];

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
}

XPanelWindow::~XPanelWindow() { XPanelStack::instance()->removePanel(this); }

void XPanelWindow::connectWindow() {
	this->ProxyWindowBase::connectWindow();

	this->window->installEventFilter(&this->eventFilter);
	this->connectScreen();

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

void XPanelWindow::setWidth(qint32 width) {
	this->mWidth = width;

	// only update the actual size if not blocked by anchors
	if (!this->mAnchors.horizontalConstraint()) {
		this->ProxyWindowBase::setWidth(width);
		this->updateDimensions();
	}
}

void XPanelWindow::setHeight(qint32 height) {
	this->mHeight = height;

	// only update the actual size if not blocked by anchors
	if (!this->mAnchors.verticalConstraint()) {
		this->ProxyWindowBase::setHeight(height);
		this->updateDimensions();
	}
}

void XPanelWindow::setScreen(QuickshellScreenInfo* screen) {
	this->ProxyWindowBase::setScreen(screen);
	this->connectScreen();
}

Anchors XPanelWindow::anchors() const { return this->mAnchors; }

void XPanelWindow::setAnchors(Anchors anchors) {
	if (this->mAnchors == anchors) return;
	this->mAnchors = anchors;
	this->updateDimensions();
	emit this->anchorsChanged();
}

qint32 XPanelWindow::exclusiveZone() const { return this->mExclusiveZone; }

void XPanelWindow::setExclusiveZone(qint32 exclusiveZone) {
	if (this->mExclusiveZone == exclusiveZone) return;
	this->mExclusiveZone = exclusiveZone;
	this->setExclusionMode(ExclusionMode::Normal);
	this->updateStrut();
	emit this->exclusiveZoneChanged();
}

ExclusionMode::Enum XPanelWindow::exclusionMode() const { return this->mExclusionMode; }

void XPanelWindow::setExclusionMode(ExclusionMode::Enum exclusionMode) {
	if (this->mExclusionMode == exclusionMode) return;
	this->mExclusionMode = exclusionMode;
	this->updateStrut();
	emit this->exclusionModeChanged();
}

Margins XPanelWindow::margins() const { return this->mMargins; }

void XPanelWindow::setMargins(Margins margins) {
	if (this->mMargins == margins) return;
	this->mMargins = margins;
	this->updateDimensions();
	emit this->marginsChanged();
}

bool XPanelWindow::aboveWindows() const { return this->mAboveWindows; }

void XPanelWindow::setAboveWindows(bool aboveWindows) {
	if (this->mAboveWindows == aboveWindows) return;
	this->mAboveWindows = aboveWindows;
	this->updateAboveWindows();
	emit this->aboveWindowsChanged();
}

bool XPanelWindow::focusable() const { return this->mFocusable; }

void XPanelWindow::setFocusable(bool focusable) {
	if (this->mFocusable == focusable) return;
	this->mFocusable = focusable;
	this->updateFocusable();
	emit this->focusableChanged();
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

void XPanelWindow::connectScreen() {
	if (this->mTrackedScreen != nullptr) {
		QObject::disconnect(this->mTrackedScreen, nullptr, this, nullptr);
	}

	this->mTrackedScreen = this->mScreen;

	if (this->mTrackedScreen != nullptr) {
		QObject::connect(
		    this->mTrackedScreen,
		    &QScreen::geometryChanged,
		    this,
		    &XPanelWindow::updateDimensionsSlot
		);
	}

	this->updateDimensions();
}

void XPanelWindow::updateDimensionsSlot() { this->updateDimensions(); }

void XPanelWindow::updateDimensions(bool propagate) {
	if (this->window == nullptr || this->window->handle() == nullptr || this->mScreen == nullptr)
		return;

	auto screenGeometry = this->mScreen->geometry();

	if (this->mExclusionMode != ExclusionMode::Ignore) {
		for (auto* panel: XPanelStack::instance()->panels(this)) {
			// we only care about windows below us
			if (panel == this) break;

			// we only care about windows in the same layer
			if (panel->mAboveWindows != this->mAboveWindows) continue;

			if (panel->mScreen != this->mScreen) continue;

			int side = -1;
			quint32 exclusiveZone = 0;
			panel->getExclusion(side, exclusiveZone);

			if (exclusiveZone == 0) continue;

			auto zone = static_cast<qint32>(exclusiveZone);

			screenGeometry.adjust(
			    side == 0 ? zone : 0,
			    side == 2 ? zone : 0,
			    side == 1 ? -zone : 0,
			    side == 3 ? -zone : 0
			);
		}
	}

	auto geometry = QRect();

	if (this->mAnchors.horizontalConstraint()) {
		geometry.setX(screenGeometry.x() + this->mMargins.mLeft);
		geometry.setWidth(screenGeometry.width() - this->mMargins.mLeft - this->mMargins.mRight);
	} else {
		if (this->mAnchors.mLeft) {
			geometry.setX(screenGeometry.x() + this->mMargins.mLeft);
		} else if (this->mAnchors.mRight) {
			geometry.setX(
			    screenGeometry.x() + screenGeometry.width() - this->mWidth - this->mMargins.mRight
			);
		} else {
			geometry.setX(screenGeometry.x() + screenGeometry.width() / 2 - this->mWidth / 2);
		}

		geometry.setWidth(this->mWidth);
	}

	if (this->mAnchors.verticalConstraint()) {
		geometry.setY(screenGeometry.y() + this->mMargins.mTop);
		geometry.setHeight(screenGeometry.height() - this->mMargins.mTop - this->mMargins.mBottom);
	} else {
		if (this->mAnchors.mTop) {
			geometry.setY(screenGeometry.y() + this->mMargins.mTop);
		} else if (this->mAnchors.mBottom) {
			geometry.setY(
			    screenGeometry.y() + screenGeometry.height() - this->mHeight - this->mMargins.mBottom
			);
		} else {
			geometry.setY(screenGeometry.y() + screenGeometry.height() / 2 - this->mHeight / 2);
		}

		geometry.setHeight(this->mHeight);
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

void XPanelWindow::getExclusion(int& side, quint32& exclusiveZone) {
	if (this->mExclusionMode == ExclusionMode::Ignore) {
		exclusiveZone = 0;
		return;
	}

	auto& anchors = this->mAnchors;
	if (anchors.mLeft || anchors.mRight || anchors.mTop || anchors.mBottom) {
		if (!anchors.horizontalConstraint()
		    && (anchors.verticalConstraint() || (!anchors.mTop && !anchors.mBottom)))
		{
			side = anchors.mLeft ? 0 : anchors.mRight ? 1 : -1;
		} else if (!anchors.verticalConstraint()
						 && (anchors.horizontalConstraint() || (!anchors.mLeft && !anchors.mRight)))
		{
			side = anchors.mTop ? 2 : anchors.mBottom ? 3 : -1;
		}
	}

	if (side == -1) return;

	auto autoExclude = this->mExclusionMode == ExclusionMode::Auto;

	if (autoExclude) {
		if (side == 0 || side == 1) {
			exclusiveZone = this->mWidth + (side == 0 ? this->mMargins.mLeft : this->mMargins.mRight);
		} else {
			exclusiveZone = this->mHeight + (side == 2 ? this->mMargins.mTop : this->mMargins.mBottom);
		}
	} else {
		exclusiveZone = this->mExclusiveZone;
	}
}

void XPanelWindow::updateStrut(bool propagate) {
	if (this->window == nullptr || this->window->handle() == nullptr) return;
	auto* conn = x11Connection();

	int side = -1;
	quint32 exclusiveZone = 0;

	this->getExclusion(side, exclusiveZone);

	if (side == -1 || this->mExclusionMode == ExclusionMode::Ignore) {
		xcb_delete_property(conn, this->window->winId(), XAtom::_NET_WM_STRUT.atom());
		xcb_delete_property(conn, this->window->winId(), XAtom::_NET_WM_STRUT_PARTIAL.atom());
		return;
	}

	// Due to missing headers it isn't even possible to do this right.
	// We assume a single xinerama monitor with a matching size root.
	auto screenGeometry = this->window->screen()->geometry();
	auto horizontal = side == 0 || side == 1;

	auto data = std::array<quint32, 12>();
	data[side] = exclusiveZone;

	auto start = horizontal ? screenGeometry.top() + this->window->y()
	                        : screenGeometry.left() + this->window->x();

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

	this->window->setFlag(Qt::WindowStaysOnBottomHint, !this->mAboveWindows);
	this->window->setFlag(Qt::WindowStaysOnTopHint, this->mAboveWindows);
}

void XPanelWindow::updateFocusable() {
	if (this->window == nullptr) return;
	this->window->setFlag(Qt::WindowDoesNotAcceptFocus, !this->mFocusable);
}

// XPanelInterface

XPanelInterface::XPanelInterface(QObject* parent)
    : PanelWindowInterface(parent)
    , panel(new XPanelWindow(this)) {

	// clang-format off
	QObject::connect(this->panel, &ProxyWindowBase::windowConnected, this, &XPanelInterface::windowConnected);
	QObject::connect(this->panel, &ProxyWindowBase::visibleChanged, this, &XPanelInterface::visibleChanged);
	QObject::connect(this->panel, &ProxyWindowBase::backerVisibilityChanged, this, &XPanelInterface::backingWindowVisibleChanged);
	QObject::connect(this->panel, &ProxyWindowBase::heightChanged, this, &XPanelInterface::heightChanged);
	QObject::connect(this->panel, &ProxyWindowBase::widthChanged, this, &XPanelInterface::widthChanged);
	QObject::connect(this->panel, &ProxyWindowBase::screenChanged, this, &XPanelInterface::screenChanged);
	QObject::connect(this->panel, &ProxyWindowBase::windowTransformChanged, this, &XPanelInterface::windowTransformChanged);
	QObject::connect(this->panel, &ProxyWindowBase::colorChanged, this, &XPanelInterface::colorChanged);
	QObject::connect(this->panel, &ProxyWindowBase::maskChanged, this, &XPanelInterface::maskChanged);

	// panel specific
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

QQmlListProperty<QObject> XPanelInterface::data() { return this->panel->data(); }
ProxyWindowBase* XPanelInterface::proxyWindow() const { return this->panel; }
QQuickItem* XPanelInterface::contentItem() const { return this->panel->contentItem(); }
bool XPanelInterface::isBackingWindowVisible() const { return this->panel->isVisibleDirect(); }

// NOLINTBEGIN
#define proxyPair(type, get, set)                                                                  \
	type XPanelInterface::get() const { return this->panel->get(); }                                 \
	void XPanelInterface::set(type value) { this->panel->set(value); }

proxyPair(bool, isVisible, setVisible);
proxyPair(qint32, width, setWidth);
proxyPair(qint32, height, setHeight);
proxyPair(QuickshellScreenInfo*, screen, setScreen);
proxyPair(QColor, color, setColor);
proxyPair(PendingRegion*, mask, setMask);

// panel specific
proxyPair(Anchors, anchors, setAnchors);
proxyPair(Margins, margins, setMargins);
proxyPair(qint32, exclusiveZone, setExclusiveZone);
proxyPair(ExclusionMode::Enum, exclusionMode, setExclusionMode);
proxyPair(bool, focusable, setFocusable);
proxyPair(bool, aboveWindows, setAboveWindows);

#undef proxyPair
// NOLINTEND
