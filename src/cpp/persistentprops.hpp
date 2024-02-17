#pragma once

#include <qobject.h>
#include <qqmlintegration.h>

#include "reload.hpp"

///! Object that holds properties that can persist across a config reload.
/// PersistentProperties holds properties declated in it across a reload, which is
/// often useful for things like keeping expandable popups open and styling them.
///
/// Below is an example of using `PersistentProperties` to keep track of the state
/// of an expandable panel. When the configuration is reloaded, the `expanderOpen` property
/// will be saved and the expandable panel will stay in the open/closed state.
///
/// ```qml
/// PersistentProperties {
///   id: persist
///   reloadableId: "persistedStates"
///
///   property bool expanderOpen: false
/// }
///
/// Button {
///   id: expanderButton
///   anchors.centerIn: parent
///   text: "toggle expander"
///   onClicked: persist.expanderOpen = !persist.expanderOpen
/// }
///
/// Rectangle {
///   anchors.top: expanderButton.bottom
///   anchors.left: expanderButton.left
///   anchors.right: expanderButton.right
///   height: 100
///
///   color: "lightblue"
///   visible: persist.expanderOpen
/// }
/// ```
class PersistentProperties: public Reloadable {
	Q_OBJECT;
	QML_ELEMENT;

public:
	PersistentProperties(QObject* parent = nullptr): Reloadable(parent) {}

	void onReload(QObject* oldInstance) override;

signals:
	/// Called every time the reload stage completes.
	/// Will be called every time, including when nothing was loaded from an old instance.
	void loaded();
	/// Called every time the properties are reloaded.
	/// Will not be called if no old instance was loaded.
	void reloaded();
};
