#include "proxiedwindow.hpp"

#include <qcoreapplication.h>
#include <qevent.h>
#include <qpoint.h>
#include <qquickwindow.h>
#include <qtest.h>
#include <qtestcase.h>

#include "../proxywindow.hpp"

void TestProxiedWindow::contextMenuWithoutPositionAndFocusItem() { // NOLINT
	// Windows that have never become the active keyboard-focus window - such
	// as layer-shell surfaces with keyboard focus disabled - have no active
	// focus item. Qt positions context menu events that carry no position at
	// the active focus item without a null check, which crashed delivery
	// inside QQuickDeliveryAgentPrivate::contextMenuTargets(). The window
	// consumes such events instead of forwarding them to Qt.
	auto window = ProxyWindowBase();
	window.reload();
	auto* backing = window.backingWindow();
	QVERIFY(backing != nullptr);

	QCOMPARE(backing->activeFocusItem(), nullptr);

	QContextMenuEvent event(QContextMenuEvent::Mouse, QPoint(), QPoint(), Qt::NoModifier);
	// The window consumes the event before it reaches Qt's delivery agent.
	// Without the guard this sendEvent crashes inside
	// QQuickDeliveryAgentPrivate::contextMenuTargets().
	QVERIFY(QCoreApplication::sendEvent(backing, &event));
}

QTEST_MAIN(TestProxiedWindow);

#include "proxiedwindow.moc"
