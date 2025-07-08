#include "lint.hpp"
#include <algorithm>

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <qqmlcontext.h>
#include <qqmlengine.h>
#include <qqmlinfo.h>
#include <qquickitem.h>
#include <qquickwindow.h>
#include <qstringliteral.h>

#include "../core/logcat.hpp"

namespace qs::debug {

namespace {
QS_LOGGING_CATEGORY(logLint, "quickshell.linter", QtWarningMsg);

void lintZeroSized(QQuickItem* item);
bool isRenderable(QQuickItem* item);
} // namespace

void lintObjectTree(QObject* object) {
	if (!logLint().isWarningEnabled()) return;

	for (auto* child: object->children()) {
		if (child->isQuickItemType()) {
			auto* item = static_cast<QQuickItem*>(child); // NOLINT;
			lintItemTree(item);
		} else {
			lintObjectTree(child);
		}
	}
}

void lintItemTree(QQuickItem* item) {
	if (!logLint().isWarningEnabled()) return;

	lintZeroSized(item);

	for (auto* child: item->childItems()) {
		lintItemTree(child);
	}
}

namespace {

void lintZeroSized(QQuickItem* item) {
	if (!item->isEnabled() || !item->isVisible()) return;
	if (item->childItems().isEmpty()) return;

	auto zeroWidth = item->width() == 0;
	auto zeroHeight = item->height() == 0;

	if (!zeroWidth && !zeroHeight) return;

	if (!isRenderable(item)) return;

	auto* ctx = QQmlEngine::contextForObject(item);
	if (!ctx || ctx->baseUrl().scheme() != QStringLiteral("qsintercept")) return;

	qmlWarning(item) << "Item is visible and has visible children, but has zero "
	                 << (zeroWidth && zeroHeight ? "width and height"
	                     : zeroWidth             ? "width"
	                                             : "height");
}

bool isRenderable(QQuickItem* item) {
	if (!item->isEnabled() || !item->isVisible()) return false;

	if (item->flags().testFlags(QQuickItem::ItemHasContents)) {
		return true;
	}

	return std::ranges::any_of(item->childItems(), [](auto* item) { return isRenderable(item); });
}

} // namespace

} // namespace qs::debug
