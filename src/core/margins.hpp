#pragma once

#include <qmargins.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtypes.h>

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
