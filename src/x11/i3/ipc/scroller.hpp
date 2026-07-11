#pragma once

#include <qcontainerfwd.h>
#include <qproperty.h>
#include <qtypes.h>

#include "connection.hpp"
#include "controller.hpp"

namespace qs::i3::ipc {

class I3Workspace;

///! Scroll scroller
class I3Scroller: public QObject {
	Q_OBJECT;
	// clang-format off
	/// The workspace for this scroller
	Q_PROPERTY(qs::i3::ipc::I3Workspace* workspace READ default NOTIFY workspaceChanged BINDABLE bindableWorkspace);
	/// Overview state
	Q_PROPERTY(bool overview READ default NOTIFY overviewChanged BINDABLE bindableOverview);
	/// Workspace scaled state
	Q_PROPERTY(bool scaled READ default NOTIFY scaledChanged BINDABLE bindableScaled);
	/// Scale for the workspace, -1.0 if not scaled
	Q_PROPERTY(double scale READ default NOTIFY scaleChanged BINDABLE bindableScale);
	/// Current mode: "horizontal" or "vertical"
	Q_PROPERTY(QString mode READ default WRITE setMode NOTIFY modeChanged BINDABLE bindableMode);
	/// Current insert mode: "after", "before", "beginning" or "end"
	Q_PROPERTY(QString insert READ default WRITE setInsert NOTIFY insertChanged BINDABLE bindableInsert);
	/// Focus new windows?
	Q_PROPERTY(bool focus READ default WRITE setFocus NOTIFY focusChanged BINDABLE bindableFocus);
	/// Center Horizontal?
	Q_PROPERTY(bool centerHorizontal READ default WRITE setCenterHorizontal NOTIFY centerHorizontalChanged BINDABLE bindableCenterHorizontal);
	/// Center Vertical?
	Q_PROPERTY(bool centerVertical READ default WRITE setCenterVertical NOTIFY centerVerticalChanged BINDABLE bindableCenterVertical);
	/// Reorder mode: "auto" or "lazy"
	Q_PROPERTY(QString reorder READ default WRITE setReorder NOTIFY reorderChanged BINDABLE bindableReorder);
	/// Last JSON returned for this scroller, as a JavaScript object.
	///
	/// This updates every time we receive a `scroller` event from i3/Sway/Scroll
	Q_PROPERTY(QVariantMap lastIpcObject READ lastIpcObject NOTIFY lastIpcObjectChanged);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("I3Scroller must be retrieved from the I3 object.");

public:
	I3Scroller(I3IpcController* ipc);

	[[nodiscard]] QBindable<I3Workspace*> bindableWorkspace() { return &this->bWorkspace; }
	[[nodiscard]] QBindable<bool> bindableOverview() { return &this->bOverview; }
	[[nodiscard]] QBindable<bool> bindableScaled() { return &this->bScaled; }
	[[nodiscard]] QBindable<double> bindableScale() { return &this->bScale; }
	[[nodiscard]] QBindable<QString> bindableMode() { return &this->bMode; }
	[[nodiscard]] QBindable<QString> bindableInsert() { return &this->bInsert; }
	[[nodiscard]] QBindable<bool> bindableFocus() { return &this->bFocus; }
	[[nodiscard]] QBindable<bool> bindableCenterHorizontal() { return &this->bCenterHorizontal; }
	[[nodiscard]] QBindable<bool> bindableCenterVertical() { return &this->bCenterVertical; }
	[[nodiscard]] QBindable<QString> bindableReorder() { return &this->bReorder; }
	[[nodiscard]] QVariantMap lastIpcObject() const;

	void updateFromObject(const QVariantMap& obj);

	void setMode(const QString &mode);
	void setInsert(const QString &insert);
	void setFocus(bool focus);
	void setCenterHorizontal(bool center);
	void setCenterVertical(bool center);
	void setReorder(const QString &reorder);

signals:
	void workspaceChanged();
	void overviewChanged();
	void scaledChanged();
	void scaleChanged();
	void modeChanged();
	void insertChanged();
	void focusChanged();
	void centerHorizontalChanged();
	void centerVerticalChanged();
	void reorderChanged();
	void lastIpcObjectChanged();

private:
	I3IpcController* ipc;

	QVariantMap mLastIpcObject;

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY(I3Scroller, I3Workspace*, bWorkspace, &I3Scroller::workspaceChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Scroller, bool, bOverview, &I3Scroller::overviewChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Scroller, bool, bScaled, &I3Scroller::scaledChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(I3Scroller, double, bScale, -1.0, &I3Scroller::scaleChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Scroller, QString, bMode, &I3Scroller::modeChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Scroller, QString, bInsert, &I3Scroller::insertChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Scroller, bool, bFocus, &I3Scroller::focusChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Scroller, bool, bCenterHorizontal, &I3Scroller::centerHorizontalChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Scroller, bool, bCenterVertical, &I3Scroller::centerVerticalChanged);
	Q_OBJECT_BINDABLE_PROPERTY(I3Scroller, QString, bReorder, &I3Scroller::reorderChanged);
	// clang-format on
};
} // namespace qs::i3::ipc
