#include "fileview.hpp"
#include <utility>

#include <qfileinfo.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qmutex.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>
#include <qthreadpool.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/util.hpp"

namespace qs::io {

Q_LOGGING_CATEGORY(logFileView, "quickshell.io.fileview", QtWarningMsg);

FileViewReader::FileViewReader(QString path, bool doStringConversion)
    : doStringConversion(doStringConversion) {
	this->state.path = std::move(path);
	this->setAutoDelete(false);
	this->blockMutex.lock();
}

void FileViewReader::run() {
	FileViewReader::read(this->state, this->doStringConversion);

	this->blockMutex.unlock();
	QMetaObject::invokeMethod(this, &FileViewReader::finished, Qt::QueuedConnection);
}

void FileViewReader::block() {
	// block until a lock can be acauired, then immediately drop it
	auto unused = QMutexLocker(&this->blockMutex);
}

void FileViewReader::finished() {
	emit this->done();
	delete this;
}

void FileViewReader::read(FileViewState& state, bool doStringConversion) {
	{
		qCDebug(logFileView) << "Reader started for" << state.path;

		auto info = QFileInfo(state.path);
		state.exists = info.exists();

		if (!state.exists) return;

		if (!info.isFile()) {
			qCCritical(logFileView) << state.path << "is not a file.";
			goto error;
		} else if (!info.isReadable()) {
			qCCritical(logFileView) << "No permission to read" << state.path;
			state.error = true;
			goto error;
		}

		auto file = QFile(state.path);

		if (!file.open(QFile::ReadOnly)) {
			qCCritical(logFileView) << "Failed to open" << state.path;
			goto error;
		}

		auto& data = state.data;
		data = QByteArray(file.size(), Qt::Uninitialized);

		qint64 i = 0;

		while (true) {
			auto r = file.read(data.data() + i, data.length() - i); // NOLINT

			if (r == -1) {
				qCCritical(logFileView) << "Failed to read" << state.path;
				goto error;
			} else if (r == 0) {
				data.resize(i);
				break;
			}

			i += r;
		}

		if (doStringConversion) {
			state.text = QString::fromUtf8(state.data);
			state.textDirty = false;
		} else {
			state.textDirty = true;
		}

		return;
	}

error:
	state.error = true;
}

void FileView::loadAsync(bool doStringConversion) {
	if (!this->reader || this->pathInFlight != this->targetPath) {
		this->cancelAsync();
		this->pathInFlight = this->targetPath;

		if (this->targetPath.isEmpty()) {
			auto state = FileViewState();
			this->updateState(state);
		} else {
			qCDebug(logFileView) << "Starting async load for" << this << "of" << this->targetPath;
			this->reader = new FileViewReader(this->targetPath, doStringConversion);
			QObject::connect(this->reader, &FileViewReader::done, this, &FileView::readerFinished);
			QThreadPool::globalInstance()->start(this->reader); // takes ownership
		}
	}
}

void FileView::cancelAsync() {
	if (this->reader) {
		qCDebug(logFileView) << "Disowning async read for" << this;
		QObject::disconnect(this->reader, nullptr, this, nullptr);
		this->reader = nullptr;
	}
}

void FileView::readerFinished() {
	if (this->sender() != this->reader) {
		qCWarning(logFileView) << "got read finished from dropped FileViewReader" << this->sender();
		return;
	}

	qCDebug(logFileView) << "Async load finished for" << this;
	this->updateState(this->reader->state);
	this->reader = nullptr;
}

void FileView::reload() { this->updatePath(); }

bool FileView::blockUntilLoaded() {
	if (this->reader != nullptr) {
		QObject::disconnect(this->reader, nullptr, this, nullptr);
		this->reader->block();
		this->updateState(this->reader->state);
		this->reader = nullptr;
		return true;
	} else return false;
}

void FileView::loadSync() {
	if (this->targetPath.isEmpty()) {
		auto state = FileViewState();
		this->updateState(state);
	} else if (!this->blockUntilLoaded()) {
		auto state = FileViewState(this->targetPath);
		FileViewReader::read(state, false);
		this->updateState(state);
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
		this->state.text = newState.text;
		this->state.textDirty = newState.textDirty;
	}

	this->state.exists = newState.exists;
	this->state.error = newState.error;

	DropEmitter::call(
	    pathChanged,
	    // existsChanged,
	    loadedChanged
	);

	if (dataChanged) this->emitDataChanged();

	if (this->state.error) emit this->loadFailed();
}

void FileView::textConversion() {
	if (this->state.textDirty) {
		this->state.text = QString::fromUtf8(this->state.data);
		this->state.textDirty = false;
	}
}

QString FileView::path() const { return this->state.path; }

void FileView::setPath(const QString& path) {
	auto p = path.startsWith("file://") ? path.sliced(7) : path;
	if (p == this->targetPath) return;
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
		// loadAsync will do this already
		this->cancelAsync();
		this->emitDataChanged();
	}
}

bool FileView::shouldBlock() const {
	return this->mBlockAllReads || (this->mBlockLoading && !this->mLoadedOrAsync);
}

QByteArray FileView::data() {
	auto guard = this->dataChangedEmitter.block();

	if (!this->mPrepared) {
		if (this->shouldBlock()) this->loadSync();
		else this->loadAsync(false);
	}

	return this->state.data;
}

QString FileView::text() {
	auto guard = this->textChangedEmitter.block();

	if (!this->mPrepared) {
		if (this->shouldBlock()) this->loadSync();
		else this->loadAsync(true);
	}

	this->textConversion();
	return this->state.text;
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
		auto wasBlocking = this->shouldBlock();

		this->mBlockLoading = blockLoading;
		emit this->blockLoadingChanged();

		if (!wasBlocking && this->shouldBlock()) {
			this->emitDataChanged();
		}
	}
}

void FileView::setBlockAllReads(bool blockAllReads) {
	if (blockAllReads != this->mBlockAllReads) {
		auto wasBlocking = this->shouldBlock();

		this->mBlockAllReads = blockAllReads;
		emit this->blockAllReadsChanged();

		if (!wasBlocking && this->shouldBlock()) {
			this->emitDataChanged();
		}
	}
}

} // namespace qs::io
