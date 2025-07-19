#include "logging.hpp"
#include <array>
#include <cerrno>
#include <cstdio>

#include <fcntl.h>
#include <qbytearrayview.h>
#include <qcoreapplication.h>
#include <qdatetime.h>
#include <qendian.h>
#include <qfilesystemwatcher.h>
#include <qhash.h>
#include <qhashfunctions.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qpair.h>
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

#include "instanceinfo.hpp"
#include "logcat.hpp"
#include "logging_p.hpp"
#include "logging_qtprivate.cpp" // NOLINT
#include "paths.hpp"
#include "ringbuf.hpp"

QS_LOGGING_CATEGORY(logBare, "quickshell.bare");

namespace qs::log {
using namespace qt_logging_registry;

QS_LOGGING_CATEGORY(logLogging, "quickshell.logging", QtWarningMsg);

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
    bool timestamp,
    const QString& prefix
) {
	if (!prefix.isEmpty()) {
		if (color) stream << "\033[90m";
		stream << '[' << prefix << ']';
		if (timestamp) stream << ' ';
		if (color) stream << "\033[0m";
	}

	if (timestamp) {
		if (color) stream << "\033[90m";
		stream << msg.time.toString("yyyy-MM-dd hh:mm:ss.zzz");
	}

	if (msg.category == "quickshell.bare") {
		if (!prefix.isEmpty()) stream << ' ';
		stream << msg.body;
	} else {
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

void CategoryFilter::apply(QLoggingCategory* category) const {
	category->setEnabled(QtDebugMsg, this->debug);
	category->setEnabled(QtInfoMsg, this->info);
	category->setEnabled(QtWarningMsg, this->warn);
	category->setEnabled(QtCriticalMsg, this->critical);
}

void CategoryFilter::applyRule(
    QLatin1StringView category,
    const qt_logging_registry::QLoggingRule& rule
) {
	auto filterpass = rule.pass(category, QtDebugMsg);
	if (filterpass != 0) this->debug = filterpass > 0;

	filterpass = rule.pass(category, QtInfoMsg);
	if (filterpass != 0) this->info = filterpass > 0;

	filterpass = rule.pass(category, QtWarningMsg);
	if (filterpass != 0) this->warn = filterpass > 0;

	filterpass = rule.pass(category, QtCriticalMsg);
	if (filterpass != 0) this->critical = filterpass > 0;
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
		LogMessage::formatMessage(
		    self->stdoutStream,
		    message,
		    self->colorLogs,
		    self->timestampLogs,
		    self->prefix
		);

		self->stdoutStream << Qt::endl;
	}

	emit self->logMessage(message, display);
}

void LogManager::filterCategory(QLoggingCategory* category) {
	auto* instance = LogManager::instance();

	auto categoryName = QLatin1StringView(category->categoryName());
	auto isQs = categoryName.startsWith(QLatin1StringView("quickshell."));

	CategoryFilter filter;

	// We don't respect log filters for qs logs because some distros like to ship
	// default configs that hide everything. QT_LOGGING_RULES is considered via the filter list.
	if (isQs) {
		// QtDebugMsg == 0, so default
		auto defaultLevel = instance->defaultLevels.value(categoryName);

		filter = CategoryFilter();
		// clang-format off
		filter.debug = instance->mDefaultLevel == QtDebugMsg || defaultLevel == QtDebugMsg;
		filter.info = filter.debug || instance->mDefaultLevel == QtInfoMsg || defaultLevel == QtInfoMsg;
		filter.warn = filter.info || instance->mDefaultLevel == QtWarningMsg || defaultLevel == QtWarningMsg;
		filter.critical = filter.warn || instance->mDefaultLevel == QtCriticalMsg || defaultLevel == QtCriticalMsg;
		// clang-format on
	} else if (instance->lastCategoryFilter) {
		instance->lastCategoryFilter(category);
		filter = CategoryFilter(category);
	}

	for (const auto& rule: *instance->rules) {
		filter.applyRule(categoryName, rule);
	}

	if (isQs && !instance->sparse) {
		// We assume the category name pointer will always be the same and be comparable in the message handler.
		instance->sparseFilters.insert(static_cast<const void*>(category->categoryName()), filter);

		// all enabled by default
		CategoryFilter().apply(category);
	} else {
		filter.apply(category);
	}

	instance->allFilters.insert(categoryName, filter);
}

LogManager* LogManager::instance() {
	static auto* instance = new LogManager(); // NOLINT
	return instance;
}

void LogManager::init(
    bool color,
    bool timestamp,
    bool sparseOnly,
    QtMsgType defaultLevel,
    const QString& rules,
    const QString& prefix
) {
	auto* instance = LogManager::instance();
	instance->colorLogs = color;
	instance->timestampLogs = timestamp;
	instance->sparse = sparseOnly;
	instance->prefix = prefix;
	instance->mDefaultLevel = defaultLevel;
	instance->mRulesString = rules;

	{
		QLoggingSettingsParser parser;
		// Load QT_LOGGING_RULES because we ignore the last category filter for QS messages
		// due to disk config files.
		parser.setContent(qEnvironmentVariable("QT_LOGGING_RULES"));
		instance->rules = new QList(parser.rules());
		parser.setContent(rules);
		instance->rules->append(parser.rules());
	}

	qInstallMessageHandler(&LogManager::messageHandler);

	instance->lastCategoryFilter = QLoggingCategory::installFilter(&LogManager::filterCategory);

	qCDebug(logLogging) << "Creating offthread logger...";
	auto* thread = new QThread();
	instance->threadProxy.moveToThread(thread);
	thread->start();

	QMetaObject::invokeMethod(
	    &instance->threadProxy,
	    &LoggingThreadProxy::initInThread,
	    Qt::BlockingQueuedConnection
	);

	qCDebug(logLogging) << "Logger initialized.";
}

void initLogCategoryLevel(const char* name, QtMsgType defaultLevel) {
	LogManager::instance()->defaultLevels.insert(QLatin1StringView(name), defaultLevel);
}

void LogManager::initFs() {
	QMetaObject::invokeMethod(
	    &LogManager::instance()->threadProxy,
	    "initFs",
	    Qt::BlockingQueuedConnection
	);
}

QString LogManager::rulesString() const { return this->mRulesString; }
QtMsgType LogManager::defaultLevel() const { return this->mDefaultLevel; }
bool LogManager::isSparse() const { return this->sparse; }

CategoryFilter LogManager::getFilter(QLatin1StringView category) {
	return this->allFilters.value(category);
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
		qInfo() << "Saving logs to" << detailedPath;
	}

	// buffered by WriteBuffer
	if (!detailedFile->open(QFile::ReadWrite | QFile::Truncate | QFile::Unbuffered)) {
		qCCritical(logLogging
		) << "Could not start detailed filesystem logger as the log file could not be created:"
		  << detailedPath;
		delete detailedFile;
		detailedFile = nullptr;
	} else {
		auto lock = flock {
		    .l_type = F_WRLCK,
		    .l_whence = SEEK_SET,
		    .l_start = 0,
		    .l_len = 0,
		    .l_pid = 0,
		};

		if (fcntl(detailedFile->handle(), F_SETLK, &lock) != 0) { // NOLINT
			qCWarning(logLogging) << "Unable to set lock marker on detailed log file. --follow from "
			                         "other instances will not work.";
		}

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

	if (!this->detailedWriter.write(msg) || (this->detailedFile && !this->detailedFile->flush())) {
		if (this->detailedFile) {
			qCCritical(logLogging) << "Detailed logger failed to write. Ending detailed logs.";
		}

		this->detailedWriter.setDevice(nullptr);
		this->detailedFile->close();
		this->detailedFile = nullptr;
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

	return CompressedLogType::Info; // unreachable under normal conditions
}

QtMsgType typeOfCompressed(CompressedLogType type) {
	switch (type) {
	case CompressedLogType::Debug: return QtDebugMsg;
	case CompressedLogType::Info: return QtInfoMsg;
	case CompressedLogType::Warn: return QtWarningMsg;
	case CompressedLogType::Critical: return QtCriticalMsg;
	}

	return QtInfoMsg; // unreachable under normal conditions
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

void WriteBuffer::writeU8(quint8 data) { this->writeBytes(reinterpret_cast<char*>(&data), 1); }

void WriteBuffer::writeU16(quint16 data) {
	data = qToLittleEndian(data);
	this->writeBytes(reinterpret_cast<char*>(&data), 2);
}

void WriteBuffer::writeU32(quint32 data) {
	data = qToLittleEndian(data);
	this->writeBytes(reinterpret_cast<char*>(&data), 4);
}

void WriteBuffer::writeU64(quint64 data) {
	data = qToLittleEndian(data);
	this->writeBytes(reinterpret_cast<char*>(&data), 8);
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
	return this->readBytes(reinterpret_cast<char*>(data), 1);
}

bool DeviceReader::readU16(quint16* data) {
	return this->readBytes(reinterpret_cast<char*>(data), 2);
}

bool DeviceReader::readU32(quint32* data) {
	return this->readBytes(reinterpret_cast<char*>(data), 4);
}

bool DeviceReader::readU64(quint64* data) {
	return this->readBytes(reinterpret_cast<char*>(data), 8);
}

void EncodedLogWriter::setDevice(QIODevice* target) { this->buffer.setDevice(target); }
void EncodedLogReader::setDevice(QIODevice* source) { this->reader.setDevice(source); }

constexpr quint8 LOG_VERSION = 2;

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

			if (secondDelta >= 0x1d) {
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
		} else if (next == EncodedLogOpcode::RecentMessageShort
		           || next == EncodedLogOpcode::RecentMessageLong)
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

			if (index >= this->recentMessages.size()) return false;
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

		*slot = LogMessage(msgType, QLatin1StringView(category.first), body, this->lastMessageTime);
		slot->readCategoryId = categoryId;
	}

	this->recentMessages.emplace(*slot);
	return true;
}

CategoryFilter EncodedLogReader::categoryFilterById(quint16 id) {
	return this->categories.value(id).second;
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
	auto readLength = this->reader.peekBytes(reinterpret_cast<char*>(bytes.data()), 7);

	if (bytes[0] != 0xff && readLength >= 1) {
		auto n = *reinterpret_cast<quint8*>(bytes.data());
		if (!this->reader.skip(1)) return false;
		*slot = qFromLittleEndian(n);
	} else if ((bytes[1] != 0xff || bytes[2] != 0xff) && readLength >= 3) {
		auto n = *reinterpret_cast<quint16*>(bytes.data() + 1);
		if (!this->reader.skip(3)) return false;
		*slot = qFromLittleEndian(n);
	} else if (readLength == 7) {
		auto n = *reinterpret_cast<quint32*>(bytes.data() + 3);
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

		auto filter = LogManager::instance()->getFilter(category);
		quint8 flags = 0;
		flags |= filter.debug << 0;
		flags |= filter.info << 1;
		flags |= filter.warn << 2;
		flags |= filter.critical << 3;

		this->buffer.writeU8(flags);
		return id;
	}
}

bool EncodedLogReader::registerCategory() {
	QByteArray name;
	quint8 flags = 0;
	if (!this->readString(&name)) return false;
	if (!this->reader.readU8(&flags)) return false;

	CategoryFilter filter;
	filter.debug = (flags >> 0) & 1;
	filter.info = (flags >> 1) & 1;
	filter.warn = (flags >> 2) & 1;
	filter.critical = (flags >> 3) & 1;

	this->categories.append(qMakePair(name, filter));
	return true;
}

bool LogReader::initialize() {
	this->reader.setDevice(this->file);

	bool readable = false;
	quint8 logVersion = 0;
	quint8 readerVersion = 0;
	if (!this->reader.readHeader(&readable, &logVersion, &readerVersion)) {
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

	return true;
}

bool LogReader::continueReading() {
	auto color = LogManager::instance()->colorLogs;
	auto tailRing = RingBuffer<LogMessage>(this->remainingTail);

	LogMessage message;
	auto stream = QTextStream(stdout);
	auto readCursor = this->file->pos();
	while (this->reader.read(&message)) {
		readCursor = this->file->pos();

		CategoryFilter filter;
		if (this->filters.contains(message.readCategoryId)) {
			filter = this->filters.value(message.readCategoryId);
		} else {
			filter = this->reader.categoryFilterById(message.readCategoryId);

			for (const auto& rule: this->rules) {
				filter.applyRule(message.category, rule);
			}

			this->filters.insert(message.readCategoryId, filter);
		}

		if (filter.shouldDisplay(message.type)) {
			if (this->remainingTail == 0) {
				LogMessage::formatMessage(stream, message, color, this->timestamps);
				stream << '\n';
			} else {
				tailRing.emplace(message);
			}
		}
	}

	if (this->remainingTail != 0) {
		for (auto i = tailRing.size() - 1; i != -1; i--) {
			auto& message = tailRing.at(i);
			LogMessage::formatMessage(stream, message, color, this->timestamps);
			stream << '\n';
		}
	}

	stream << Qt::flush;

	if (this->file->pos() != readCursor) {
		qCritical() << "An error occurred parsing the end of this log file.";
		qCritical() << "Remaining data:" << this->file->readAll();
		return false;
	}

	return true;
}

void LogFollower::FcntlWaitThread::run() {
	auto lock = flock {
	    .l_type = F_RDLCK, // won't block other read locks when we take it
	    .l_whence = SEEK_SET,
	    .l_start = 0,
	    .l_len = 0,
	    .l_pid = 0,
	};

	auto r = fcntl(this->follower->reader->file->handle(), F_SETLKW, &lock); // NOLINT

	if (r != 0) {
		qCWarning(logLogging).nospace()
		    << "Failed to wait for write locks to be removed from log file with error code " << errno
		    << ": " << qt_error_string();
	}
}

bool LogFollower::follow() {
	QObject::connect(&this->waitThread, &QThread::finished, this, &LogFollower::onFileLocked);

	QObject::connect(
	    &this->fileWatcher,
	    &QFileSystemWatcher::fileChanged,
	    this,
	    &LogFollower::onFileChanged
	);

	this->fileWatcher.addPath(this->path);
	this->waitThread.start();

	auto r = QCoreApplication::exec();
	return r == 0;
}

void LogFollower::onFileChanged() {
	if (!this->reader->continueReading()) {
		QCoreApplication::exit(1);
	}
}

void LogFollower::onFileLocked() {
	if (!this->reader->continueReading()) {
		QCoreApplication::exit(1);
	} else {
		QCoreApplication::exit(0);
	}
}

bool readEncodedLogs(
    QFile* file,
    const QString& path,
    bool timestamps,
    int tail,
    bool follow,
    const QString& rulespec
) {
	QList<QLoggingRule> rules;

	{
		QLoggingSettingsParser parser;
		parser.setContent(rulespec);
		rules = parser.rules();
	}

	auto reader = LogReader(file, timestamps, tail, rules);

	if (!reader.initialize()) return false;
	if (!reader.continueReading()) return false;

	if (follow) {
		auto follower = LogFollower(&reader, path);
		return follower.follow();
	}

	return true;
}

} // namespace qs::log
