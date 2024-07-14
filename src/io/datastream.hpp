#pragma once

#include <qbytearray.h>
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
/// See also: @@DataStream$, @@SplitParser
class DataStreamParser: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("base class");

public:
	explicit DataStreamParser(QObject* parent = nullptr): QObject(parent) {}

	// the buffer will be sent in both slots if there is data remaining from a previous parser
	virtual void parseBytes(QByteArray& incoming, QByteArray& buffer) = 0;

signals:
	/// Emitted when data is read from the stream.
	void read(QString data);
};

///! Parser for delimited data streams.
/// Parser for delimited data streams. [read()] is emitted once per delimited chunk of the stream.
///
/// [read()]: ../datastreamparser#sig.read
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

	[[nodiscard]] QString splitMarker() const;
	void setSplitMarker(QString marker);

signals:
	void splitMarkerChanged();

private:
	QString mSplitMarker = "\n";
	bool mSplitMarkerChanged = false;
};
