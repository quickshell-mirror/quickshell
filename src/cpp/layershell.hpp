#pragma once

#include <LayerShellQt/window.h>
#include <qobject.h>
#include <qqmlintegration.h>
#include <qqmllist.h>
#include <qquickwindow.h>
#include <qscreen.h>
#include <qtmetamacros.h>
#include <qtypes.h>
#include <qvariant.h>
#include <qwindow.h>

#include "proxywindow.hpp"
#include "qmlscreen.hpp"

class Anchors {
	Q_GADGET;
	Q_PROPERTY(bool left MEMBER mLeft);
	Q_PROPERTY(bool right MEMBER mRight);
	Q_PROPERTY(bool top MEMBER mTop);
	Q_PROPERTY(bool bottom MEMBER mBottom);

public:
	bool mLeft = false;
	bool mRight = false;
	bool mTop = false;
	bool mBottom = false;
};

class Margins {
	Q_GADGET;
	Q_PROPERTY(qint32 left MEMBER mLeft);
	Q_PROPERTY(qint32 right MEMBER mRight);
	Q_PROPERTY(qint32 top MEMBER mTop);
	Q_PROPERTY(qint32 bottom MEMBER mBottom);

public:
	qint32 mLeft = 0;
	qint32 mRight = 0;
	qint32 mTop = 0;
	qint32 mBottom = 0;
};

namespace Layer { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	Background = 0,
	Bottom = 1,
	Top = 2,
	Overlay = 3,
};
Q_ENUM_NS(Enum);

} // namespace Layer

namespace KeyboardFocus { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	None = 0,
	Exclusive = 1,
	OnDemand = 2,
};
Q_ENUM_NS(Enum);

} // namespace KeyboardFocus

namespace ScreenConfiguration { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
	Window = 0,
	Compositor = 1,
};
Q_ENUM_NS(Enum);

} // namespace ScreenConfiguration

class ProxyShellWindow: public ProxyWindowBase {
	// clang-format off
	Q_OBJECT;
	Q_PROPERTY(QtShellScreenInfo* screen READ screen WRITE setScreen NOTIFY screenChanged);
	Q_PROPERTY(Anchors anchors READ anchors WRITE setAnchors NOTIFY anchorsChanged);
	Q_PROPERTY(qint32 exclusionZone READ exclusiveZone WRITE setExclusiveZone NOTIFY exclusionZoneChanged)
	Q_PROPERTY(Margins margins READ margins WRITE setMargins NOTIFY marginsChanged)
	Q_PROPERTY(Layer::Enum layer READ layer WRITE setLayer NOTIFY layerChanged)
	Q_PROPERTY(QString scope READ scope WRITE setScope)
	Q_PROPERTY(KeyboardFocus::Enum keyboardFocus READ keyboardFocus WRITE setKeyboardFocus NOTIFY keyboardFocusChanged)
	Q_PROPERTY(ScreenConfiguration::Enum screenConfiguration READ screenConfiguration WRITE setScreenConfiguration)
	Q_PROPERTY(bool closeOnDismissed READ closeOnDismissed WRITE setCloseOnDismissed);
	Q_CLASSINFO("DefaultProperty", "data");
	QML_ELEMENT;
	// clang-format on

protected:
	void earlyInit(QObject* old) override;

public:
	void componentComplete() override;
	QQuickWindow* disownWindow() override;

	QQmlListProperty<QObject> data();

	void setVisible(bool visible) override;
	bool isVisible() override;

	void setWidth(qint32 width) override;
	qint32 width() override;

	void setHeight(qint32 height) override;
	qint32 height() override;

	void setScreen(QtShellScreenInfo* screen);
	[[nodiscard]] QtShellScreenInfo* screen() const;

	void setAnchors(Anchors anchors);
	[[nodiscard]] Anchors anchors() const;

	void setExclusiveZone(qint32 zone);
	[[nodiscard]] qint32 exclusiveZone() const;

	void setMargins(Margins margins);
	[[nodiscard]] Margins margins() const;

	void setLayer(Layer::Enum layer);
	[[nodiscard]] Layer::Enum layer() const;

	void setScope(const QString& scope);
	[[nodiscard]] QString scope() const;

	void setKeyboardFocus(KeyboardFocus::Enum focus);
	[[nodiscard]] KeyboardFocus::Enum keyboardFocus() const;

	void setScreenConfiguration(ScreenConfiguration::Enum configuration);
	[[nodiscard]] ScreenConfiguration::Enum screenConfiguration() const;

	void setCloseOnDismissed(bool close);
	[[nodiscard]] bool closeOnDismissed() const;

signals:
	void screenChanged();
	void anchorsChanged();
	void marginsChanged();
	void exclusionZoneChanged();
	void layerChanged();
	void keyboardFocusChanged();

private:
	LayerShellQt::Window* shellWindow = nullptr;
	bool anchorsInitialized = false;

	// needed to ensure size dosent fuck up when changing layershell attachments
	// along with setWidth and setHeight overrides
	qint32 requestedWidth = 100;
	qint32 requestedHeight = 100;

	// width/height must be set before anchors, so we have to track anchors and apply them late
	bool complete = false;
	bool stagingVisible = false;
	Anchors stagingAnchors;
};
