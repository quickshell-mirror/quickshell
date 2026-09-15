#include "keyboard.hpp"
#include <algorithm>

#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qproperty.h>
#include <qstring.h>
#include <qtypes.h>
#include <qvariant.h>

#include "../../../core/logcat.hpp"
#include "../../../core/model.hpp"
#include "../../../keyboard/keyboard_layout.hpp"
#include "connection.hpp"

namespace qs::hyprland::ipc {

namespace {
QS_LOGGING_CATEGORY(logHyprlandKb, "quickshell.hyprland.ipc.keyboard", QtWarningMsg);
} // namespace

ObjectModel<qs::keyboard::KeyboardLayout>* HyprlandKeyboard::availableLayouts() {
	return &this->mAvailableLayouts;
}

void HyprlandKeyboard::updateFromDeviceObject(const QVariantMap& object) {
	auto name = object.value("name").toString();
	auto layouts = object.value("layout").toString().split(','); // short names
	auto activeLayoutIndex = object.value("active_layout_index").toInt();

	Qt::beginPropertyUpdateGroup();
	this->bName = name;

	if (!layouts.isEmpty()) {
		auto& existing = this->mAvailableLayouts.valueList();

		for (qsizetype i = 0; i < layouts.size(); ++i) {
			auto kmName = layouts.at(i);

			auto iter = std::ranges::find_if(existing, [&](qs::keyboard::KeyboardLayout* lay) {
				return lay->bindableShortName().value() == kmName;
			});

			qs::keyboard::KeyboardLayout* layout = nullptr;
			if (iter != existing.end()) {
				layout = *iter;
			} else {
				layout = new qs::keyboard::KeyboardLayout(this);
				layout->resolveFromShortName(kmName);
				this->mAvailableLayouts.insertObject(layout, i);
			}

			if (i == activeLayoutIndex) {
				this->bActiveLayout = layout;
			}
		}

		this->bActiveLayoutIndex = activeLayoutIndex;

		if (this->bActiveLayout.value() == nullptr && !layouts.empty()) {
			this->bActiveLayout = this->mAvailableLayouts.valueList().first();
		}
	}

	Qt::endPropertyUpdateGroup();
}

void HyprlandKeyboard::updateActiveLayout(const QString& activeLayoutName) {
	const auto& layouts = this->mAvailableLayouts.valueList();

	for (qint32 i = 0; i < layouts.size(); ++i) {
		if (layouts.at(i)->bindableFullName().value() == activeLayoutName) {
			Qt::beginPropertyUpdateGroup();
			this->bActiveLayout = layouts.at(i);
			this->bActiveLayoutIndex = i;
			Qt::endPropertyUpdateGroup();
			return;
		}
	}

	qCDebug(logHyprlandKb) << "Layout" << activeLayoutName << "not found in available layouts";

	// Layout not in the list - create a transient one.
	auto* layout = new qs::keyboard::KeyboardLayout(this);
	layout->resolveFromFullName(activeLayoutName);
	auto idx = this->mAvailableLayouts.valueList().size();
	this->mAvailableLayouts.insertObject(layout, idx);

	Qt::beginPropertyUpdateGroup();
	this->bActiveLayout = layout;
	this->bActiveLayoutIndex = static_cast<qint32>(idx);
	Qt::endPropertyUpdateGroup();
}

void HyprlandKeyboard::cycleLayout() {
	auto count = this->mAvailableLayouts.valueList().size();
	if (count == 0) return;

	auto next = (this->bActiveLayoutIndex.value() + 1) % static_cast<qint32>(count);
	this->switchToLayout(next);
}

void HyprlandKeyboard::switchToLayout(qint32 index) {
	this->ipc->dispatch(QString("switchxkblayout %1 %2").arg(this->bName.value()).arg(index), false);
}

void HyprlandKeyboard::onActiveLayoutDestroyed() { this->bActiveLayout = nullptr; }

} // namespace qs::hyprland::ipc
