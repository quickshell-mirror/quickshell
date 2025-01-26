#pragma once

#include <qlist.h>
#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qqmlparserstatus.h>
#include <qrunnable.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qurl.h>

class ColorQuantizerOperation
    : public QObject
    , public QRunnable {
	Q_OBJECT;

public:
	explicit ColorQuantizerOperation(QUrl* source, qreal depth, qreal rescaleSize);

	void run() override;
	void tryCancel();

signals:
	void done(QList<QColor> colors);

private slots:
	void finished();

private:
	static char findBiggestColorRange(const QList<QColor>& rgbValues);

	void quantizeImage(const QAtomicInteger<bool>& shouldCancel = false);

	QList<QColor> quantization(
	    QList<QColor>& rgbValues,
	    qreal depth,
	    const QAtomicInteger<bool>& shouldCancel = false
	);

	void finishRun();

	QAtomicInteger<bool> shouldCancel = false;
	QList<QColor> colors;
	QUrl* source;
	qreal maxDepth;
	qreal rescaleSize;
};

///! Color Quantization Utility
/// A color quantization utility used for getting prevalent colors in an image, by
/// averaging out the image's color data recursively.
///
/// #### Example
/// ```qml
/// ColorQuantizer {
///   id: colorQuantizer
///   source: Qt.resolvedUrl("./yourImage.png")
///   depth: 3 // Will produce 8 colors (2³)
///   rescaleSize: 64 // Rescale to 64x64 for faster processing
/// }
/// ```
class ColorQuantizer
    : public QObject
    , public QQmlParserStatus {
	Q_OBJECT;
	QML_ELEMENT;
	Q_INTERFACES(QQmlParserStatus);
	/// Access the colors resulting from the color quantization performed.
	/// > [!NOTE] The amount of colors returned from the quantization is determined by
	/// > the property depth, specifically 2ⁿ where n is the depth.
	Q_PROPERTY(QList<QColor> colors READ default NOTIFY colorsChanged BINDABLE bindableColors);

	/// Path to the image you'd like to run the color quantization on.
	Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged);

	/// Max depth for the color quantization. Each level of depth represents another
	/// binary split of the color space
	Q_PROPERTY(qreal depth READ depth WRITE setDepth NOTIFY depthChanged);

	/// The size to rescale the image to, when rescaleSize is 0 then no scaling will be done.
	/// > [!NOTE] Results from color quantization doesn't suffer much when rescaling, it's
	/// > reccommended to rescale, otherwise the quantization process will take much longer.
	Q_PROPERTY(qreal rescaleSize READ rescaleSize WRITE setRescaleSize NOTIFY rescaleSizeChanged);

public:
	explicit ColorQuantizer(QObject* parent = nullptr): QObject(parent) {}

	void componentComplete() override;
	void classBegin() override {}

	[[nodiscard]] QBindable<QList<QColor>> bindableColors() { return &this->bColors; }

	[[nodiscard]] QUrl source() const { return mSource; }
	void setSource(const QUrl& source);

	[[nodiscard]] qreal depth() const { return mDepth; }
	void setDepth(qreal depth);

	[[nodiscard]] qreal rescaleSize() const { return mRescaleSize; }
	void setRescaleSize(int rescaleSize);

signals:
	void colorsChanged();
	void sourceChanged();
	void depthChanged();
	void rescaleSizeChanged();

public slots:
	void operationFinished(const QList<QColor>& result);

private:
	void quantizeAsync();
	void cancelAsync();

	bool componentCompleted = false;
	ColorQuantizerOperation* liveOperation = nullptr;
	QUrl mSource;
	qreal mDepth = 0;
	qreal mRescaleSize = 0;

	Q_OBJECT_BINDABLE_PROPERTY(
	    ColorQuantizer,
	    QList<QColor>,
	    bColors,
	    &ColorQuantizer::colorsChanged
	);
};
