#pragma once

#include <utility>

#include <qatomic.h>
#include <qdebug.h>
#include <qfilesystemwatcher.h>
#include <qlogging.h>
#include <qmutex.h>
#include <qobject.h>
#include <qpointer.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qqmlparserstatus.h>
#include <qrunnable.h>
#include <qstringview.h>
#include <qtclasshelpermacros.h>
#include <qtmetamacros.h>

#include "../core/doc.hpp"
#include "../core/util.hpp"

namespace qs::io {

class FileViewError: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_SINGLETON;

public:
	enum Enum : quint8 {
		/// No error occured.
		Success = 0,
		/// An unknown error occured. Check the logs for details.
		Unknown = 1,
		/// The file to read does not exist.
		FileNotFound = 2,
		/// Permission to read/write the file was not granted, or permission
		/// to create parent directories was not granted when writing the file.
		PermissionDenied = 3,
		/// The specified path to read/write exists and was not a file.
		NotAFile = 4,
	};
	Q_ENUM(Enum);

	Q_INVOKABLE static QString toString(qs::io::FileViewError::Enum value);
};

struct FileViewData {
	FileViewData() = default;
	FileViewData(QString text): text(std::move(text)) {}
	FileViewData(QByteArray data): data(std::move(data)) {}

	[[nodiscard]] bool operator==(const FileViewData& other) const;
	[[nodiscard]] bool isEmpty() const;

	operator const QString&() const;
	operator const QByteArray&() const;

private:
	mutable QString text;
	mutable QByteArray data;
};

struct FileViewState {
	FileViewState() = default;
	explicit FileViewState(QString path): path(std::move(path)) {}

	QString path;
	FileViewData data;
	bool exists = false;
	bool printErrors = true;
	FileViewError::Enum error = FileViewError::Success;
};

class FileView;

class FileViewOperation
    : public QObject
    , public QRunnable {
	Q_OBJECT;

public:
	explicit FileViewOperation(FileView* owner);

	void block();

	// Attempt to cancel the operation, which may or may not be possible.
	// If possible, block() returns sooner.
	void tryCancel();

	FileViewState state;

signals:
	void done();

private slots:
	void finished();

protected:
	QMutex blockMutex;
	QPointer<FileView> owner;
	QAtomicInteger<bool> shouldCancel = false;

	void finishRun();
};

class FileViewReader: public FileViewOperation {
public:
	explicit FileViewReader(FileView* owner, bool doStringConversion)
	    : FileViewOperation(owner)
	    , doStringConversion(doStringConversion) {}

	void run() override;

	static void read(
	    FileView* view,
	    FileViewState& state,
	    bool doStringConversion,
	    const QAtomicInteger<bool>& shouldCancel = false
	);

	bool doStringConversion;
};

class FileViewWriter: public FileViewOperation {
public:
	explicit FileViewWriter(FileView* owner, bool doAtomicWrite)
	    : FileViewOperation(owner)
	    , doAtomicWrite(doAtomicWrite) {}

	void run() override;

	static void write(
	    FileView* view,
	    FileViewState& state,
	    bool doAtomicWrite,
	    const QAtomicInteger<bool>& shouldCancel = false
	);

	bool doAtomicWrite;
};

class FileViewAdapter;

///! Simple accessor for small files.
/// A reader for small to medium files that don't need seeking/cursor access,
/// suitable for most text files.
///
/// #### Example: Reading a JSON as text
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
///
/// Also see @@JsonAdapter for an alternative way to handle reading and writing JSON files.
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
	/// If true (default false), all calls to @@setText() or @@setData() will block the
	/// UI thread until the write succeeds or fails.
	///
	/// > [!WARNING] Blocking operations should be used carefully to avoid stutters and other performance
	/// > degradations. Blocking means that your interface **WILL NOT FUNCTION** during the call.
	Q_PROPERTY(bool blockWrites READ default WRITE default NOTIFY blockWritesChanged BINDABLE bindableBlockWrites);
	/// If true (default), all calls to @@setText() or @@setData() will be performed atomically,
	/// meaning if the write fails for any reason, the file will not be modified.
	///
	/// > [!NOTE] This works by creating another file with the desired content, and renaming
	/// > it over the existing file if successful.
	Q_PROPERTY(bool atomicWrites READ default WRITE default NOTIFY blockWritesChanged BINDABLE bindableAtomicWrites);
	/// If true (default), read or write errors will be printed to the quickshell logs.
	/// If false, all known errors will not be printed.
	QSDOC_PROPERTY_OVERRIDE(bool printErrors READ default WRITE default NOTIFY printErrorsChanged);
	/// If true (defaule false), @@fileChanged() will be called whenever the content of the file
	/// changes on disk, including when @@setText() or @@setData() are used.
	///
	/// > [!NOTE] You can reload the file's content whenever it changes on disk like so:
	/// > ```qml
	/// > FileView {
	/// >   // ...
	/// >   watchChanges: true
	/// >   onFileChanged: this.reload()
	/// > }
	/// > ```
	Q_PROPERTY(bool watchChanges READ default WRITE default NOTIFY watchChangesChanged BINDABLE bindableWatchChanges);
	/// In addition to directly reading/writing the file as text, *adapters* can be used to
	/// expose a file's content in new ways.
	///
	/// An adapter will automatically be given the loaded file's content.
	/// Its state may be saved with @@writeAdapter().
	///
	/// Currently the only adapter is @@JsonAdapter.
	Q_PROPERTY(FileViewAdapter* adapter READ adapter WRITE setAdapter NOTIFY adapterChanged);

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
	QSDOC_HIDE Q_PROPERTY(bool __printErrors READ default WRITE default NOTIFY printErrorsChanged BINDABLE bindablePrintErrors);
	// clang-format on
	Q_CLASSINFO("DefaultProperty", "adapter");
	QML_NAMED_ELEMENT(FileViewInternal);
	QSDOC_NAMED_ELEMENT(FileView);

