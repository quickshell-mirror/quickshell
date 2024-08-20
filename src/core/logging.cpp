#include "logging.hpp"
#include <array>
#include <cstdio>

#include <qbytearrayview.h>
#include <qdatetime.h>
#include <qendian.h>
#include <qhash.h>
#include <qhashfunctions.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qstring.h>
#include <qstringview.h>
#include <qsysinfo.h>
#include <qtenvironmentvariables.h>
#include <qtextstream.h>
#include <qthread.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <sys/mman.h>
#include <sys/sendfile.h>

#include "crashinfo.hpp"
#include "logging_p.hpp"
#include "logging_qtprivate.cpp" // NOLINT
#include "paths.hpp"

namespace qs::log {

Q_LOGGING_CATEGORY(logLogging, "quickshell.logging", QtWarningMsg);

bool LogMessage::operator==(const LogMessage& other) const {
	// note: not including time
	return this->type == other.type && this->category == other.category && this->body == other.body;
}

size_t qHash(const LogMessage& message) {
	return qHash(message.type) ^ qHash(message.category) ^ qHash(message.body);
}

void LogMessage::formatMessage(
    QTextStream& stream,
    const LogMessage& msg,
    bool color,
    bool timestamp
) {
	if (timestamp) {
		if (color) stream << "\033[90m";
		stream << msg.time.toString("yyyy-MM-dd hh:mm:ss.zzz");
	}

	if (color) {
		switch (msg.type) {
		case QtDebugMsg: stream << "\033[34m DEBUG"; break;
		case QtInfoMsg: stream << "\033[32m  INFO"; break;
		case QtWarningMsg: stream << "\033[33m  WARN"; break;
		case QtCriticalMsg: stream << "\033[31m ERROR"; break;
		case QtFatalMsg: stream << "\033[31m FATAL"; break;
		}
	} else {
		switch (msg.type) {
		case QtDebugMsg: stream << " DEBUG"; break;
		case QtInfoMsg: stream << "  INFO"; break;
		case QtWarningMsg: stream << "  WARN"; break;
		case QtCriticalMsg: stream << " ERROR"; break;
		case QtFatalMsg: stream << " FATAL"; break;
		}
	}

	const auto isDefault = msg.category == "default";

	if (color && !isDefault && msg.type != QtFatalMsg) stream << "\033[97m";

	if (!isDefault) {
		stream << ' ' << msg.category;
	}

	if (color && msg.type != QtFatalMsg) stream << "\033[0m";

	stream << ": " << msg.body;

	if (color && msg.type == QtFatalMsg) stream << "\033[0m";
}

bool CategoryFilter::shouldDisplay(QtMsgType type) const {
	switch (type) {
	case QtDebugMsg: return this->debug;
	case QtInfoMsg: return this->info;
	case QtWarningMsg: return this->warn;
	case QtCriticalMsg: return this->critical;
	default: return true;
	}
}

LogManager::LogManager(): stdoutStream(stdout) {}

void LogManager::messageHandler(
    QtMsgType type,
    const QMessageLogContext& context,
    const QString& msg
) {
	auto message = LogMessage(type, QLatin1StringView(context.category), msg.toUtf8());

	auto* self = LogManager::instance();

	auto display = true;

	const auto* key = static_cast<const void*>(context.category);

	if (self->sparseFilters.contains(key)) {
		display = self->sparseFilters.value(key).shouldDisplay(type);
	}

	if (display) {
		LogMessage::formatMessage(self->stdoutStream, message, self->colorLogs, false);
		self->stdoutStream << Qt::endl;
	}

	emit self->logMessage(message, display);
}

void LogManager::filterCategory(QLoggingCategory* category) {
	auto* instance = LogManager::instance();

	if (instance->lastCategoryFilter) {
		instance->lastCategoryFilter(category);
	}

	if (QLatin1StringView(category->categoryName()).startsWith(QLatin1StringView("quickshell"))) {
		// We assume the category name pointer will always be the same and be comparable in the message handler.
		LogManager::instance()->sparseFilters.insert(
		    static_cast<const void*>(category->categoryName()),
		    CategoryFilter(category)
		);

		category->setEnabled(QtDebugMsg, true);
		category->setEnabled(QtInfoMsg, true);
		category->setEnabled(QtWarningMsg, true);
		category->setEnabled(QtCriticalMsg, true);
	}
}

LogManager* LogManager::instance() {
	static auto* instance = new LogManager(); // NOLINT
	return instance;
}

void LogManager::init(bool color, bool sparseOnly) {
	auto* instance = LogManager::instance();
	instance->colorLogs = color;

	qInstallMessageHandler(&LogManager::messageHandler);

	if (!sparseOnly) {
		instance->lastCategoryFilter = QLoggingCategory::installFilter(&LogManager::filterCategory);
	}

	qCDebug(logLogging) << "Creating offthread logger...";
	auto* thread = new QThread();
	instance->threadProxy.moveToThread(thread);
	thread->start();
	QMetaObject::invokeMethod(&instance->threadProxy, "initInThread", Qt::BlockingQueuedConnection);
	qCDebug(logLogging) << "Logger initialized.";
}

void LogManager::initFs() {
	QMetaObject::invokeMethod(
	    &LogManager::instance()->threadProxy,
	    "initFs",
	    Qt::BlockingQueuedConnection
	);
}

void LoggingThreadProxy::initInThread() {
	this->logging = new ThreadLogging(this);
	this->logging->init();
}

void LoggingThreadProxy::initFs() { this->logging->initFs(); }

void ThreadLogging::init() {
	auto logMfd = memfd_create("quickshell:logs", 0);

	if (logMfd == -1) {
		qCCritical(logLogging) << "Failed to create memfd for initial log storage"
		                       << qt_error_string(-1);
	}

	auto dlogMfd = memfd_create("quickshell:detailedlogs", 0);

	if (dlogMfd == -1) {
		qCCritical(logLogging) << "Failed to create memfd for initial detailed log storage"
		                       << qt_error_string(-1);
	}

	if (logMfd != -1) {
		this->file = new QFile();
		this->file->open(logMfd, QFile::ReadWrite, QFile::AutoCloseHandle);
		this->fileStream.setDevice(this->file);
	}

	if (dlogMfd != -1) {
		crash::CrashInfo::INSTANCE.logFd = dlogMfd;

		this->detailedFile = new QFile();
		// buffered by WriteBuffer
		this->detailedFile->open(dlogMfd, QFile::ReadWrite | QFile::Unbuffered, QFile::AutoCloseHandle);
		this->detailedWriter.setDevice(this->detailedFile);

		if (!this->detailedWriter.writeHeader()) {
			qCCritical(logLogging) << "Could not write header for detailed logs.";
			this->detailedWriter.setDevice(nullptr);
			delete this->detailedFile;
			this->detailedFile = nullptr;
		}
	}

	// This connection is direct so it works while the event loop is destroyed between
	// QCoreApplication delete and Q(Gui)Application launch.
	QObject::connect(
	    LogManager::instance(),
	    &LogManager::logMessage,
	    this,
	    &ThreadLogging::onMessage,
	    Qt::DirectConnection
	);

	qCDebug(logLogging) << "Created memfd" << logMfd << "for early logs.";
	qCDebug(logLogging) << "Created memfd" << dlogMfd << "for early detailed logs.";
}

void ThreadLogging::initFs() {
	qCDebug(logLogging) << "Starting filesystem logging...";
	auto* runDir = QsPaths::instance()->instanceRunDir();

	if (!runDir) {
		qCCritical(logLogging
		) << "Could not start filesystem logging as the runtime directory could not be created.";
		return;
	}

	auto path = runDir->filePath("log.log");
	auto detailedPath = runDir->filePath("log.qslog");
	auto* file = new QFile(path);
	auto* detailedFile = new QFile(detailedPath);

	if (!file->open(QFile::ReadWrite | QFile::Truncate)) {
		qCCritical(logLogging
		) << "Could not start filesystem logger as the log file could not be created:"
		  << path;
		delete file;
		file = nullptr;
	} else {
		qInfo() << "Saving logs to" << path;
	}

	// buffered by WriteBuffer
	if (!detailedFile->open(QFile::ReadWrite | QFile::Truncate | QFile::Unbuffered)) {
		qCCritical(logLogging
		) << "Could not start detailed filesystem logger as the log file could not be created:"
		  << detailedPath;
		delete detailedFile;
		detailedFile = nullptr;
	} else {
		qCInfo(logLogging) << "Saving detailed logs to" << path;
	}

	qCDebug(logLogging) << "Copying memfd logs to log file...";

	if (file) {
		auto* oldFile = this->file;
		if (oldFile) {
			oldFile->seek(0);
			sendfile(file->handle(), oldFile->handle(), nullptr, oldFile->size());
		}

		this->file = file;
		this->fileStream.setDevice(file);
		delete oldFile;
	}

	if (detailedFile) {
		auto* oldFile = this->detailedFile;
		if (oldFile) {
			oldFile->seek(0);
			sendfile(detailedFile->handle(), oldFile->handle(), nullptr, oldFile->size());
		}

		crash::CrashInfo::INSTANCE.logFd = detailedFile->handle();

		this->detailedFile = detailedFile;
		this->detailedWriter.setDevice(detailedFile);

		if (!oldFile) {
			if (!this->detailedWriter.writeHeader()) {
				qCCritical(logLogging) << "Could not write header for detailed logs.";
				this->detailedWriter.setDevice(nullptr);
				delete this->detailedFile;
				this->detailedFile = nullptr;
			}
		}

		delete oldFile;
	}

	qCDebug(logLogging) << "Switched logging to disk logs.";

	auto* logManager = LogManager::instance();
	QObject::disconnect(logManager, &LogManager::logMessage, this, &ThreadLogging::onMessage);

	QObject::connect(
	    logManager,
	    &LogManager::logMessage,
	    this,
	    &ThreadLogging::onMessage,
	    Qt::QueuedConnection
	);

	qCDebug(logLogging) << "Switched threaded logger to queued eventloop connection.";
}

void ThreadLogging::onMessage(const LogMessage& msg, bool showInSparse) {
	if (showInSparse) {
		if (this->fileStream.device() == nullptr) return;
		LogMessage::formatMessage(this->fileStream, msg, false, true);
		this->fileStream << Qt::endl;
	}

	if (this->detailedWriter.write(msg)) {
		this->detailedFile->flush();
	} else if (this->detailedFile != nullptr) {
		qCCritical(logLogging) << "Detailed logger failed to write. Ending detailed logs.";
	}
}

CompressedLogType compressedTypeOf(QtMsgType type) {
	switch (type) {
	case QtDebugMsg: return CompressedLogType::Debug;
	case QtInfoMsg: return CompressedLogType::Info;
	case QtWarningMsg: return CompressedLogType::Warn;
	case QtCriticalMsg:
	case QtFatalMsg: return CompressedLogType::Critical;
	}
}

QtMsgType typeOfCompressed(CompressedLogType type) {
	switch (type) {
	case CompressedLogType::Debug: return QtDebugMsg;
	case CompressedLogType::Info: return QtInfoMsg;
	case CompressedLogType::Warn: return QtWarningMsg;
	case CompressedLogType::Critical: return QtCriticalMsg;
	}
}

void WriteBuffer::setDevice(QIODevice* device) { this->device = device; }
bool WriteBuffer::hasDevice() const { return this->device; }

bool WriteBuffer::flush() {
	auto written = this->device->write(this->buffer);
	auto success = written == this->buffer.length();
	this->buffer.clear();
	return success;
}

void WriteBuffer::writeBytes(const char* data, qsizetype length) {
	this->buffer.append(data, length);
}

void WriteBuffer::writeU8(quint8 data) {
	this->writeBytes(reinterpret_cast<char*>(&data), 1); // NOLINT
}

void WriteBuffer::writeU16(quint16 data) {
	data = qToLittleEndian(data);
	this->writeBytes(reinterpret_cast<char*>(&data), 2); // NOLINT
}

void WriteBuffer::writeU32(quint32 data) {
	data = qToLittleEndian(data);
	this->writeBytes(reinterpret_cast<char*>(&data), 4); // NOLINT
}

void WriteBuffer::writeU64(quint64 data) {
	data = qToLittleEndian(data);
	this->writeBytes(reinterpret_cast<char*>(&data), 8); // NOLINT
}

void DeviceReader::setDevice(QIODevice* device) { this->device = device; }
bool DeviceReader::hasDevice() const { return this->device; }

bool DeviceReader::readBytes(char* data, qsizetype length) {
	return this->device->read(data, length) == length;
}

qsizetype DeviceReader::peekBytes(char* data, qsizetype length) {
	return this->device->peek(data, length);
}

bool DeviceReader::skip(qsizetype length) { return this->device->skip(length) == length; }

bool DeviceReader::readU8(quint8* data) {
	return this->readBytes(reinterpret_cast<char*>(data), 1); // NOLINT
}

bool DeviceReader::readU16(quint16* data) {
	return this->readBytes(reinterpret_cast<char*>(data), 2); // NOLINT
}

bool DeviceReader::readU32(quint32* data) {
	return this->readBytes(reinterpret_cast<char*>(data), 4); // NOLINT
}

bool DeviceReader::readU64(quint64* data) {
	return this->readBytes(reinterpret_cast<char*>(data), 8); // NOLINT
}

void EncodedLogWriter::setDevice(QIODevice* target) { this->buffer.setDevice(target); }
void EncodedLogReader::setDevice(QIODevice* source) { this->reader.setDevice(source); }

constexpr quint8 LOG_VERSION = 1;

bool EncodedLogWriter::writeHeader() {
	this->buffer.writeU8(LOG_VERSION);
	return this->buffer.flush();
}

bool EncodedLogReader::readHeader(bool* success, quint8* version, quint8* readerVersion) {
	if (!this->reader.readU8(version)) return false;
	*success = *version == LOG_VERSION;
	*readerVersion = LOG_VERSION;
	return true;
}

bool EncodedLogWriter::write(const LogMessage& message) {
	if (!this->buffer.hasDevice()) return false;

	LogMessage* prevMessage = nullptr;
	auto index = this->recentMessages.indexOf(message, &prevMessage);

	// If its a dupe, save memory by reusing the buffer of the first message and letting
	// the new one be deallocated.
	auto body = prevMessage ? prevMessage->body : message.body;
	this->recentMessages.emplace(message.type, message.category, body, message.time);

	if (index != -1) {
		auto secondDelta = this->lastMessageTime.secsTo(message.time);

		if (secondDelta < 16 && index < 16) {
			this->writeOp(EncodedLogOpcode::RecentMessageShort);
			this->buffer.writeU8(index | (secondDelta << 4));
		} else {
			this->writeOp(EncodedLogOpcode::RecentMessageLong);
			this->buffer.writeU8(index);
			this->writeVarInt(secondDelta);
		}

		goto finish;
	} else {
		auto categoryId = this->getOrCreateCategory(message.category);
		this->writeVarInt(categoryId);

		auto writeFullTimestamp = [this, &message]() {
			this->buffer.writeU64(message.time.toSecsSinceEpoch());
		};

		if (message.type == QtFatalMsg) {
			this->buffer.writeU8(0xff);
			writeFullTimestamp();
		} else {
			quint8 field = compressedTypeOf(message.type);

			auto secondDelta = this->lastMessageTime.secsTo(message.time);
			if (secondDelta >= 0x1d) {
				// 0x1d = followed by delta int
				// 0x1e = followed by epoch delta int
				field |= (secondDelta < 0xffff ? 0x1d : 0x1e) << 3;
			} else {
				field |= secondDelta << 3;
			}

			this->buffer.writeU8(field);

			if (secondDelta > 29) {
				if (secondDelta > 0xffff) {
					writeFullTimestamp();
				} else {
					this->writeVarInt(secondDelta);
				}
			}
		}

		this->writeString(message.body);
	}

finish:
	// copy with second precision
	this->lastMessageTime = QDateTime::fromSecsSinceEpoch(message.time.toSecsSinceEpoch());
	return this->buffer.flush();
}

bool EncodedLogReader::read(LogMessage* slot) {
start:
	quint32 next = 0;
	if (!this->readVarInt(&next)) return false;

	if (next < EncodedLogOpcode::BeginCategories) {
		if (next == EncodedLogOpcode::RegisterCategory) {
			if (!this->registerCategory()) return false;
			goto start;
		} else if (next == EncodedLogOpcode::RecentMessageShort || next == EncodedLogOpcode::RecentMessageLong)
		{
			quint8 index = 0;
			quint32 secondDelta = 0;

			if (next == EncodedLogOpcode::RecentMessageShort) {
				quint8 field = 0;
				if (!this->reader.readU8(&field)) return false;
				index = field & 0xf;
				secondDelta = field >> 4;
			} else {
				if (!this->reader.readU8(&index)) return false;
				if (!this->readVarInt(&secondDelta)) return false;
			}

			if (index < 0 || index >= this->recentMessages.size()) return false;
			*slot = this->recentMessages.at(index);
			this->lastMessageTime = this->lastMessageTime.addSecs(static_cast<qint64>(secondDelta));
			slot->time = this->lastMessageTime;
		}
	} else {
		auto categoryId = next - EncodedLogOpcode::BeginCategories;
		auto category = this->categories.value(categoryId);

		quint8 field = 0;
		if (!this->reader.readU8(&field)) return false;

		auto msgType = QtDebugMsg;
		quint64 secondDelta = 0;
		auto needsTimeRead = false;

		if (field == 0xff) {
			msgType = QtFatalMsg;
			needsTimeRead = true;
		} else {
			msgType = typeOfCompressed(static_cast<CompressedLogType>(field & 0x07));
			secondDelta = field >> 3;

			if (secondDelta == 0x1d) {
				quint32 slot = 0;
				if (!this->readVarInt(&slot)) return false;
				secondDelta = slot;
			} else if (secondDelta == 0x1e) {
				needsTimeRead = true;
			}
		}

		if (needsTimeRead) {
			if (!this->reader.readU64(&secondDelta)) return false;
		}

		this->lastMessageTime = this->lastMessageTime.addSecs(static_cast<qint64>(secondDelta));

		QByteArray body;
		if (!this->readString(&body)) return false;

		*slot = LogMessage(msgType, QLatin1StringView(category), body, this->lastMessageTime);
		slot->readCategoryId = categoryId;
	}

	this->recentMessages.emplace(*slot);
	return true;
}

void EncodedLogWriter::writeOp(EncodedLogOpcode opcode) { this->buffer.writeU8(opcode); }

void EncodedLogWriter::writeVarInt(quint32 n) {
	if (n < 0xff) {
		this->buffer.writeU8(n);
	} else if (n < 0xffff) {
		this->buffer.writeU8(0xff);
		this->buffer.writeU16(n);
	} else {
		this->buffer.writeU8(0xff);
		this->buffer.writeU16(0xffff);
		this->buffer.writeU32(n);
	}
}

bool EncodedLogReader::readVarInt(quint32* slot) {
	auto bytes = std::array<quint8, 7>();
	auto readLength = this->reader.peekBytes(reinterpret_cast<char*>(bytes.data()), 7); // NOLINT

	if (bytes[0] != 0xff && readLength >= 1) {
		auto n = *reinterpret_cast<quint8*>(bytes.data()); // NOLINT
		if (!this->reader.skip(1)) return false;
		*slot = qFromLittleEndian(n);
	} else if ((bytes[1] != 0xff || bytes[2] != 0xff) && readLength >= 3) {
		auto n = *reinterpret_cast<quint16*>(bytes.data() + 1); // NOLINT
		if (!this->reader.skip(3)) return false;
		*slot = qFromLittleEndian(n);
	} else if (readLength == 7) {
		auto n = *reinterpret_cast<quint32*>(bytes.data() + 3); // NOLINT
		if (!this->reader.skip(7)) return false;
		*slot = qFromLittleEndian(n);
	} else return false;

	return true;
}

void EncodedLogWriter::writeString(QByteArrayView bytes) {
	this->writeVarInt(bytes.length());
	this->buffer.writeBytes(bytes.constData(), bytes.length());
}

bool EncodedLogReader::readString(QByteArray* slot) {
	quint32 length = 0;
	if (!this->readVarInt(&length)) return false;

	*slot = QByteArray(length, Qt::Uninitialized);
	auto r = this->reader.readBytes(slot->data(), slot->size());
	return r;
}

quint16 EncodedLogWriter::getOrCreateCategory(QLatin1StringView category) {
	if (this->categories.contains(category)) {
		return this->categories.value(category);
	} else {
		this->writeOp(EncodedLogOpcode::RegisterCategory);
		// id is implicitly the next available id
		this->writeString(category);

		auto id = this->nextCategory++;
		this->categories.insert(category, id);

		return id;
	}
}

bool EncodedLogReader::registerCategory() {
	QByteArray name;
	if (!this->readString(&name)) return false;
	this->categories.append(name);
	return true;
}

bool readEncodedLogs(QIODevice* device, bool timestamps, const QString& rulespec) {
	using namespace qt_logging_registry;

	QList<QLoggingRule> rules;

	{
		QLoggingSettingsParser parser;
		parser.setContent(rulespec);
		rules = parser.rules();
	}

	auto reader = EncodedLogReader();
	reader.setDevice(device);

	bool readable = false;
	quint8 logVersion = 0;
	quint8 readerVersion = 0;
	if (!reader.readHeader(&readable, &logVersion, &readerVersion)) {
		qCritical() << "Failed to read log header.";
		return false;
	}

	if (!readable) {
		qCritical() << "This log was encoded with version" << logVersion
		            << "of the quickshell log encoder, which cannot be decoded by the current "
		               "version of quickshell, with log version"
		            << readerVersion;
		return false;
	}

	auto color = LogManager::instance()->colorLogs;

	auto filters = QHash<quint16, CategoryFilter>();

	LogMessage message;
	auto stream = QTextStream(stdout);
	while (reader.read(&message)) {
		CategoryFilter filter;
		if (filters.contains(message.readCategoryId)) {
			filter = filters.value(message.readCategoryId);
		} else {
			for (const auto& rule: rules) {
				auto filterpass = rule.pass(message.category, QtDebugMsg);
				if (filterpass != 0) filter.debug = filterpass > 0;

				filterpass = rule.pass(message.category, QtInfoMsg);
				if (filterpass != 0) filter.info = filterpass > 0;

				filterpass = rule.pass(message.category, QtWarningMsg);
				if (filterpass != 0) filter.warn = filterpass > 0;

				filterpass = rule.pass(message.category, QtCriticalMsg);
				if (filterpass != 0) filter.critical = filterpass > 0;
			}

			filters.insert(message.readCategoryId, filter);
		}

		if (filter.shouldDisplay(message.type)) {
			LogMessage::formatMessage(stream, message, color, timestamps);
			stream << '\n';
		}
	}

	stream << Qt::flush;

	if (!device->atEnd()) {
		qCritical() << "An error occurred parsing the end of this log file.";
		qCritical() << "Remaining data:" << device->readAll();
	}

	return true;
}

} // namespace qs::log
