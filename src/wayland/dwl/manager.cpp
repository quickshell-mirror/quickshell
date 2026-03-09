#include "manager.hpp"
#include <cstdint>

#include <qapplication.h>
#include <qbytearray.h>
#include <qcontainerfwd.h>
#include <qguiapplication.h>
#include <qlist.h>
#include <qlogging.h>
#include <qloggingcategory.h>
#include <qobject.h>
#include <private/qwaylandscreen_p.h>
#include <qscreen.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qwaylandclientextension.h>
#include <wayland-client-protocol.h>

#include "../../core/logcat.hpp"
#include "output.hpp"

namespace {
QS_LOGGING_CATEGORY(logDwlIpc, "quickshell.dwl", QtWarningMsg);
}

namespace qs::dwl {

DwlIpcManager::DwlIpcManager(): QWaylandClientExtensionTemplate(2) {
	QObject::connect(this, &QWaylandClientExtension::activeChanged, this, [this]() {
		if (!this->isActive()) {
			qCWarning(logDwlIpc) << "DWL is not available";
			return;
		}

		const auto screens = QGuiApplication::screens();
		for (QScreen* screen: screens) this->onScreenAdded(screen);

		QObject::connect(qApp, &QGuiApplication::screenAdded, this, &DwlIpcManager::onScreenAdded);
		QObject::connect(qApp, &QGuiApplication::screenRemoved, this, &DwlIpcManager::onScreenRemoved);
	});
}

DwlIpcManager* DwlIpcManager::instance() {
	static auto* instance = new DwlIpcManager();
	return instance;
}

quint32 DwlIpcManager::tagCount() const { return this->mTagCount; }
QStringList DwlIpcManager::layouts() const { return this->mLayouts; }
QList<DwlIpcOutput*> DwlIpcManager::outputs() const { return this->mOutputs; }

void DwlIpcManager::onScreenAdded(QScreen* screen) {
	auto* waylandScreen = dynamic_cast<QtWaylandClient::QWaylandScreen*>(screen->handle());
	if (!waylandScreen) return;
	this->bindOutput(waylandScreen->output(), screen->name());
}

void DwlIpcManager::onScreenRemoved(QScreen* screen) {
	auto* waylandScreen = dynamic_cast<QtWaylandClient::QWaylandScreen*>(screen->handle());
	if (!waylandScreen) return;
	this->removeOutput(waylandScreen->output());
}

DwlIpcOutput* DwlIpcManager::bindOutput(struct wl_output* wlOutput, const QString& name) {
	if (auto* existing = this->mOutputMap.value(wlOutput, nullptr)) return existing;

	auto* output = new DwlIpcOutput(this->get_output(wlOutput), name, this);
	output->initTags(this->mTagCount);
	this->mOutputs.append(output);
	this->mOutputMap.insert(wlOutput, output);
	emit this->outputAdded(output);

	return output;
}

void DwlIpcManager::removeOutput(struct wl_output* wlOutput) {
	auto* output = this->mOutputMap.take(wlOutput);
	if (!output) return;

	this->mOutputs.removeOne(output);
	emit this->outputRemoved(output);
	output->deleteLater();
}

void DwlIpcManager::zdwl_ipc_manager_v2_tags(uint32_t amount) {
	if (amount == this->mTagCount) return;
	this->mTagCount = amount;
	for (DwlIpcOutput* o: this->mOutputs) o->initTags(amount);
	emit this->tagCountChanged();
}

void DwlIpcManager::zdwl_ipc_manager_v2_layout(const QString& name) {
	this->mLayouts.append(name);
	emit this->layoutsChanged();
}

} // namespace qs::dwl
