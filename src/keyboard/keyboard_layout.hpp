#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

namespace qs::keyboard {

class KeyboardLayout: public QObject {
	Q_OBJECT;
	// clang-format off
	/// Short xkb layout name.
	Q_PROPERTY(QString shortName READ default NOTIFY shortNameChanged BINDABLE bindableShortName);
	/// xkb layout variant
	Q_PROPERTY(QString variant READ default NOTIFY variantChanged BINDABLE bindableVariant);
	/// Short xkb layout description
	Q_PROPERTY(QString shortDescription READ default NOTIFY shortDescriptionChanged BINDABLE bindableShortDescription);
	/// Full xkb layout description
	Q_PROPERTY(QString fullName READ default NOTIFY fullNameChanged BINDABLE bindableFullName);
	// clang-format on
	QML_NAMED_ELEMENT(KeyboardLayout);
	QML_UNCREATABLE("KeyboardLayouts are accessed through a keyboard device object.");

public:
	explicit KeyboardLayout(QObject* parent): QObject(parent) {}

	void resolveFromShortName(const QString& shortName);
	void resolveFromFullName(const QString& fullName);

	[[nodiscard]] QBindable<QString> bindableShortName() { return &this->bShortName; }
	[[nodiscard]] QBindable<QString> bindableVariant() { return &this->bVariant; }
	[[nodiscard]] QBindable<QString> bindableShortDescription() { return &this->bShortDescription; }
	[[nodiscard]] QBindable<QString> bindableFullName() { return &this->bFullName; }

signals:
	void shortNameChanged();
	void variantChanged();
	void shortDescriptionChanged();
	void fullNameChanged();

private:
	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(KeyboardLayout, QString, bShortName, &KeyboardLayout::shortNameChanged);
	Q_OBJECT_BINDABLE_PROPERTY(KeyboardLayout, QString, bVariant, &KeyboardLayout::variantChanged);
	Q_OBJECT_BINDABLE_PROPERTY(KeyboardLayout, QString, bShortDescription, &KeyboardLayout::shortDescriptionChanged);
	Q_OBJECT_BINDABLE_PROPERTY(KeyboardLayout, QString, bFullName, &KeyboardLayout::fullNameChanged);
	// clang-format on
};

} // namespace qs::keyboard

Q_DECLARE_OPAQUE_POINTER(qs::keyboard::KeyboardLayout*);
