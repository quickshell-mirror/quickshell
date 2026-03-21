#pragma once

#include <QObject>
#include <QStringList>
#include <QtTypes>
#include <qqmlintegration.h>
#include <qtmetamacros.h>

#include "manager.hpp"
#include "output.hpp"

namespace qs::dwl {

class DwlIpcQml: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;
	QML_NAMED_ELEMENT(DwlIpc);

	Q_PROPERTY(quint32 tagCount READ tagCount NOTIFY tagCountChanged);
	Q_PROPERTY(QStringList layouts READ layouts NOTIFY layoutsChanged);
	Q_PROPERTY(QList<qs::dwl::DwlIpcOutput*> outputs READ outputs NOTIFY outputsChanged);
	Q_PROPERTY(bool available READ available NOTIFY availableChanged);

public:
	explicit DwlIpcQml(QObject* parent = nullptr);

	[[nodiscard]] quint32 tagCount() const;
	[[nodiscard]] QStringList layouts() const;
	[[nodiscard]] QList<DwlIpcOutput*> outputs() const;
	[[nodiscard]] bool available() const;

	[[nodiscard]] Q_INVOKABLE qs::dwl::DwlIpcOutput* outputForName(const QString& name) const;

private:
	DwlIpcManager* manager = nullptr;

signals:
	void tagCountChanged();
	void layoutsChanged();
	void outputsChanged();
	void availableChanged();
};

} // namespace qs::dwl
