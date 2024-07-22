#pragma once

#include <qlist.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qtmetamacros.h>

#ifdef QS_TEST
class TestTransformWatcher;
#endif

///! Monitor of all geometry changes between two objects.
/// The TransformWatcher monitors all properties that affect the geometry
/// of two @@QtQuick.Item$s relative to eachother.
///
/// > [!INFO] The algorithm responsible for determining the relationship
/// > between `a` and `b` is biased towards `a` being a parent of `b`,
/// > or `a` being closer to the common parent of `a` and `b` than `b`.
class TransformWatcher: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(QQuickItem* a READ a WRITE setA NOTIFY aChanged);
	Q_PROPERTY(QQuickItem* b READ b WRITE setB NOTIFY bChanged);
	/// Known common parent of both `a` and `b`. Defaults to `null`.
	///
	/// This property can be used to optimize the algorithm that figures out
	/// the relationship between `a` and `b`. Setting it to something that is not
	/// a common parent of both `a` and `b` will prevent the path from being determined
	/// correctly, and setting it to `null` will disable the optimization.
	Q_PROPERTY(QQuickItem* commonParent READ commonParent WRITE setCommonParent NOTIFY commonParentChanged);
	/// This property is updated whenever the geometry of any item in the path from `a` to `b` changes.
	///
	/// Its value is undefined, and is intended to trigger an expression update.
	Q_PROPERTY(QObject* transform READ transform NOTIFY transformChanged);
	// clang-format on
	QML_ELEMENT;

public:
	explicit TransformWatcher(QObject* parent = nullptr): QObject(parent) {}

	[[nodiscard]] QQuickItem* a() const;
	void setA(QQuickItem* a);

	[[nodiscard]] QQuickItem* b() const;
	void setB(QQuickItem* b);

	[[nodiscard]] QQuickItem* commonParent() const;
	void setCommonParent(QQuickItem* commonParent);

	[[nodiscard]] QObject* transform() const { return nullptr; } // NOLINT

signals:
	void transformChanged();

	void aChanged();
	void bChanged();
	void commonParentChanged();

private slots:
	void recalcChains();
	void itemDestroyed();
	void aDestroyed();
	void bDestroyed();

private:
	void resolveChains(QQuickItem* a, QQuickItem* b, QQuickItem* commonParent);
	void resolveChains();
	void linkItem(QQuickItem* item) const;
	void linkChains();
	void unlinkChains();

	QQuickItem* mA = nullptr;
	QQuickItem* mB = nullptr;
	QQuickItem* mCommonParent = nullptr;

	// a -> traverse parent chain -> parent window -> global scope -> child window -> traverse child chain -> b
	QList<QQuickItem*> parentChain;
	QList<QQuickItem*> childChain;
	QQuickWindow* parentWindow = nullptr;
	QQuickWindow* childWindow = nullptr;

#ifdef QS_TEST
	friend class TestTransformWatcher;
#endif
};
