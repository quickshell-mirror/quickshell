#pragma once

#include <qbytearray.h>
#include <qiodevice.h>
#include <qtypes.h>

class StreamReader {
public:
	void setDevice(QIODevice* device);

	void startTransaction();
	QByteArray readBytes(qsizetype count);
	QByteArray readUntil(char terminator);
	void readInto(char* ptr, qsizetype count);
	qint32 readI32();
	bool commitTransaction();
	void reset();

private:
	bool fill();

	QIODevice* device = nullptr;
	QByteArray buffer;
	qsizetype cursor = 0;
	bool failed = false;
};
