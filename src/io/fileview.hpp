#pragma once

#include <utility>

#include <qlogging.h>
#include <qmutex.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qqmlparserstatus.h>
#include <qrunnable.h>
#include <qtmetamacros.h>

#include "../core/doc.hpp"
#include "../core/util.hpp"

namespace qs::io {

struct FileViewState {
	FileViewState() = default;
	explicit FileViewState(QString path): path(std::move(path)) {}

	QString path;
	QString text;
	QByteArray data;
	bool textDirty = false;
	bool exists = false;
	bool error = false;
};

class FileView;

class FileViewReader
    : public QObject
    , public QRunnable {
	Q_OBJECT;

public:
	explicit FileViewReader(QString path, bool doStringConversion);

	void run() override;
	void block();

	FileViewState state;

	static void read(FileViewState& state, bool doStringConversion);

signals:
	void done();

private slots:
	void finished();

private:
	bool doStringConversion;
	QMutex blockMutex;
};

///! Simplified reader for small files.
/// A reader for small to medium files that don't need seeking/cursor access,
/// suitable for most text files.
///
/// #### Example: Reading a JSON
/// ```qml
/// FileView {
///   id: jsonFile
///   path: Qt.resolvedUrl("./your.json")
///   // Forces the file to be loaded by the time we call JSON.parse().
///   // see blockLoading's property documentation for details.
///   blockLoading: true
/// }
///
/// readonly property var jsonData: JSON.parse(jsonFile.text())
/// ```
class FileView: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The path to the file that should be read, or an empty string to unload the file.
	QSDOC_PROPERTY_OVERRIDE(QString path READ path WRITE setPath NOTIFY pathChanged);
	/// If the file should be loaded in the background immediately when set. Defaults to true.
	///
	/// This may either increase or decrease the amount of time it takes to load the file
	/// depending on how large the file is, how fast its storage is, and how you access its data.
	QSDOC_PROPERTY_OVERRIDE(bool preload READ shouldPreload WRITE setPreload NOTIFY preloadChanged);
	/// If @@text() and @@data() should block all operations until the file is loaded. Defaults to false.
	///
	/// If the file is already loaded, no blocking will occur.
	/// If a file was loaded, and @@path was changed to a new file, no blocking will occur.
	///
	/// > [!WARNING] Blocking operations should be used carefully to avoid stutters and other performance
	/// > degradations. Blocking means that your interface **WILL NOT FUNCTION** during the call.
	/// >
	/// > **We recommend you use a blocking load ONLY for files loaded before the windows of your shell
	/// > are loaded, which happens after `Component.onCompleted` runs for the root component of your shell.**
	/// >
	/// > The most reasonable use case would be to load things like configuration files that the program
	/// > must have available.
	QSDOC_PROPERTY_OVERRIDE(bool blockLoading READ blockLoading WRITE setBlockLoading NOTIFY blockLoadingChanged);
	/// If @@text() and @@data() should block all operations while a file loads. Defaults to false.
	///
	/// This is nearly identical to @@blockLoading, but will additionally block when
	/// a file is loaded and @@path changes.
	///
	/// > [!WARNING] We cannot think of a valid use case for this.
	/// > You almost definitely want @@blockLoading.
	QSDOC_PROPERTY_OVERRIDE(bool blockAllReads READ blockAllReads WRITE setBlockAllReads NOTIFY blockAllReadsChanged);

	QSDOC_HIDE Q_PROPERTY(QString __path READ path WRITE setPath NOTIFY pathChanged);
	QSDOC_HIDE Q_PROPERTY(QString __text READ text NOTIFY internalTextChanged);
	QSDOC_HIDE Q_PROPERTY(QByteArray __data READ data NOTIFY internalDataChanged);
	QSDOC_HIDE Q_PROPERTY(bool __preload READ shouldPreload WRITE setPreload NOTIFY preloadChanged);
	/// If a file is currently loaded, which may or may not be the one currently specified by @@path.
	///
	/// > [!INFO] If a file is loaded, @@path is changed, and a new file is loaded,
	/// > this property will stay true the whole time.
	/// > If @@path is set to an empty string to unload the file it will become false.
	Q_PROPERTY(bool loaded READ isLoadedOrAsync NOTIFY loadedOrAsyncChanged);
	QSDOC_HIDE Q_PROPERTY(bool __blockLoading READ blockLoading WRITE setBlockLoading NOTIFY blockLoadingChanged);
	QSDOC_HIDE Q_PROPERTY(bool __blockAllReads READ blockAllReads WRITE setBlockAllReads NOTIFY blockAllReadsChanged);
	// clang-format on
	QML_NAMED_ELEMENT(FileViewInternal);
	QSDOC_NAMED_ELEMENT(FileView);

public:
	explicit FileView(QObject* parent = nullptr): QObject(parent) {}

