#pragma once

#include <qdebug.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

struct ModelOperation {
	enum Enum : quint8 {
		Insert,
		Remove,
		Move,
	};

	ModelOperation(Enum operation, qint32 index, qint32 length, qint32 destIndex = -1)
	    : operation(operation)
	    , index(index)
	    , length(length)
	    , destIndex(destIndex) {}

	Enum operation;
	qint32 index = 0;
	qint32 length = 0;
	qint32 destIndex = -1;

	[[nodiscard]] bool operator==(const ModelOperation& other) const;
};

QDebug& operator<<(QDebug& debug, const ModelOperation& op);

class TestScriptModel: public QObject {
	Q_OBJECT;

private slots:
	static void unique_data(); // NOLINT
	static void unique();
};
