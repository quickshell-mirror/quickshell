#pragma once

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qquickitem.h>
#include <qsgnode.h>
#include <qtmetamacros.h>

#include "manager.hpp"

namespace qs::wayland::screencopy {

///! Displays a video stream from other windows or a monitor.
/// ScreencopyView displays live video streams or single captured frames from valid
/// capture sources. See @@captureSource for details on which objects are accepted.
class ScreencopyView: public QQuickItem {
	Q_OBJECT;
	QML_ELEMENT;
	// clang-format off
	/// The object to capture from. Accepts any of the following:
	/// - `null` - Clears the displayed image.
	/// - @@Quickshell.ShellScreen - A monitor.
	///   Requires a compositor that supports `wlr-screencopy-unstable`
	///   or both `ext-image-copy-capture-v1` and `ext-capture-source-v1`.
	/// - @@Quickshell.Wayland.Toplevel - A toplevel window.
	///   Requires a compositor that supports `hyprland-toplevel-export-v1`.
	Q_PROPERTY(QObject* captureSource READ captureSource WRITE setCaptureSource NOTIFY captureSourceChanged);
	/// If true, the system cursor will be painted on the image. Defaults to false.
	Q_PROPERTY(bool paintCursor READ paintCursors WRITE setPaintCursors NOTIFY paintCursorsChanged);
	/// If true, a live video feed from the capture source will be displayed instead of a still image.
	/// Defaults to false.
	Q_PROPERTY(bool live READ live WRITE setLive NOTIFY liveChanged);
	/// If true, the view has content ready to display. Content is not always immediately available,
	/// and this property can be used to avoid displaying it until ready.
	Q_PROPERTY(bool hasContent READ default NOTIFY hasContentChanged BINDABLE bindableHasContent);
	/// The size of the source image. Valid when @@hasContent is true.
	Q_PROPERTY(QSize sourceSize READ default NOTIFY sourceSizeChanged BINDABLE bindableSourceSize);
	/// If nonzero, the width and height constraints set for this property will constrain those
	/// dimensions of the ScreencopyView's implicit size, maintaining the image's aspect ratio.
	Q_PROPERTY(QSizeF constraintSize READ default WRITE default NOTIFY constraintSizeChanged BINDABLE bindableConstraintSize);
	// clang-format on

public:
	explicit ScreencopyView(QQuickItem* parent = nullptr);

	void componentComplete() override;

	/// Capture a single frame. Has no effect if @@live is true.
	Q_INVOKABLE void captureFrame();

	[[nodiscard]] QObject* captureSource() const { return this->mCaptureSource; }
	void setCaptureSource(QObject* captureSource);

	[[nodiscard]] bool paintCursors() const { return this->mPaintCursors; }
	void setPaintCursors(bool paintCursors);

	[[nodiscard]] bool live() const { return this->mLive; }
	void setLive(bool live);

	[[nodiscard]] QBindable<bool> bindableHasContent() { return &this->bHasContent; }
	[[nodiscard]] QBindable<QSize> bindableSourceSize() { return &this->bSourceSize; }
	[[nodiscard]] QBindable<QSizeF> bindableConstraintSize() { return &this->bConstraintSize; }

signals:
	/// The compositor has ended the video stream. Attempting to restart it may or may not work.
	void stopped();

	void captureSourceChanged();
	void paintCursorsChanged();
	void liveChanged();
	void hasContentChanged();
	void sourceSizeChanged();
	void constraintSizeChanged();

protected:
	QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

private slots:
	void onCaptureSourceDestroyed();
	void onFrameCaptured();
	void destroyContextWithUpdate() { this->destroyContext(); }
	void onBuffersReady();

private:
	void destroyContext(bool update = true);
	void createContext();
	void updateImplicitSize();

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(ScreencopyView, bool, bHasContent, &ScreencopyView::hasContentChanged);
	Q_OBJECT_BINDABLE_PROPERTY(ScreencopyView, QSize, bSourceSize, &ScreencopyView::sourceSizeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(ScreencopyView, QSizeF, bConstraintSize, &ScreencopyView::constraintSizeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(ScreencopyView, QSizeF, bImplicitSize, &ScreencopyView::updateImplicitSize);
	// clang-format on

	QObject* mCaptureSource = nullptr;
	bool mPaintCursors = false;
	bool mLive = false;
	ScreencopyContext* context = nullptr;
	bool completed = false;
};

} // namespace qs::wayland::screencopy
