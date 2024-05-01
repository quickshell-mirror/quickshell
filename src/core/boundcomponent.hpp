#pragma once

#include <qmetaobject.h>
#include <qobject.h>
#include <qqmlcomponent.h>
#include <qqmlparserstatus.h>
#include <qquickitem.h>
#include <qsignalmapper.h>
#include <qtmetamacros.h>

#include "incubator.hpp"

///! Component loader that allows setting initial properties.
/// Component loader that allows setting initial properties, primarily useful for
/// escaping cyclic dependency errors.
///
/// Properties defined on the BoundComponent will be applied to its loaded component,
/// including required properties, and will remain reactive. Functions created with
/// the names of signal handlers will also be attached to signals of the loaded component.
///
/// ```qml {filename="MyComponent.qml"}
/// MouseArea {
///   required property color color;
///   width: 100
///   height: 100
///
///   Rectangle {
///     anchors.fill: parent
///     color: parent.color
///   }
/// }
/// ```
///
/// ```qml
/// BoundComponent {
///   source: "MyComponent.qml"
///
///   // this is the same as assigning to `color` on MyComponent if loaded normally.
///   property color color: "red";
///
///   // this will be triggered when the `clicked` signal from the MouseArea is sent.
///   function onClicked() {
///     color = "blue";
///   }
/// }
/// ```
class BoundComponent: public QQuickItem {
	Q_OBJECT;
	// clang-format off
	/// The loaded component. Will be null until it has finished loading.
	Q_PROPERTY(QObject* item READ item NOTIFY loaded);
	/// The source to load, as a Component.
	Q_PROPERTY(QQmlComponent* sourceComponent READ sourceComponent WRITE setSourceComponent NOTIFY sourceComponentChanged);
	/// The source to load, as a Url.
	Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged);
	/// If property values should be bound after they are initially set. Defaults to `true`.
	Q_PROPERTY(bool bindValues READ bindValues WRITE setBindValues NOTIFY bindValuesChanged);
	Q_PROPERTY(qreal implicitWidth READ implicitWidth NOTIFY implicitWidthChanged);
	Q_PROPERTY(qreal implicitHeight READ implicitHeight NOTIFY implicitHeightChanged);
	// clang-format on
	QML_ELEMENT;

public:
	explicit BoundComponent(QQuickItem* parent = nullptr): QQuickItem(parent) {}

	void componentComplete() override;

	[[nodiscard]] QObject* item() const;

	[[nodiscard]] QQmlComponent* sourceComponent() const;
	void setSourceComponent(QQmlComponent* sourceComponent);

	[[nodiscard]] QString source() const;
	void setSource(QString source);

	[[nodiscard]] bool bindValues() const;
	void setBindValues(bool bindValues);

signals:
	void loaded();
	void sourceComponentChanged();
	void sourceChanged();
	void bindValuesChanged();

private slots:
	void onComponentDestroyed();
	void onIncubationCompleted();
	void onIncubationFailed();
	void updateSize();
	void updateImplicitSize();

private:
	void disconnectComponent();
	void tryCreate();

	QString mSource;
	bool mBindValues = true;
	QQmlComponent* mComponent = nullptr;
	bool ownsComponent = false;
	QsQmlIncubator* incubator = nullptr;
	QObject* object = nullptr;
	QQuickItem* mItem = nullptr;
	bool componentCompleted = false;
};

class BoundComponentPropertyProxy: public QObject {
	Q_OBJECT;

public:
	BoundComponentPropertyProxy(
	    QObject* from,
	    QObject* to,
	    QMetaProperty fromProperty,
	    QMetaProperty toProperty
	);

public slots:
	void onNotified();

private:
	QObject* from;
	QObject* to;
	QMetaProperty fromProperty;
	QMetaProperty toProperty;
};
