#include "fileview.hpp"
#include <array>
#include <utility>

#include <qatomic.h>
#include <qdir.h>
#include <qfiledevice.h>
#include <qfileinfo.h>
#include <qfilesystemwatcher.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qmutex.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qqmlinfo.h>
#include <qsavefile.h>
#include <qscopedpointer.h>
#include <qthreadpool.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/logcat.hpp"
#include "../core/util.hpp"

namespace qs::io {

namespace {
QS_LOGGING_CATEGORY(logFileView, "quickshell.io.fileview", QtWarningMsg);
}

QString FileViewError::toString(FileViewError::Enum value) {
	switch (value) {
	case Success: return "Success";
	case Unknown: return "An unknown error has occurred";
	case FileNotFound: return "The specified file does not exist";
	case PermissionDenied: return "Permission denied";
	case NotAFile: return "The specified path was not a file";
	default: return "Invalid error";
	}
}

bool FileViewData::operator==(const FileViewData& other) const {
	if (this->data == other.data && !this->data.isEmpty()) return true;
	if (this->text == other.text && !this->text.isEmpty()) return true;
	return this->operator const QByteArray&() == other.operator const QByteArray&();
}

bool FileViewData::isEmpty() const { return this->data.isEmpty() && this->text.isEmpty(); }

FileViewData::operator const QString&() const {
	if (this->text.isEmpty() && !this->data.isEmpty()) {
		this->text = QString::fromUtf8(this->data);
	}

	return this->text;
}

FileViewData::operator const QByteArray&() const {
	if (this->data.isEmpty() && !this->text.isEmpty()) {
		this->data = this->text.toUtf8();
	}

	return this->data;
}

FileViewOperation::FileViewOperation(FileView* owner): owner(owner) {
	this->setAutoDelete(false);
	this->blockMutex.lock();
}

void FileViewOperation::block() {
	// block until a lock can be acauired, then immediately drop it
	auto unused = QMutexLocker(&this->blockMutex);
}

void FileViewOperation::tryCancel() { this->shouldCancel.storeRelease(true); }

void FileViewOperation::finishRun() {
	this->blockMutex.unlock();
	QMetaObject::invokeMethod(this, &FileViewOperation::finished, Qt::QueuedConnection);
}

void FileViewOperation::finished() {
	emit this->done();
	// Delete happens on the main thread, after done(), meaning no operation accesses
	// will be a UAF.
	delete this;
}

void FileViewReader::run() {
	if (!this->shouldCancel) {
		FileViewReader::read(this->owner, this->state, this->doStringConversion, this->shouldCancel);

		if (this->shouldCancel.loadAcquire()) {
			qCDebug(logFileView) << "Read" << this << "of" << state.path << "canceled for" << this->owner;
		}
	}

	this->finishRun();
}

void FileViewReader::read(
    FileView* view,
    FileViewState& state,
    bool doStringConversion,
    const QAtomicInteger<bool>& shouldCancel
) {
	qCDebug(logFileView) << "Reader started for" << state.path;

	auto info = QFileInfo(state.path);
	state.exists = info.exists();

	if (!state.exists) {
		if (state.printErrors) {
			qmlWarning(view) << "Read of " << state.path << " failed: File does not exist.";
		}

		state.error = FileViewError::FileNotFound;
		return;
	}

	if (!info.isFile()) {
		if (state.printErrors) {
			qmlWarning(view) << "Read of " << state.path << " failed: Not a file.";
		}

		state.error = FileViewError::NotAFile;
		return;
	} else if (!info.isReadable()) {
		if (state.printErrors) {
			qmlWarning(view) << "Read of " << state.path << " failed: Permission denied.";
		}

		state.error = FileViewError::PermissionDenied;
		return;
	}

	if (shouldCancel.loadAcquire()) return;

	auto file = QFile(state.path);

	if (!file.open(QFile::ReadOnly)) {
		qmlWarning(view) << "Read of " << state.path << " failed: Unknown failure when opening file.";
		state.error = FileViewError::Unknown;
		return;
	}

	if (shouldCancel.loadAcquire()) return;

	if (file.size() != 0) {
		auto data = QByteArray(file.size(), Qt::Uninitialized);
		qint64 i = 0;

		while (true) {
			if (shouldCancel.loadAcquire()) return;

			auto r = file.read(data.data() + i, data.length() - i); // NOLINT

			if (r == -1) {
				qmlWarning(view) << "Read of " << state.path << " failed: read() failed.";

				state.error = FileViewError::Unknown;
				return;
			} else if (r == 0) {
				data.resize(i);
				break;
			}

			i += r;
		}

		state.data = data;
	} else { // Mostly happens in /proc and friends, which have zero sized files with content.
		QByteArray data;
		auto buf = std::array<char, 4096>();

		while (true) {
			if (shouldCancel.loadAcquire()) return;

			auto r = file.read(buf.data(), buf.size()); // NOLINT

			if (r == -1) {
				qmlWarning(view) << "Read of " << state.path << " failed: read() failed.";

				state.error = FileViewError::Unknown;
				return;
			} else {
				data.append(buf.data(), r);
				if (r == 0) break;
			}
		}

		state.data = data;
	}

	if (shouldCancel.loadAcquire()) return;

	if (doStringConversion) {
		state.data.operator const QString&();
	}
}

void FileViewWriter::run() {
	if (!this->shouldCancel.loadAcquire()) {
		FileViewWriter::write(this->owner, this->state, this->doAtomicWrite, this->shouldCancel);

		if (this->shouldCancel.loadAcquire()) {
			qCDebug(logFileView) << "Write" << this << "of" << state.path << "canceled for"
			                     << this->owner;
		}
	}

	this->finishRun();
}

void FileViewWriter::write(
    FileView* view,
    FileViewState& state,
    bool doAtomicWrite,
    const QAtomicInteger<bool>& shouldCancel
) {
	qCDebug(logFileView) << "Writer started for" << state.path;

	auto info = QFileInfo(state.path);
	state.exists = info.exists();

	if (!state.exists) {
		auto dir = info.dir();
		if (!dir.mkpath(".")) {
			if (state.printErrors) {
				qmlWarning(view) << "Write of " << state.path
				                 << " failed: Could not create parent directories of file.";
			}

			state.error = FileViewError::PermissionDenied;
			return;
		}
	} else if (!info.isWritable()) {
		if (state.printErrors) {
			qmlWarning(view) << "Write of " << state.path << " failed: Permission denied.";
		}

		state.error = FileViewError::PermissionDenied;
		return;
	}

	if (shouldCancel.loadAcquire()) return;

	QScopedPointer<QFileDevice> file;
	if (doAtomicWrite) {
		file.reset(new QSaveFile(state.path));
	} else {
		file.reset(new QFile(state.path));
	}

	if (!file->open(QFile::WriteOnly)) {
		qmlWarning(view) << "Write of " << state.path << " failed: Unknown error when opening file.";
		state.error = FileViewError::Unknown;
		return;
	}

	if (shouldCancel.loadAcquire()) return;

	const QByteArray& data = state.data;
	qint64 i = 0;

	while (true) {
		if (shouldCancel.loadAcquire()) return;

		auto r = file->write(data.data() + i, data.length() - i); // NOLINT

		if (r == -1) {
			qmlWarning(view) << "Write of " << state.path << " failed: write() failed.";

			state.error = FileViewError::Unknown;
			return;
		} else {
			i += r;
			if (i == data.length()) break;
		}
	}

	if (shouldCancel.loadAcquire()) return;

	if (doAtomicWrite) {
		if (!reinterpret_cast<QSaveFile*>(file.get())->commit()) {
			qmlWarning(view) << "Write of " << state.path << " failed: Atomic commit failed.";
		}
	}
}

FileView::~FileView() {
	if (this->mAdapter) {
		this->mAdapter->setFileView(nullptr);
	}
}

void FileView::loadAsync(bool doStringConversion) {
	// Writes update via operationFinished, making a read both invalid and outdated.
	if (!this->liveOperation || this->pathInFlight != this->targetPath) {
		this->cancelAsync();
		this->pathInFlight = this->targetPath;

		if (this->targetPath.isEmpty()) {
			auto state = FileViewState();
			this->updateState(state);
		} else {
			qCDebug(logFileView) << "Starting async load for" << this << "of" << this->targetPath;
			auto* reader = new FileViewReader(this, doStringConversion);
			reader->state.path = this->targetPath;
			reader->state.printErrors = this->bPrintErrors;
			QObject::connect(reader, &FileViewOperation::done, this, &FileView::operationFinished);
			QThreadPool::globalInstance()->start(reader); // takes ownership
			this->liveOperation = reader;
		}
	}
}

void FileView::saveAsync() {
	if (this->targetPath.isEmpty()) {
		qmlWarning(this) << "Cannot write file, as no path has been specified.";
		this->writeData = FileViewData();
	} else {
		// cancel will blank the data if waiting
		auto data = this->writeData;

		this->cancelAsync();

		qCDebug(logFileView) << "Starting async save for" << this << "of" << this->targetPath;
		auto* writer = new FileViewWriter(this, this->bAtomicWrites);
		writer->state.path = this->targetPath;
		writer->state.data = std::move(data);
		writer->state.printErrors = this->bPrintErrors;
		QObject::connect(writer, &FileViewOperation::done, this, &FileView::operationFinished);
		QThreadPool::globalInstance()->start(writer); // takes ownership
		this->liveOperation = writer;
	}
}

void FileView::cancelAsync() {
	if (!this->liveOperation) return;
	this->liveOperation->tryCancel();

	if (this->liveReader()) {
		qCDebug(logFileView) << "Disowning async read for" << this;
		QObject::disconnect(this->liveOperation, nullptr, this, nullptr);
		this->liveOperation = nullptr;
	} else if (this->liveWriter()) {
		// We don't want to start a read or write operation in the middle of a write.
		// This really shouldn't block but it isn't worth fixing for now.
		qCDebug(logFileView) << "Blocking on write for" << this;
		this->waitForJob();
	}
}

void FileView::operationFinished() {
	if (this->sender() != this->liveOperation) {
		qCWarning(logFileView) << "got operation finished from dropped operation" << this->sender();
		return;
	}

	qCDebug(logFileView) << "Async operation finished for" << this;
	this->writeData = FileViewData();
	this->updateState(this->liveOperation->state);

	if (this->liveReader()) {
		if (this->state.error) emit this->loadFailed(this->state.error);
		else emit this->loaded();
	} else {
		if (this->state.error) emit this->saveFailed(this->state.error);
		else emit this->saved();
	}

	this->liveOperation = nullptr;
}

void FileView::reload() { this->updatePath(); }

bool FileView::waitForJob() {
	if (this->liveOperation != nullptr) {
		QObject::disconnect(this->liveOperation, nullptr, this, nullptr);
		this->liveOperation->block();
		this->writeData = FileViewData();
		this->updateState(this->liveOperation->state);

		if (this->liveReader()) {
			if (this->state.error) emit this->loadFailed(this->state.error);
			else emit this->loaded();
		} else {
			if (this->state.error) emit this->saveFailed(this->state.error);
			else emit this->saved();
		}

		this->liveOperation = nullptr;
		return true;
	} else return false;
}

void FileView::loadSync() {
	if (this->targetPath.isEmpty()) {
		auto state = FileViewState();
		this->updateState(state);
	} else if (!this->waitForJob()) {
		auto state = FileViewState(this->targetPath);
		state.printErrors = this->bPrintErrors;
		FileViewReader::read(this, state, false);
		this->updateState(state);

		if (this->state.error) emit this->loadFailed(this->state.error);
		else emit this->loaded();
	}
}

void FileView::saveSync() {
	if (this->targetPath.isEmpty()) {
		qmlWarning(this) << "Cannot write file, as no path has been specified.";
		this->writeData = FileViewData();
	} else {
		// Both reads and writes will be outdated.
		if (this->liveOperation) this->cancelAsync();

		auto state = FileViewState(this->targetPath);
		state.data = this->writeData;
		state.printErrors = this->bPrintErrors;
		FileViewWriter::write(this, state, this->bAtomicWrites);
		this->writeData = FileViewData();
		this->updateState(state);

		if (this->state.error) emit this->saveFailed(this->state.error);
		else emit this->saved();
	}
}

void FileView::updateState(FileViewState& newState) {
	DEFINE_DROP_EMIT_IF(newState.path != this->state.path, this, pathChanged);
	// assume if the path was changed the data also changed
	auto dataChanged = pathChanged || newState.data != this->state.data;
	// DEFINE_DROP_EMIT_IF(newState.exists != this->state.exists, this, existsChanged);

	this->mPrepared = true;
	auto loadedChanged = this->setLoadedOrAsync(!newState.path.isEmpty() && newState.exists);

	this->state.path = std::move(newState.path);

	if (dataChanged) {
		this->state.data = newState.data;
	}

	this->state.exists = newState.exists;
	this->state.error = newState.error;

	DropEmitter::call(
	    pathChanged,
	    // existsChanged,
	    loadedChanged
	);

	if (dataChanged) this->emitDataChanged();
}

QString FileView::path() const { return this->state.path; }

void FileView::setPath(const QString& path) {
	auto p = path.startsWith("file://") ? path.sliced(7) : path;
	if (p == this->targetPath) return;

	if (this->liveWriter()) {
		this->waitForJob();
	} else {
		this->cancelAsync();
	}

	this->targetPath = p;
	this->updatePath();
}

void FileView::updatePath() {
	this->mPrepared = false;

	if (this->targetPath.isEmpty()) {
		auto state = FileViewState();
		this->updateState(state);
	} else if (this->mPreload) {
		this->loadAsync(true);
	} else {
		this->emitDataChanged();
	}

	this->updateWatchedFiles();
}

void FileView::updateWatchedFiles() {
	// If inotify events are sent to the watcher after deletion and deleteLater
	// isn't used, a use after free in the QML engine will occur.
	if (this->watcher) {
		this->watcher->deleteLater();
		this->watcher = nullptr;
	}

	if (!this->targetPath.isEmpty() && this->bWatchChanges) {
		qCDebug(logFileView) << "Creating watcher for" << this << "at" << this->targetPath;
		this->watcher = new QFileSystemWatcher(this);
		this->watcher->addPath(this->targetPath);

		auto dirPath = this->targetPath;
		if (!dirPath.contains("/")) dirPath = "./" % dirPath;

		if (auto lastIndex = dirPath.lastIndexOf('/'); lastIndex != -1) {
			dirPath = dirPath.sliced(0, lastIndex);
			this->watcher->addPath(dirPath);
		}

		QObject::connect(
		    this->watcher,
		    &QFileSystemWatcher::fileChanged,
		    this,
		    &FileView::onWatchedFileChanged
		);

		QObject::connect(
		    this->watcher,
		    &QFileSystemWatcher::directoryChanged,
		    this,
		    &FileView::onWatchedDirectoryChanged
		);
	}
}

void FileView::onWatchedFileChanged() {
	if (!this->watcher->files().contains(this->targetPath)) {
		this->watcher->addPath(this->targetPath);
	}

	emit this->fileChanged();
}

void FileView::onWatchedDirectoryChanged() {
	if (!this->watcher->files().contains(this->targetPath) && QFileInfo(this->targetPath).exists()) {
		// the file was just created
		this->watcher->addPath(this->targetPath);
		emit this->fileChanged();
	}
}

bool FileView::shouldBlockRead() const {
	return this->mBlockAllReads || (this->mBlockLoading && !this->mLoadedOrAsync);
}

FileViewReader* FileView::liveReader() const {
	return dynamic_cast<FileViewReader*>(this->liveOperation);
}

FileViewWriter* FileView::liveWriter() const {
	return dynamic_cast<FileViewWriter*>(this->liveOperation);
}

const FileViewData& FileView::writeCmpData() const {
	return this->writeData.isEmpty() ? this->state.data : this->writeData;
}

QByteArray FileView::data() {
	auto guard = this->dataChangedEmitter.block();

	if (!this->mPrepared) {
		if (this->shouldBlockRead()) this->loadSync();
		else this->loadAsync(false);
	}

	return this->state.data;
}

QString FileView::text() {
	auto guard = this->textChangedEmitter.block();

	if (!this->mPrepared) {
		if (this->shouldBlockRead()) this->loadSync();
		else this->loadAsync(true);
	}

	return this->state.data;
}

void FileView::setData(const QByteArray& data) {
	if (this->writeCmpData().operator const QByteArray&() == data) return;
	this->writeData = data;

	if (this->bBlockWrites) this->saveSync();
	else this->saveAsync();
}

void FileView::setText(const QString& text) {
	if (this->writeCmpData().operator const QString&() == text) return;
	this->writeData = text;

	if (this->bBlockWrites) this->saveSync();
	else this->saveAsync();
}

void FileView::emitDataChanged() {
	this->dataChangedEmitter.call(this);
	this->textChangedEmitter.call(this);
	emit this->dataChanged();
	emit this->textChanged();
}

DEFINE_MEMBER_GETSET(FileView, isLoadedOrAsync, setLoadedOrAsync);
DEFINE_MEMBER_GET(FileView, shouldPreload);
DEFINE_MEMBER_GET(FileView, blockLoading);
DEFINE_MEMBER_GET(FileView, blockAllReads);

void FileView::setPreload(bool preload) {
	if (preload != this->mPreload) {
		this->mPreload = preload;
		emit this->preloadChanged();

		if (preload) this->emitDataChanged();

		if (!this->mPrepared && this->mPreload) {
			this->loadAsync(false);
		}
	}
}

void FileView::setBlockLoading(bool blockLoading) {
	if (blockLoading != this->mBlockLoading) {
		auto wasBlocking = this->shouldBlockRead();

		this->mBlockLoading = blockLoading;
		emit this->blockLoadingChanged();

		if (!wasBlocking && this->shouldBlockRead()) {
			this->emitDataChanged();
		}
	}
}

void FileView::setBlockAllReads(bool blockAllReads) {
	if (blockAllReads != this->mBlockAllReads) {
		auto wasBlocking = this->shouldBlockRead();

		this->mBlockAllReads = blockAllReads;
		emit this->blockAllReadsChanged();

		if (!wasBlocking && this->shouldBlockRead()) {
			this->emitDataChanged();
		}
	}
}

FileViewAdapter* FileView::adapter() const { return this->mAdapter; }

void FileView::setAdapter(FileViewAdapter* adapter) {
	if (adapter == this->mAdapter) return;

	if (this->mAdapter) {
		this->mAdapter->setFileView(nullptr);
		QObject::disconnect(this->mAdapter, nullptr, this, nullptr);
	}

	this->mAdapter = adapter;

	if (adapter) {
		this->mAdapter->setFileView(this);
		QObject::connect(adapter, &FileViewAdapter::adapterUpdated, this, &FileView::adapterUpdated);
		QObject::connect(adapter, &QObject::destroyed, this, &FileView::onAdapterDestroyed);
	}

	emit this->adapterChanged();
}

void FileView::writeAdapter() {
	if (!this->mAdapter) {
		qmlWarning(this) << "Cannot call writeAdapter without an adapter.";
		return;
	}

	this->setData(this->mAdapter->serializeAdapter());
}

void FileView::onAdapterDestroyed() { this->mAdapter = nullptr; }

void FileViewAdapter::setFileView(FileView* fileView) {
	if (fileView == this->mFileView) return;

	if (this->mFileView) {
		QObject::disconnect(this->mFileView, nullptr, this, nullptr);
	}

	this->mFileView = fileView;

	if (fileView) {
		QObject::connect(fileView, &FileView::dataChanged, this, &FileViewAdapter::onDataChanged);
		this->setFileView(fileView);
	} else {
		this->setFileView(nullptr);
	}
}

void FileViewAdapter::onDataChanged() { this->deserializeAdapter(this->mFileView->data()); }

} // namespace qs::io
