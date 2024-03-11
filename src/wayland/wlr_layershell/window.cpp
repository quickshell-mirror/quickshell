#include "window.hpp"
#include <utility>

#include <private/qwaylandwindow_p.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <qwindow.h>

#include "../../core/panelinterface.hpp"
#include "shell_integration.hpp"
#include "surface.hpp"

LayershellWindowExtension* LayershellWindowExtension::get(QWindow* window) {
	auto v = window->property("layershell_ext");

	if (v.canConvert<LayershellWindowExtension*>()) {
		return v.value<LayershellWindowExtension*>();
	} else {
		return nullptr;
	}
}

bool LayershellWindowExtension::attach(QWindow* window) {
	if (this->surface != nullptr)
		throw "Cannot change the attached window of a LayershellWindowExtension";

	auto* current = LayershellWindowExtension::get(window);

	bool hasSurface = false;

	if (current != nullptr) {
		if (current->mNamespace != this->mNamespace) return false;

		if (current->surface != nullptr) {
			if (current->surface->qwindow()->screen() != window->screen()) return false;
			this->surface = current->surface;

			// update window with current settings, leveraging old extension's cached values
			current->setAnchors(this->mAnchors);
			current->setMargins(this->mMargins);
			current->setExclusiveZone(this->mExclusiveZone);
			current->setLayer(this->mLayer);
			current->setKeyboardFocus(this->mKeyboardFocus);

			this->surface->ext = this;
			current->surface = nullptr;
			current->deleteLater();

			hasSurface = true;
		}
	}

	if (!hasSurface) {
		// Qt appears to be resetting the window's screen on creation on some systems. This works around it.
		auto* screen = window->screen();
		window->create();
		window->setScreen(screen);

		auto* waylandWindow = dynamic_cast<QtWaylandClient::QWaylandWindow*>(window->handle());
		if (waylandWindow == nullptr) {
			qWarning() << window << "is not a wayland window. Cannot create layershell surface.";
			return false;
		}

		static QSWaylandLayerShellIntegration* layershellIntegration = nullptr; // NOLINT
		if (layershellIntegration == nullptr) {
			layershellIntegration = new QSWaylandLayerShellIntegration();
			if (!layershellIntegration->initialize(waylandWindow->display())) {
				delete layershellIntegration;
				layershellIntegration = nullptr;
				qWarning() << "Failed to initialize layershell integration";
			}
		}

		waylandWindow->setShellIntegration(layershellIntegration);
	}

	this->setParent(window);
	window->setProperty("layershell_ext", QVariant::fromValue(this));
	return true;
}

void LayershellWindowExtension::setAnchors(Anchors anchors) {
	if (anchors != this->mAnchors) {
		this->mAnchors = anchors;
		if (this->surface != nullptr) this->surface->updateAnchors();
		emit this->anchorsChanged();
	}
}

Anchors LayershellWindowExtension::anchors() const { return this->mAnchors; }

void LayershellWindowExtension::setMargins(Margins margins) {
	if (margins != this->mMargins) {
		this->mMargins = margins;
		if (this->surface != nullptr) this->surface->updateMargins();
		emit this->marginsChanged();
	}
}

Margins LayershellWindowExtension::margins() const { return this->mMargins; }

void LayershellWindowExtension::setExclusiveZone(qint32 exclusiveZone) {
	if (exclusiveZone != this->mExclusiveZone) {
		this->mExclusiveZone = exclusiveZone;
		if (this->surface != nullptr) this->surface->updateExclusiveZone();
		emit this->exclusiveZoneChanged();
	}
}

qint32 LayershellWindowExtension::exclusiveZone() const { return this->mExclusiveZone; }

void LayershellWindowExtension::setLayer(WlrLayer::Enum layer) {
	if (layer != this->mLayer) {
		this->mLayer = layer;
		if (this->surface != nullptr) this->surface->updateLayer();
		emit this->layerChanged();
	}
}

WlrLayer::Enum LayershellWindowExtension::layer() const { return this->mLayer; }

void LayershellWindowExtension::setKeyboardFocus(WlrKeyboardFocus::Enum focus) {
	if (focus != this->mKeyboardFocus) {
		this->mKeyboardFocus = focus;
		if (this->surface != nullptr) this->surface->updateKeyboardFocus();
		emit this->keyboardFocusChanged();
	}
}

WlrKeyboardFocus::Enum LayershellWindowExtension::keyboardFocus() const {
	return this->mKeyboardFocus;
}

void LayershellWindowExtension::setUseWindowScreen(bool value) {
	this->useWindowScreen = value; // has no effect post configure
}

void LayershellWindowExtension::setNamespace(QString ns) {
	if (!this->isConfigured()) this->mNamespace = std::move(ns);
}

QString LayershellWindowExtension::ns() const { return this->mNamespace; }

bool LayershellWindowExtension::isConfigured() const { return this->surface != nullptr; }