public:
	explicit FileView(QObject* parent = nullptr): QObject(parent) {}
	~FileView() override;
	Q_DISABLE_COPY_MOVE(FileView);

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
	Q_INVOKABLE bool waitForJob();
	/// Unload the loaded file and reload it, usually in response to changes.
	///
	/// This will not block if @@blockLoading is set, only if @@blockAllReads is true.
	/// It acts the same as changing @@path to a new file, except loading the same file.
	Q_INVOKABLE void reload();
	/// Write the content of the current @@adapter to the selected file.
	Q_INVOKABLE void writeAdapter();

	[[nodiscard]] QString path() const;
	void setPath(const QString& path);

	[[nodiscard]] QByteArray data();
	[[nodiscard]] QString text();

	// These generally should not be called prior to component completion, making it safe not to force
	// property resolution.

	/// Sets the content of the file specified by @@path as an [ArrayBuffer].
	///
	/// @@atomicWrites and @@blockWrites affect the behavior of this function.
	///
	/// @@saved(s) or @@saveFailed(s) will be emitted on completion.
	Q_INVOKABLE void setData(const QByteArray& data);
	/// Sets the content of the file specified by @@path as text.
	///
	/// @@atomicWrites and @@blockWrites affect the behavior of this function.
	///
	/// @@saved(s) or @@saveFailed(s) will be emitted on completion.
	///
	/// [ArrayBuffer]: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/ArrayBuffer
	Q_INVOKABLE void setText(const QString& text);

	// Const bindables functions silently do nothing on setValue.
	[[nodiscard]] QBindable<bool> bindableBlockWrites() { return &this->bBlockWrites; }
	[[nodiscard]] QBindable<bool> bindableAtomicWrites() { return &this->bAtomicWrites; }

	[[nodiscard]] QBindable<bool> bindablePrintErrors() { return &this->bPrintErrors; }
	[[nodiscard]] QBindable<bool> bindableWatchChanges() { return &this->bWatchChanges; }

	[[nodiscard]] FileViewAdapter* adapter() const;
	void setAdapter(FileViewAdapter* adapter);

signals:
	/// Emitted if the file was loaded successfully.
	void loaded();
	/// Emitted if the file failed to load.
	void loadFailed(qs::io::FileViewError::Enum error);
	/// Emitted if the file was saved successfully.
	void saved();
	/// Emitted if the file failed to save.
	void saveFailed(qs::io::FileViewError::Enum error);
	/// Emitted if the file changes on disk and @@watchChanges is true.
	void fileChanged();
	/// Emitted when the active @@adapter$'s data is changed.
	void adapterUpdated();

	void pathChanged();
	QSDOC_HIDE void internalTextChanged();
	QSDOC_HIDE void internalDataChanged();
	QSDOC_HIDE void textChanged();
	QSDOC_HIDE void dataChanged();
	void preloadChanged();
	void loadedOrAsyncChanged();
	void blockLoadingChanged();
	void blockAllReadsChanged();
	void blockWritesChanged();
	void atomicWritesChanged();
	void printErrorsChanged();
	void watchChangesChanged();
	void adapterChanged();

private slots:
	void operationFinished();
	void onAdapterDestroyed();

private:
	void loadAsync(bool doStringConversion);
	void saveAsync();
	void cancelAsync();
	void loadSync();
	void saveSync();
	void updateState(FileViewState& newState);
	void updatePath();
	void updateWatchedFiles();
	void onWatchedFileChanged();
	void onWatchedDirectoryChanged();

	[[nodiscard]] bool shouldBlockRead() const;
	[[nodiscard]] FileViewReader* liveReader() const;
	[[nodiscard]] FileViewWriter* liveWriter() const;
	[[nodiscard]] const FileViewData& writeCmpData() const;

	FileViewState state;
	FileViewData writeData;
	FileViewOperation* liveOperation = nullptr;
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

	FileViewAdapter* mAdapter = nullptr;
	QFileSystemWatcher* watcher = nullptr;

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

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(FileView, bool, bBlockWrites, &FileView::blockWritesChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FileView, bool, bAtomicWrites, true, &FileView::atomicWritesChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FileView, bool, bPrintErrors, true, &FileView::printErrorsChanged);
	Q_OBJECT_BINDABLE_PROPERTY(FileView, bool, bWatchChanges, &FileView::watchChangesChanged);
	// clang-format on

	QS_BINDING_SUBSCRIBE_METHOD(FileView, bWatchChanges, updateWatchedFiles, onValueChanged);

	void setPreload(bool preload);
	void setBlockLoading(bool blockLoading);
	void setBlockAllReads(bool blockAllReads);
};

/// See @@FileView.adapter.
class FileViewAdapter: public QObject {
	Q_OBJECT;
	QML_ELEMENT;
	QML_UNCREATABLE("");

public:
	void setFileView(FileView* fileView);
	virtual void deserializeAdapter(const QByteArray& data) = 0;
	[[nodiscard]] virtual QByteArray serializeAdapter() = 0;

signals:
	/// This signal is fired when data in the adapter changes, and triggers @@FileView.adapterUpdated(s).
	void adapterUpdated();

private slots:
	void onDataChanged();

protected:
	FileView* mFileView = nullptr;
};

} // namespace qs::io
