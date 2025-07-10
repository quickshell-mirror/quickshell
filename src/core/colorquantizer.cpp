#include "colorquantizer.hpp"
#include <algorithm>

#include <qatomic.h>
#include <qcolor.h>
#include <qdatetime.h>
#include <qimage.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qminmax.h>
#include <qnamespace.h>
#include <qnumeric.h>
#include <qobject.h>
#include <qqmllist.h>
#include <qrgb.h>
#include <qthreadpool.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "logcat.hpp"

namespace {
QS_LOGGING_CATEGORY(logColorQuantizer, "quickshell.colorquantizer", QtWarningMsg);
}

ColorQuantizerOperation::ColorQuantizerOperation(QUrl* source, qreal depth, qreal rescaleSize)
    : source(source)
    , maxDepth(depth)
    , rescaleSize(rescaleSize) {
	setAutoDelete(false);
}

void ColorQuantizerOperation::quantizeImage(const QAtomicInteger<bool>& shouldCancel) {
	if (shouldCancel.loadAcquire() || source->isEmpty()) return;

	colors.clear();

	auto image = QImage(source->toLocalFile());
	if ((image.width() > rescaleSize || image.height() > rescaleSize) && rescaleSize > 0) {
		image = image.scaled(
		    static_cast<int>(rescaleSize),
		    static_cast<int>(rescaleSize),
		    Qt::KeepAspectRatio,
		    Qt::SmoothTransformation
		);
	}

	if (image.isNull()) {
		qCWarning(logColorQuantizer) << "Failed to load image from" << source->toString();
		return;
	}

	QList<QColor> pixels;
	for (int y = 0; y != image.height(); ++y) {
		for (int x = 0; x != image.width(); ++x) {
			auto pixel = image.pixel(x, y);
			if (qAlpha(pixel) == 0) continue;

			pixels.append(QColor::fromRgb(pixel));
		}
	}

	auto startTime = QDateTime::currentDateTime();

	colors = quantization(pixels, 0);

	auto endTime = QDateTime::currentDateTime();
	auto milliseconds = startTime.msecsTo(endTime);
	qCDebug(logColorQuantizer) << "Color Quantization took: " << milliseconds << "ms";
}

QList<QColor> ColorQuantizerOperation::quantization(
    QList<QColor>& rgbValues,
    qreal depth,
    const QAtomicInteger<bool>& shouldCancel
) {
	if (shouldCancel.loadAcquire()) return QList<QColor>();

	if (depth >= maxDepth || rgbValues.isEmpty()) {
		if (rgbValues.isEmpty()) return QList<QColor>();

		auto totalR = 0;
		auto totalG = 0;
		auto totalB = 0;

		for (const auto& color: rgbValues) {
			if (shouldCancel.loadAcquire()) return QList<QColor>();

			totalR += color.red();
			totalG += color.green();
			totalB += color.blue();
		}

		auto avgColor = QColor(
		    qRound(totalR / static_cast<double>(rgbValues.size())),
		    qRound(totalG / static_cast<double>(rgbValues.size())),
		    qRound(totalB / static_cast<double>(rgbValues.size()))
		);

		return QList<QColor>() << avgColor;
	}

	auto dominantChannel = findBiggestColorRange(rgbValues);
	std::ranges::sort(rgbValues, [dominantChannel](const auto& a, const auto& b) {
		if (dominantChannel == 'r') return a.red() < b.red();
		else if (dominantChannel == 'g') return a.green() < b.green();
		return a.blue() < b.blue();
	});

	auto mid = rgbValues.size() / 2;

	auto leftHalf = rgbValues.mid(0, mid);
	auto rightHalf = rgbValues.mid(mid);

	QList<QColor> result;
	result.append(quantization(leftHalf, depth + 1));
	result.append(quantization(rightHalf, depth + 1));

	return result;
}

char ColorQuantizerOperation::findBiggestColorRange(const QList<QColor>& rgbValues) {
	if (rgbValues.isEmpty()) return 'r';

	auto rMin = 255;
	auto gMin = 255;
	auto bMin = 255;
	auto rMax = 0;
	auto gMax = 0;
	auto bMax = 0;

	for (const auto& color: rgbValues) {
		rMin = qMin(rMin, color.red());
		gMin = qMin(gMin, color.green());
		bMin = qMin(bMin, color.blue());

		rMax = qMax(rMax, color.red());
		gMax = qMax(gMax, color.green());
		bMax = qMax(bMax, color.blue());
	}

	auto rRange = rMax - rMin;
	auto gRange = gMax - gMin;
	auto bRange = bMax - bMin;

	auto biggestRange = qMax(rRange, qMax(gRange, bRange));
	if (biggestRange == rRange) {
		return 'r';
	} else if (biggestRange == gRange) {
		return 'g';
	} else {
		return 'b';
	}
}

void ColorQuantizerOperation::finishRun() {
	QMetaObject::invokeMethod(this, &ColorQuantizerOperation::finished, Qt::QueuedConnection);
}

void ColorQuantizerOperation::finished() {
	emit this->done(colors);
	delete this;
}

void ColorQuantizerOperation::run() {
	if (!this->shouldCancel) {
		this->quantizeImage();

		if (this->shouldCancel.loadAcquire()) {
			qCDebug(logColorQuantizer) << "Color quantization" << this << "cancelled";
		}
	}

	this->finishRun();
}

void ColorQuantizerOperation::tryCancel() { this->shouldCancel.storeRelease(true); }

void ColorQuantizer::componentComplete() {
	componentCompleted = true;
	if (!mSource.isEmpty()) quantizeAsync();
}

void ColorQuantizer::setSource(const QUrl& source) {
	if (mSource != source) {
		mSource = source;
		emit this->sourceChanged();

		if (this->componentCompleted && !mSource.isEmpty()) quantizeAsync();
	}
}

void ColorQuantizer::setDepth(qreal depth) {
	if (mDepth != depth) {
		mDepth = depth;
		emit this->depthChanged();

		if (this->componentCompleted) quantizeAsync();
	}
}

void ColorQuantizer::setRescaleSize(int rescaleSize) {
	if (mRescaleSize != rescaleSize) {
		mRescaleSize = rescaleSize;
		emit this->rescaleSizeChanged();

		if (this->componentCompleted) quantizeAsync();
	}
}

void ColorQuantizer::operationFinished(const QList<QColor>& result) {
	bColors = result;
	this->liveOperation = nullptr;
	emit this->colorsChanged();
}

void ColorQuantizer::quantizeAsync() {
	if (this->liveOperation) this->cancelAsync();

	qCDebug(logColorQuantizer) << "Starting color quantization asynchronously";
	this->liveOperation = new ColorQuantizerOperation(&mSource, mDepth, mRescaleSize);

	QObject::connect(
	    this->liveOperation,
	    &ColorQuantizerOperation::done,
	    this,
	    &ColorQuantizer::operationFinished
	);

	QThreadPool::globalInstance()->start(this->liveOperation);
}

void ColorQuantizer::cancelAsync() {
	if (!this->liveOperation) return;

	this->liveOperation->tryCancel();
	QThreadPool::globalInstance()->waitForDone();

	QObject::disconnect(this->liveOperation, nullptr, this, nullptr);
	this->liveOperation = nullptr;
}
