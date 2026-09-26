#include "keyboard_layout.hpp"
#include <functional>
#include <optional>

#include <qlogging.h>
#include <qloggingcategory.h>
#include <qproperty.h>
#include <qstring.h>
#include <xkbcommon/xkbregistry.h>

#include "../core/logcat.hpp"

namespace qs::keyboard {

namespace {
QS_LOGGING_CATEGORY(logKbLayout, "quickshell.keyboard.layout", QtWarningMsg);

struct Layout {
	QString fullName;
	QString shortName;
	QString variant;
	QString shortDescription;
};

QString optString(const char* s) { return s ? QString::fromUtf8(s) : QString(); }

std::optional<Layout> findLayout(const std::function<bool(rxkb_layout*)>& matcher) {
	auto* context = rxkb_context_new(RXKB_CONTEXT_LOAD_EXOTIC_RULES);
	rxkb_context_parse_default_ruleset(context);

	for (auto* layout = rxkb_layout_first(context); layout != nullptr;
	     layout = rxkb_layout_next(layout))
	{
		if (!matcher(layout)) continue;

		auto name = QString::fromUtf8(rxkb_layout_get_name(layout));
		auto fullName = QString::fromUtf8(rxkb_layout_get_description(layout));
		auto variant = optString(rxkb_layout_get_variant(layout));
		auto description = optString(rxkb_layout_get_brief(layout));

		rxkb_context_unref(context);

		return Layout {
		    .fullName = fullName,
		    .shortName = name,
		    .variant = variant,
		    .shortDescription = description
		};
	}

	rxkb_context_unref(context);

	return std::nullopt;
}
} // namespace

void KeyboardLayout::resolveFromShortName(const QString& shortName) {
	auto result = findLayout([&](rxkb_layout* layout) {
		return shortName == QString::fromUtf8(rxkb_layout_get_name(layout));
	});

	if (!result) {
		qCDebug(logKbLayout) << "didn't find layout for short name" << shortName;
		return;
	}

	Qt::beginPropertyUpdateGroup();
	this->bShortName = result->shortName;
	this->bVariant = result->variant;
	this->bShortDescription = result->shortDescription;
	this->bFullName = result->fullName;
	Qt::endPropertyUpdateGroup();
}

void KeyboardLayout::resolveFromFullName(const QString& fullName) {
	auto result = findLayout([&](rxkb_layout* layout) {
		return fullName == QString::fromUtf8(rxkb_layout_get_description(layout));
	});

	if (!result) {
		qCDebug(logKbLayout) << "didn't find layout for full name" << fullName;
		return;
	}

	Qt::beginPropertyUpdateGroup();
	this->bShortName = result->shortName;
	this->bVariant = result->variant;
	this->bShortDescription = result->shortDescription;
	this->bFullName = result->fullName;
	Qt::endPropertyUpdateGroup();
}

} // namespace qs::keyboard