	/// Returns the data of the file specified by @@path as text.
	///
	/// If @@blockAllReads is true, all changes to @@path will cause the program to block
	/// when this function is called.
	///
	/// If @@blockLoading is true, reading this property before the file has been loaded
	/// will block, but changing @@path or calling @@reload() will return the old data
	/// until the load completes.
	///
	/// If neither is true, an empty string will be returned if no file is loaded,
	/// otherwise it will behave as in the case above.
	///
	/// > [!INFO] Due to technical limitations, @@text() could not be a property,
	/// > however you can treat it like a property, it will trigger property updates
	/// > as a property would, and the signal `textChanged()` is present.
	//@ Q_INVOKABLE QString text();
	/// Returns the data of the file specified by @@path as an [ArrayBuffer].
	///
	/// If @@blockAllReads is true, all changes to @@path will cause the program to block
	/// when this function is called.
	///
	/// If @@blockLoading is true, reading this property before the file has been loaded
	/// will block, but changing @@path or calling @@reload() will return the old data
	/// until the load completes.
	///
	/// If neither is true, an empty buffer will be returned if no file is loaded,
	/// otherwise it will behave as in the case above.
	///
	/// > [!INFO] Due to technical limitations, @@data() could not be a property,
	/// > however you can treat it like a property, it will trigger property updates
	/// > as a property would, and the signal `dataChanged()` is present.
	///
	/// [ArrayBuffer]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer
	//@ Q_INVOKABLE QByteArray data();

	/// Block all operations until the currently running load completes.
	///
	/// > [!WARNING] See @@blockLoading for an explanation and warning about blocking.
	Q_INVOKABLE bool blockUntilLoaded();
	/// Unload the loaded file and reload it, usually in response to changes.
	///
	/// This will not block if @@blockLoading is set, only if @@blockAllReads is true.
	/// It acts the same as changing @@path to a new file, except loading the same file.
	Q_INVOKABLE void reload();

	[[nodiscard]] QString path() const;
	void setPath(const QString& path);

	[[nodiscard]] QByteArray data();
	[[nodiscard]] QString text();

signals:
	///! Fires if the file failed to load. A warning will be printed in the log.
	void loadFailed();

	void pathChanged();
	QSDOC_HIDE void internalTextChanged();
	QSDOC_HIDE void internalDataChanged();
	QSDOC_HIDE void textChanged();
	QSDOC_HIDE void dataChanged();
	void preloadChanged();
	void loadedOrAsyncChanged();
	void blockLoadingChanged();
	void blockAllReadsChanged();

private slots:
	void readerFinished();

private:
	void loadAsync(bool doStringConversion);
	void cancelAsync();
	void loadSync();
	void updateState(FileViewState& newState);
	void textConversion();
	void updatePath();

	[[nodiscard]] bool shouldBlock() const;

	FileViewState state;
	FileViewReader* reader = nullptr;
	QString pathInFlight;

	QString targetPath;
	bool mAsyncUpdate = true;
	bool mWritable = false;
	bool mCreate = false;
	bool mPreload = true;
	bool mPrepared = false;
	bool mLoadedOrAsync = false;
	bool mBlockLoading = false;
	bool mBlockAllReads = false;

	GuardedEmitter<&FileView::internalTextChanged> textChangedEmitter;
	GuardedEmitter<&FileView::internalDataChanged> dataChangedEmitter;
	void emitDataChanged();

	DECLARE_PRIVATE_MEMBER(
	    FileView,
	    isLoadedOrAsync,
	    setLoadedOrAsync,
	    mLoadedOrAsync,
	    loadedOrAsyncChanged
	);

public:
	DECLARE_MEMBER_WITH_GET(FileView, shouldPreload, mPreload, preloadChanged);
	DECLARE_MEMBER_WITH_GET(FileView, blockLoading, mBlockLoading, blockLoadingChanged);
	DECLARE_MEMBER_WITH_GET(FileView, blockAllReads, mBlockAllReads, blockAllReadsChanged);

	void setPreload(bool preload);
	void setBlockLoading(bool blockLoading);
	void setBlockAllReads(bool blockAllReads);
};

} // namespace qs::io
