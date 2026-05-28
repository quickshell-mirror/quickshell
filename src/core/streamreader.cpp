#include "streamreader.hpp"
#include <cstring>

#include <qbytearray.h>
#include <qiodevice.h>
#include <qtypes.h>

void StreamReader::setDevice(QIODevice* device) {
	this->reset();
	this->device = device;
}

void StreamReader::startTransaction() {
	this->cursor = 0;
	this->failed = false;
}

bool StreamReader::fill() {
	auto available = this->device->bytesAvailable();
	if (available <= 0) return false;
	auto oldSize = this->buffer.size();
	this->buffer.resize(oldSize + available);
	auto bytesRead = this->device->read(this->buffer.data() + oldSize, available); // NOLINT

	if (bytesRead <= 0) {
		this->buffer.resize(oldSize);
		return false;
	}

	this->buffer.resize(oldSize + bytesRead);
	return true;
}

QByteArray StreamReader::readBytes(qsizetype count) {
	if (this->failed) return {};

	auto needed = this->cursor + count;

	while (this->buffer.size() < needed) {
		if (!this->fill()) {
			this->failed = true;
			return {};
		}
	}

	auto result = this->buffer.mid(this->cursor, count);
	this->cursor += count;
	return result;
}

QByteArray StreamReader::readUntil(char terminator) {
	if (this->failed) return {};

	auto searchFrom = this->cursor;
	auto idx = this->buffer.indexOf(terminator, searchFrom);

	while (idx == -1) {
		searchFrom = this->buffer.size();
		if (!this->fill()) {
			this->failed = true;
			return {};
		}

		idx = this->buffer.indexOf(terminator, searchFrom);
	}

	auto length = idx - this->cursor + 1;
	auto result = this->buffer.mid(this->cursor, length);
	this->cursor += length;
	return result;
}

void StreamReader::readInto(char* ptr, qsizetype count) {
	auto data = this->readBytes(count);
	if (!data.isEmpty()) memcpy(ptr, data.data(), count);
}

qint32 StreamReader::readI32() {
	qint32 value = 0;
	this->readInto(reinterpret_cast<char*>(&value), sizeof(qint32));
	return value;
}

bool StreamReader::commitTransaction() {
	if (this->failed) {
		this->cursor = 0;
		return false;
	}

	this->buffer.remove(0, this->cursor);
	this->cursor = 0;
	return true;
}

void StreamReader::reset() {
	this->buffer.clear();
	this->cursor = 0;
}
