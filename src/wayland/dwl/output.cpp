#include "output.hpp"
#include <cstdint>
#include <utility>

#include <QList>
#include <QObject>
#include <QString>
#include <QtTypes>

#include "tag.hpp"
#include "wayland-dwl-ipc-unstable-v2-client-protocol.h"

namespace qs::dwl {

DwlIpcOutput::DwlIpcOutput(::zdwl_ipc_output_v2* handle, QString name, QObject* parent)
    : QObject(parent)
    , QtWayland::zdwl_ipc_output_v2(handle)
    , mOutputName(std::move(name)) {}

DwlIpcOutput::~DwlIpcOutput() {
	if (this->isInitialized()) this->release();
}

bool DwlIpcOutput::active() const { return this->mActive; }
QString DwlIpcOutput::title() const { return this->mTitle; }
QString DwlIpcOutput::appId() const { return this->mAppId; }
quint32 DwlIpcOutput::layoutIndex() const { return this->mLayoutIndex; }
QString DwlIpcOutput::layoutSymbol() const { return this->mLayoutSymbol; }
bool DwlIpcOutput::fullscreen() const { return this->mFullscreen; }
bool DwlIpcOutput::floating() const { return this->mFloating; }
QList<DwlTag*> DwlIpcOutput::tags() const { return this->mTags; }
QString DwlIpcOutput::kbLayout() const { return this->mKbLayout; }
const QString& DwlIpcOutput::outputName() const { return this->mOutputName; }

void DwlIpcOutput::setTags(quint32 tagmask, quint32 toggleTagset) {
	this->set_tags(tagmask, toggleTagset);
}
void DwlIpcOutput::setClientTags(quint32 andTags, quint32 xorTags) {
	this->set_client_tags(andTags, xorTags);
}
void DwlIpcOutput::setLayout(quint32 index) { this->set_layout(index); }

void DwlIpcOutput::initTags(quint32 count) {
	for (DwlTag* t: this->mTags) t->deleteLater();
	this->mTags.clear();
	this->mTags.reserve(static_cast<qsizetype>(count));
	for (quint32 i = 0; i < count; ++i) this->mTags.append(new DwlTag(i, this));
	emit this->tagsChanged();
}

// Event overrides (double-buffered via frame)

void DwlIpcOutput::zdwl_ipc_output_v2_toggle_visibility() { emit this->toggleVisibility(); }

void DwlIpcOutput::zdwl_ipc_output_v2_active(uint32_t active) {
	this->mPending.hasActive = true;
	this->mPending.active = active != 0;
}

void DwlIpcOutput::zdwl_ipc_output_v2_tag(
    uint32_t tag,
    uint32_t state,
    uint32_t clients,
    uint32_t focused
) {
	if (std::cmp_less(tag, this->mTags.size()))
		this->mTags[static_cast<qsizetype>(tag)]->updateState(state, clients, focused);
}

void DwlIpcOutput::zdwl_ipc_output_v2_layout(uint32_t layout) {
	this->mPending.hasLayoutIndex = true;
	this->mPending.layoutIndex = layout;
}

void DwlIpcOutput::zdwl_ipc_output_v2_title(const QString& title) {
	this->mPending.hasTitle = true;
	this->mPending.title = title;
}

void DwlIpcOutput::zdwl_ipc_output_v2_appid(const QString& appid) {
	this->mPending.hasAppId = true;
	this->mPending.appId = appid;
}

void DwlIpcOutput::zdwl_ipc_output_v2_layout_symbol(const QString& layout) {
	this->mPending.hasLayoutSymbol = true;
	this->mPending.layoutSymbol = layout;
}

void DwlIpcOutput::zdwl_ipc_output_v2_fullscreen(uint32_t is_fullscreen) {
	this->mPending.hasFullscreen = true;
	this->mPending.fullscreen = is_fullscreen != 0;
}

void DwlIpcOutput::zdwl_ipc_output_v2_floating(uint32_t is_floating) {
	this->mPending.hasFloating = true;
	this->mPending.floating = is_floating != 0;
}

void DwlIpcOutput::zdwl_ipc_output_v2_kb_layout(const QString& kb_layout) {
	this->mPending.hasKbLayout = true;
	this->mPending.kbLayout = kb_layout;
}

void DwlIpcOutput::zdwl_ipc_output_v2_frame() {
	auto& p = this->mPending;

	if (p.hasActive && p.active != this->mActive) {
		this->mActive = p.active;
		emit this->activeChanged();
	}

	if (p.hasTitle && p.title != this->mTitle) {
		this->mTitle = p.title;
		emit this->titleChanged();
	}

	if (p.hasAppId && p.appId != this->mAppId) {
		this->mAppId = p.appId;
		emit this->appIdChanged();
	}

	if (p.hasLayoutIndex && p.layoutIndex != this->mLayoutIndex) {
		this->mLayoutIndex = p.layoutIndex;
		emit this->layoutIndexChanged();
	}

	if (p.hasLayoutSymbol && p.layoutSymbol != this->mLayoutSymbol) {
		this->mLayoutSymbol = p.layoutSymbol;
		emit this->layoutSymbolChanged();
	}

	if (p.hasFullscreen && p.fullscreen != this->mFullscreen) {
		this->mFullscreen = p.fullscreen;
		emit this->fullscreenChanged();
	}

	if (p.hasFloating && p.floating != this->mFloating) {
		this->mFloating = p.floating;
		emit this->floatingChanged();
	}

	if (p.hasKbLayout && p.kbLayout != this->mKbLayout) {
		this->mKbLayout = p.kbLayout;
		emit this->kbLayoutChanged();
	}

	p = {};
	emit this->frame();
}

} // namespace qs::dwl
