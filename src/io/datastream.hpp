#pragma once

#include <qbytearray.h>
#include <qcontainerfwd.h>
#include <qlocalsocket.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qvariant.h>

class DataStreamParser;

///! Data source that can be streamed into a parser.
/// See also: @@DataStreamParser
class DataStream: public QObject {
	Q_OBJECT;
	/// The parser to stream data from this source into.
	/// If the parser is null no data will be read.
	Q_PROPERTY(DataStreamParser* parser READ reader WRITE setReader NOTIFY readerChanged);
	QML_ELEMENT;
	QML_UNCREATABLE("base class");

public:
	explicit DataStream(QObject* parent = nullptr): QObject(parent) {}

	[[nodiscard]] DataStreamParser* reader() const;
	void setReader(DataStreamParser* reader);

signals:
	void readerChanged();

public slots:
	void onBytesAvailable();

protected:
	[[nodiscard]] virtual QIODevice* ioDevice() const = 0;

private slots:
	void onReaderDestroyed();

protected:
	DataStreamParser* mReader = nullptr;
	QByteArray buffer;
};

///! Parser for streamed input data.
/// See also: @@DataStream, @@SplitParser.
class DataStreamParser: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("base class");

public:
	explicit DataStreamParser(QObject* parent = nullptr): QObject(parent) {}

	// the buffer will be sent in both slots if there is data remaining from a previous parser
	virtual void parseBytes(QByteArray& incoming, QByteArray& buffer) = 0;
	virtual void streamEnded(QByteArray& /*buffer*/) {}

signals:
	/// Emitted when data is read from the stream.
	void read(QString data);
};

///! DataStreamParser for delimited data streams.
/// DataStreamParser for delimited data streams. @@DataStreamParser.read(s) is emitted once per delimited chunk of the stream.
class SplitParser: public DataStreamParser {
	Q_OBJECT;
	/// The delimiter for parsed data. May be multiple characters. Defaults to `\n`.
	///
	/// If the delimiter is empty read lengths may be arbitrary (whatever is returned by the
	/// underlying read call.)
	Q_PROPERTY(QString splitMarker READ splitMarker WRITE setSplitMarker NOTIFY splitMarkerChanged);
	QML_ELEMENT;

public:
	explicit SplitParser(QObject* parent = nullptr): DataStreamParser(parent) {}

	void parseBytes(QByteArray& incoming, QByteArray& buffer) override;
	void streamEnded(QByteArray& buffer) override;

	[[nodiscard]] QString splitMarker() const;
	void setSplitMarker(QString marker);

signals:
	void splitMarkerChanged();

private:
	QString mSplitMarker = "\n";
	bool mSplitMarkerChanged = false;
};

///! DataStreamParser that collects all output into a buffer
/// StdioCollector collects all process output into a buffer exposed as @@text or @@data.
class StdioCollector: public DataStreamParser {
	Q_OBJECT;
	QML_ELEMENT;
	/// The stdio buffer exposed as text. if @@waitForEnd is true, this will not change
	/// until the stream ends.
	Q_PROPERTY(QString text READ text NOTIFY dataChanged);
	/// The stdio buffer exposed as an [ArrayBuffer]. if @@waitForEnd is true, this will not change
	/// until the stream ends.
	///
	/// [ArrayBuffer]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer
	Q_PROPERTY(QByteArray data READ data NOTIFY dataChanged);
	/// If true, @@text and @@data will not be updated until the stream ends. Defaults to true.
	Q_PROPERTY(bool waitForEnd READ waitForEnd WRITE setWaitForEnd NOTIFY waitForEndChanged);

public:
	explicit StdioCollector(QObject* parent = nullptr): DataStreamParser(parent) {}

	void parseBytes(QByteArray& incoming, QByteArray& buffer) override;
	void streamEnded(QByteArray& buffer) override;

	[[nodiscard]] QString text() const { return this->mData; }
	[[nodiscard]] QByteArray data() const { return this->mData; }

	[[nodiscard]] bool waitForEnd() const { return this->mWaitForEnd; }
	void setWaitForEnd(bool waitForEnd);

signals:
	void waitForEndChanged();
	void dataChanged();
	void streamFinished();

private:
	bool mWaitForEnd = true;
	QByteArray mData;
};
