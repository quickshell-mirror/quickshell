#pragma once

#include <limits>

#include <qobject.h>
#include <qproperty.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

class QJsonObject;

namespace qs::niri::ipc {

class NiriIpc;

///! Niri window/toplevel.
/// Represents a window as reported by niri.
class NiriWindow: public QObject {
	Q_OBJECT;
	// clang-format off
	Q_PROPERTY(qint64 id READ default NOTIFY idChanged BINDABLE bindableId);
	Q_PROPERTY(QString title READ default NOTIFY titleChanged BINDABLE bindableTitle);
	Q_PROPERTY(QString appId READ default NOTIFY appIdChanged BINDABLE bindableAppId);
	Q_PROPERTY(qint64 workspaceId READ default NOTIFY workspaceIdChanged BINDABLE bindableWorkspaceId);
	Q_PROPERTY(bool focused READ default NOTIFY focusedChanged BINDABLE bindableFocused);
	/// The output name this window is on, derived from its workspace assignment.
	Q_PROPERTY(QString output READ default NOTIFY outputChanged BINDABLE bindableOutput);
	/// X position in niri's scrolling layout. For floating windows, this is INT32_MAX.
	Q_PROPERTY(qint32 positionX READ default NOTIFY positionXChanged BINDABLE bindablePositionX);
	/// Y position in niri's scrolling layout. For floating windows, this is INT32_MAX.
	Q_PROPERTY(qint32 positionY READ default NOTIFY positionYChanged BINDABLE bindablePositionY);
	/// Whether this window is floating (not part of the scrolling layout).
	Q_PROPERTY(bool isFloating READ default NOTIFY isFloatingChanged BINDABLE bindableIsFloating);
	// clang-format on
	QML_ELEMENT;
	QML_UNCREATABLE("NiriWindows must be retrieved from the Niri object.");

public:
	static constexpr qint32 FLOATING_POSITION = std::numeric_limits<qint32>::max();

	explicit NiriWindow(QObject* parent): QObject(parent) {}

	void updateFromJson(const QJsonObject& object, const QString& outputName);
	void updateLayout(const QJsonObject& layout);
	void setFocused(bool focused);
	void setOutput(const QString& output);

	[[nodiscard]] QBindable<qint64> bindableId() { return &this->bId; }
	[[nodiscard]] QBindable<QString> bindableTitle() { return &this->bTitle; }
	[[nodiscard]] QBindable<QString> bindableAppId() { return &this->bAppId; }
	[[nodiscard]] QBindable<qint64> bindableWorkspaceId() { return &this->bWorkspaceId; }
	[[nodiscard]] QBindable<bool> bindableFocused() { return &this->bFocused; }
	[[nodiscard]] QBindable<QString> bindableOutput() { return &this->bOutput; }
	[[nodiscard]] QBindable<qint32> bindablePositionX() { return &this->bPositionX; }
	[[nodiscard]] QBindable<qint32> bindablePositionY() { return &this->bPositionY; }
	[[nodiscard]] QBindable<bool> bindableIsFloating() { return &this->bIsFloating; }

signals:
	void idChanged();
	void titleChanged();
	void appIdChanged();
	void workspaceIdChanged();
	void focusedChanged();
	void outputChanged();
	void positionXChanged();
	void positionYChanged();
	void isFloatingChanged();

private:
	void updatePositionFromLayout(const QJsonObject& layout);

	// clang-format off
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriWindow, qint64, bId, -1, &NiriWindow::idChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWindow, QString, bTitle, &NiriWindow::titleChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWindow, QString, bAppId, &NiriWindow::appIdChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriWindow, qint64, bWorkspaceId, -1, &NiriWindow::workspaceIdChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWindow, bool, bFocused, &NiriWindow::focusedChanged);
	Q_OBJECT_BINDABLE_PROPERTY(NiriWindow, QString, bOutput, &NiriWindow::outputChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriWindow, qint32, bPositionX, FLOATING_POSITION, &NiriWindow::positionXChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriWindow, qint32, bPositionY, FLOATING_POSITION, &NiriWindow::positionYChanged);
	Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(NiriWindow, bool, bIsFloating, true, &NiriWindow::isFloatingChanged);
	// clang-format on
};

} // namespace qs::niri::ipc
