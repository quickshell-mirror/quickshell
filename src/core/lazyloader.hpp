#pragma once

#include <QtQml/qqmlcomponent.h>
#include <qobject.h>
#include <qqmlincubator.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "incubator.hpp"
#include "reload.hpp"

///! Asynchronous component loader.
/// The LazyLoader can be used to prepare components that don't need to be
/// created immediately, such as windows that aren't visible until triggered
/// by another action. It works on creating the component in the gaps between
/// frame rendering to prevent blocking the interface thread.
/// It can also be used to preserve memory by loading components only
/// when you need them and unloading them afterward.
///
/// Note that when reloading the UI due to changes, lazy loaders will always
/// load synchronously so windows can be reused.
///
/// #### Example
/// The following example creates a PopupWindow asynchronously as the bar loads.
/// This means the bar can be shown onscreen before the popup is ready, however
/// trying to show the popup before it has finished loading in the background
/// will cause the UI thread to block.
///
/// ```qml
/// import QtQuick
/// import QtQuick.Controls
/// import Quickshell
///
/// ShellRoot {
///   PanelWindow {
///     id: window
///     height: 50
///
///     anchors {
///       bottom: true
///       left: true
///       right: true
///     }
///
///     LazyLoader {
///       id: popupLoader
///
///       // start loading immediately
///       loading: true
///
///       // this window will be loaded in the background during spare
///       // frame time unless active is set to true, where it will be
///       // loaded in the foreground
///       PopupWindow {
///         // position the popup above the button
///         parentWindow: window
///         relativeX: window.width / 2 - width / 2
///         relativeY: -height
///
///         // some heavy component here
///
///         width: 200
///         height: 200
///       }
///     }
///
///     Button {
///       anchors.centerIn: parent
///       text: "show popup"
///
///       // accessing popupLoader.item will force the loader to
///       // finish loading on the UI thread if it isn't finished yet.
///       onClicked: popupLoader.item.visible = !popupLoader.item.visible
///     }
///   }
/// }
/// ```
///
/// > [!WARNING] Components that internally load other components must explicitly
/// > support asynchronous loading to avoid blocking.
/// >
/// > Notably, @@Variants does not corrently support asynchronous
/// > loading, meaning using it inside a LazyLoader will block similarly to not
/// > having a loader to start with.
///
/// > [!WARNING] LazyLoaders do not start loading before the first window is created,
/// > meaning if you create all windows inside of lazy loaders, none of them will ever load.
class LazyLoader: public Reloadable {
	Q_OBJECT;
	/// The fully loaded item if the loader is @@loading or @@active, or `null`
	/// if neither @@loading nor @@active.
	///
	/// Note that the item is owned by the LazyLoader, and destroying the LazyLoader
	/// will destroy the item.
	///
	/// > [!WARNING] If you access the `item` of a loader that is currently loading,
	/// > it will block as if you had set `active` to true immediately beforehand.
	/// >
	/// > You can instead set @@loading and listen to @@activeChanged(s) signal to
	/// > ensure loading happens asynchronously.
	Q_PROPERTY(QObject* item READ item NOTIFY itemChanged);
	/// If the loader is actively loading.
	///
	/// If the component is not loaded, setting this property to true will start
	/// loading it asynchronously. If the component is already loaded, setting
	/// this property has no effect.
	///
	/// See also: @@activeAsync.
	Q_PROPERTY(bool loading READ isLoading WRITE setLoading NOTIFY loadingChanged);
	/// If the component is fully loaded.
	///
	/// Setting this property to `true` will force the component to load to completion,
	/// blocking the UI, and setting it to `false` will destroy the component, requiring
	/// it to be loaded again.
	///
	/// See also: @@activeAsync.
	Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged);
	/// If the component is fully loaded.
	///
	/// Setting this property to true will asynchronously load the component similarly to
	/// @@loading. Reading it or setting it to false will behanve
	/// the same as @@active.
	Q_PROPERTY(bool activeAsync READ isActive WRITE setActiveAsync NOTIFY activeChanged);
	/// The component to load. Mutually exclusive to @@source.
	Q_PROPERTY(QQmlComponent* component READ component WRITE setComponent NOTIFY componentChanged);
	/// The URI to load the component from. Mutually exclusive to @@component.
	Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged);
	Q_CLASSINFO("DefaultProperty", "component");
	QML_ELEMENT;

public:
	void onReload(QObject* oldInstance) override;

	[[nodiscard]] bool isActive() const;
	void setActive(bool active);
	void setActiveAsync(bool active);

	[[nodiscard]] bool isLoading() const;
	void setLoading(bool loading);

	[[nodiscard]] QObject* item();
	void setItem(QObject* item);

	[[nodiscard]] QQmlComponent* component() const;
	void setComponent(QQmlComponent* component);

	[[nodiscard]] QString source() const;
	void setSource(QString source);

signals:
	void activeChanged();
	void loadingChanged();
	void itemChanged();
	void sourceChanged();
	void componentChanged();

private slots:
	void onIncubationCompleted();
	void onIncubationFailed();
	void onComponentDestroyed();

private:
	void incubateIfReady(bool overrideReloadCheck = false);
	void waitForObjectCreation();

	bool targetLoading = false;
	bool targetActive = false;
	QObject* mItem = nullptr;
	QString mSource;
	QQmlComponent* mComponent = nullptr;
	QsQmlIncubator* incubator = nullptr;
	bool cleanupComponent = false;
};
