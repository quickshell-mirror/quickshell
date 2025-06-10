#include "datastream.hpp"
#include <algorithm>
#include <utility>

#include <qlocalsocket.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>

DataStreamParser* DataStream::reader() const { return this->mReader; }

void DataStream::setReader(DataStreamParser* reader) {
	if (reader == this->mReader) return;

	if (this->mReader != nullptr) {
		QObject::disconnect(this->mReader, nullptr, this, nullptr);
	}

	this->mReader = reader;

	if (reader != nullptr) {
		QObject::connect(reader, &QObject::destroyed, this, &DataStream::onReaderDestroyed);
	}

	emit this->readerChanged();

	if (reader != nullptr && !this->buffer.isEmpty()) {
		reader->parseBytes(this->buffer, this->buffer);
	}
}

void DataStream::onReaderDestroyed() {
	this->mReader = nullptr;
	emit this->readerChanged();
}

void DataStream::onBytesAvailable() {
	if (this->mReader == nullptr) return;
	auto buf = this->ioDevice()->readAll();
	this->mReader->parseBytes(buf, this->buffer);
}

void SplitParser::parseBytes(QByteArray& incoming, QByteArray& buffer) {
	if (this->mSplitMarker.isEmpty()) {
		if (!buffer.isEmpty()) {
			emit this->read(QString(buffer));
			buffer.clear();
		}

		emit this->read(QString(incoming));
		return;
	}

	// make sure we dont miss any delimiters in the buffer if the delimiter changes
	if (this->mSplitMarkerChanged) {
		this->mSplitMarkerChanged = false;
		this->parseBytes(buffer, buffer);
	}

	auto marker = this->mSplitMarker.toUtf8();
	auto mlen = marker.length();

	auto blen = buffer.size();
	auto ilen = incoming.size();

	qsizetype start = &incoming == &buffer ? 0 : -blen;
	for (auto readi = -std::min(blen, mlen - 1); readi <= ilen - mlen; readi++) {
		for (auto marki = 0; marki < mlen; marki++) {
			qint8 byte; // NOLINT
			if (readi + marki < 0) byte = buffer[blen + readi + marki];
			else byte = incoming[readi + marki];

			if (byte != marker[marki]) goto fail;
		}

		{
			QByteArray slice;
			if (start < 0) slice = buffer.sliced(0, std::min(blen, blen + readi));
			if (readi > 0) {
				auto sstart = std::max(static_cast<qsizetype>(0), start);
				slice.append(incoming.sliced(sstart, readi - sstart));
			}
			readi += mlen;
			start = readi;
			emit this->read(QString(slice));
		}

	fail:;
	}

	if (start < 0) {
		buffer.append(incoming);
	} else {
		// Will break the init case if inlined. Must be before clear.
		auto slice = incoming.sliced(start);
		buffer.clear();
		buffer.insert(0, slice);
	}
}

void SplitParser::streamEnded(QByteArray& buffer) {
	if (!buffer.isEmpty()) emit this->read(QString(buffer));
}

QString SplitParser::splitMarker() const { return this->mSplitMarker; }

void SplitParser::setSplitMarker(QString marker) {
	if (marker == this->mSplitMarker) return;

	this->mSplitMarker = std::move(marker);
	this->mSplitMarkerChanged = true;
	emit this->splitMarkerChanged();
}

void StdioCollector::parseBytes(QByteArray& incoming, QByteArray& buffer) {
	buffer.append(incoming);

	if (!this->mWaitForEnd) {
		this->mData = buffer;
		emit this->dataChanged();
	}
}

void StdioCollector::streamEnded(QByteArray& buffer) {
	if (this->mWaitForEnd) {
		this->mData = buffer;
		emit this->dataChanged();
	}

	emit this->streamFinished();
}

void StdioCollector::setWaitForEnd(bool waitForEnd) {
	if (waitForEnd == this->mWaitForEnd) return;
	this->mWaitForEnd = waitForEnd;
	emit this->waitForEndChanged();
}
