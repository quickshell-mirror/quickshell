#pragma once

#include <qcontainerfwd.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../../../core/doc.hpp"
#include "../../../core/model.hpp"
#include "../../../keyboard/keyboard_layout.hpp"
#include "connection.hpp"

namespace qs::hyprland::ipc {

///! Keyboard input device
/// Keyboard input device. Represents a physical or virtual keyboard device as reported by Hyprland.
///
/// Provides the list of configured layouts and the currently active one.
/// Accessed through @@Hyprland.keyboards or @@Hyprland.activeKeyboard.
class HyprlandKeyboard: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Name of the keyboard device as reported by Hyprland.
	Q_PROPERTY(QString name READ default NOTIFY nameChanged BINDABLE bindableName);
	/// The currently active layout on this keyboard.
	Q_PROPERTY(qs::keyboard::KeyboardLayout* activeLayout READ default NOTIFY activeLayoutChanged BINDABLE bindableActiveLayout);
	/// Index of the active layout in @@availableLayouts.
	Q_PROPERTY(qint32 activeLayoutIndex READ default NOTIFY activeLayoutIndexChanged BINDABLE bindableActiveLayoutIndex);
	QSDOC_TYPE_OVERRIDE(ObjectModel<qs::keyboard::KeyboardLayout>*);
	/// All available layouts configured for this keyboard.
	Q_PROPERTY(UntypedObjectModel* availableLayouts READ availableLayouts CONSTANT);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("HyprlandKeyboards must be retrieved from the HyprlandIpc object.");

public:
	explicit HyprlandKeyboard(HyprlandIpc* ipc): QObject(ipc), ipc(ipc), mAvailableLayouts(this) {}

	void updateFromDeviceObject(const QVariantMap& object);
	void updateActiveLayout(const QString& activeLayoutName);

	[[nodiscard]] QBindable<QString> bindableName() { return &this->bName; }
	[[nodiscard]] QBindable<qs::keyboard::KeyboardLayout*> bindableActiveLayout() const {
		return &this->bActiveLayout;
	}
	[[nodiscard]] QBindable<qint32> bindableActiveLayoutIndex() { return &this->bActiveLayoutIndex; }

	[[nodiscard]] ObjectModel<qs::keyboard::KeyboardLayout>* availableLayouts();

	/// Switch to the next layout in @@availableLayouts, wrapping around.
	Q_INVOKABLE void cycleLayout();
	/// Switch to a specific layout by its index in @@availableLayouts.
	Q_INVOKABLE void switchToLayout(qint32 index);

signals:
	void nameChanged();
	void activeLayoutChanged();
	void activeLayoutIndexChanged();

private slots:
	void onActiveLayoutDestroyed();

private:
	HyprlandIpc* ipc;
	ObjectModel<qs::keyboard::KeyboardLayout> mAvailableLayouts;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandKeyboard, QString, bName, &HyprlandKeyboard::nameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandKeyboard, qs::keyboard::KeyboardLayout*, bActiveLayout, &HyprlandKeyboard::activeLayoutChanged);
	Q_OBJECT_BINDABLE_PROPERTY(HyprlandKeyboard, qint32, bActiveLayoutIndex, &HyprlandKeyboard::activeLayoutIndexChanged);
	// clang-format on
};

} // namespace qs::hyprland::ipc
