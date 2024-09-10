#pragma once
#include <utility>

#include <qbytearrayview.h>
#include <qcontainerfwd.h>
#include <qfile.h>
#include <qfilesystemwatcher.h>
#include <qlogging.h>
#include <qobject.h>
#include <qthread.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "logging.hpp"
#include "logging_qtprivate.hpp"
#include "ringbuf.hpp"

namespace qs::log {

enum EncodedLogOpcode : quint8 {
	RegisterCategory = 0,
	RecentMessageShort,
	RecentMessageLong,
	BeginCategories,
};

enum CompressedLogType : quint8 {
	Debug = 0,
	Info = 1,
	Warn = 2,
	Critical = 3,
};

CompressedLogType compressedTypeOf(QtMsgType type);
QtMsgType typeOfCompressed(CompressedLogType type);

class WriteBuffer {
public:
	void setDevice(QIODevice* device);
	[[nodiscard]] bool hasDevice() const;
	[[nodiscard]] bool flush();
	void writeBytes(const char* data, qsizetype length);
	void writeU8(quint8 data);
	void writeU16(quint16 data);
	void writeU32(quint32 data);
	void writeU64(quint64 data);

private:
	QIODevice* device = nullptr;
	QByteArray buffer;
};

class DeviceReader {
public:
	void setDevice(QIODevice* device);
	[[nodiscard]] bool hasDevice() const;
	[[nodiscard]] bool readBytes(char* data, qsizetype length);
	// peek UP TO length
	[[nodiscard]] qsizetype peekBytes(char* data, qsizetype length);
	[[nodiscard]] bool skip(qsizetype length);
	[[nodiscard]] bool readU8(quint8* data);
	[[nodiscard]] bool readU16(quint16* data);
	[[nodiscard]] bool readU32(quint32* data);
	[[nodiscard]] bool readU64(quint64* data);

private:
	QIODevice* device = nullptr;
};

class EncodedLogWriter {
public:
	void setDevice(QIODevice* target);
	[[nodiscard]] bool writeHeader();
	[[nodiscard]] bool write(const LogMessage& message);

private:
	void writeOp(EncodedLogOpcode opcode);
	void writeVarInt(quint32 n);
	void writeString(QByteArrayView bytes);
	quint16 getOrCreateCategory(QLatin1StringView category);

	WriteBuffer buffer;

	QHash<QLatin1StringView, quint16> categories;
	quint16 nextCategory = EncodedLogOpcode::BeginCategories;

	QDateTime lastMessageTime = QDateTime::fromSecsSinceEpoch(0);
	HashBuffer<LogMessage> recentMessages {256};
};

class EncodedLogReader {
public:
	void setDevice(QIODevice* source);
	[[nodiscard]] bool readHeader(bool* success, quint8* logVersion, quint8* readerVersion);
	// WARNING: log messages written to the given slot are invalidated when the log reader is destroyed.
	[[nodiscard]] bool read(LogMessage* slot);
	[[nodiscard]] CategoryFilter categoryFilterById(quint16 id);

private:
	[[nodiscard]] bool readVarInt(quint32* slot);
	[[nodiscard]] bool readString(QByteArray* slot);
	[[nodiscard]] bool registerCategory();

	DeviceReader reader;
	QVector<QPair<QByteArray, CategoryFilter>> categories;
	QDateTime lastMessageTime = QDateTime::fromSecsSinceEpoch(0);
	RingBuffer<LogMessage> recentMessages {256};
};

class ThreadLogging: public QObject {
	Q_OBJECT;

public:
	explicit ThreadLogging(QObject* parent): QObject(parent) {}

	void init();
	void initFs();
	void setupFileLogging();

private slots:
	void onMessage(const LogMessage& msg, bool showInSparse);

private:
	QFile* file = nullptr;
	QTextStream fileStream;
	QFile* detailedFile = nullptr;
	EncodedLogWriter detailedWriter;
};

class LogFollower;

class LogReader {
public:
	explicit LogReader(
	    QFile* file,
	    bool timestamps,
	    int tail,
	    QList<qt_logging_registry::QLoggingRule> rules
	)
	    : file(file)
	    , timestamps(timestamps)
	    , remainingTail(tail)
	    , rules(std::move(rules)) {}

	bool initialize();
	bool continueReading();

private:
	QFile* file;
	EncodedLogReader reader;
	bool timestamps;
	int remainingTail;
	QHash<quint16, CategoryFilter> filters;
	QList<qt_logging_registry::QLoggingRule> rules;

	friend class LogFollower;
};

class LogFollower: public QObject {
	Q_OBJECT;

public:
	explicit LogFollower(LogReader* reader, QString path): reader(reader), path(std::move(path)) {}

	bool follow();

private slots:
	void onFileChanged();
	void onFileLocked();

private:
	LogReader* reader;
	QString path;
	QFileSystemWatcher fileWatcher;

	class FcntlWaitThread: public QThread {
	public:
		explicit FcntlWaitThread(LogFollower* follower): follower(follower) {}

	protected:
		void run() override;

	private:
		LogFollower* follower;
	};

	FcntlWaitThread waitThread {this};
};

} // namespace qs::log
