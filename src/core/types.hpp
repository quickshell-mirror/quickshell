#pragma once

#include <qdebug.h>
#include <qmargins.h>
#include <qnamespace.h>
#include <qpoint.h>
#include <qqmlintegration.h>
#include <qrect.h>
#include <qtmetamacros.h>
#include <qtypes.h>

class Box {
	Q_GADGET;
	Q_PROPERTY(qint32 x MEMBER x);
	Q_PROPERTY(qint32 y MEMBER y);
	Q_PROPERTY(qint32 w MEMBER w);
	Q_PROPERTY(qint32 h MEMBER h);
	Q_PROPERTY(qint32 width MEMBER w);
	Q_PROPERTY(qint32 height MEMBER h);
	QML_CONSTRUCTIBLE_VALUE;
	QML_VALUE_TYPE(box);

public:
	explicit Box() = default;
	Box(qint32 x, qint32 y, qint32 w, qint32 h): x(x), y(y), w(w), h(h) {}

	Q_INVOKABLE Box(const QRect& rect): x(rect.x()), y(rect.y()), w(rect.width()), h(rect.height()) {}
	Q_INVOKABLE Box(const QPoint& rect): x(rect.x()), y(rect.y()) {}

	Q_INVOKABLE Box(const QRectF& rect)
	    : x(static_cast<qint32>(rect.x()))
	    , y(static_cast<qint32>(rect.y()))
	    , w(static_cast<qint32>(rect.width()))
	    , h(static_cast<qint32>(rect.height())) {}

	Q_INVOKABLE Box(const QPointF& rect)
	    : x(static_cast<qint32>(rect.x()))
	    , y(static_cast<qint32>(rect.y())) {}

	bool operator==(const Box& other) const;

	qint32 x = 0;
	qint32 y = 0;
	qint32 w = 0;
	qint32 h = 0;

	[[nodiscard]] QRect qrect() const;
	[[nodiscard]] bool isEmpty() const { return this->w == 0 && this->h == 0; }
};

QDebug operator<<(QDebug debug, const Box& box);

class Margins {
	Q_GADGET;
	Q_PROPERTY(qint32 left MEMBER left);
	Q_PROPERTY(qint32 right MEMBER right);
	Q_PROPERTY(qint32 top MEMBER top);
	Q_PROPERTY(qint32 bottom MEMBER bottom);
	QML_CONSTRUCTIBLE_VALUE;
	QML_VALUE_TYPE(margins);

public:
	[[nodiscard]] bool operator==(const Margins& other) const noexcept {
		// clang-format off
		return this->left == other.left
			&& this->right == other.right
			&& this->top == other.top
			&& this->bottom == other.bottom;
		// clang-format on
	}

	qint32 left = 0;
	qint32 right = 0;
	qint32 top = 0;
	qint32 bottom = 0;

	[[nodiscard]] QMargins qmargins() const;
};

///! Top Left Right Bottom flags.
/// Edge flags can be combined with the `|` operator.
namespace Edges { // NOLINT
Q_NAMESPACE;
QML_NAMED_ELEMENT(Edges);

enum Enum : quint8 {
	None = 0,
	Top = Qt::TopEdge,
	Left = Qt::LeftEdge,
	Right = Qt::RightEdge,
	Bottom = Qt::BottomEdge,
};
Q_ENUM_NS(Enum);
Q_DECLARE_FLAGS(Flags, Enum);

Qt::Edges toQt(Flags edges);
bool isOpposing(Flags edges);

}; // namespace Edges

Q_DECLARE_OPERATORS_FOR_FLAGS(Edges::Flags);
