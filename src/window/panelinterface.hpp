#pragma once

#include <qnamespace.h>
#include <qqmlintegration.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "../core/doc.hpp"
#include "../core/types.hpp"
#include "windowinterface.hpp"

class Anchors {
	Q_GADGET;
	Q_PROPERTY(bool left MEMBER mLeft);
	Q_PROPERTY(bool right MEMBER mRight);
	Q_PROPERTY(bool top MEMBER mTop);
	Q_PROPERTY(bool bottom MEMBER mBottom);
	QML_VALUE_TYPE(panelAnchors);
	QML_STRUCTURED_VALUE;

public:
	[[nodiscard]] bool horizontalConstraint() const noexcept { return this->mLeft && this->mRight; }
	[[nodiscard]] bool verticalConstraint() const noexcept { return this->mTop && this->mBottom; }

	[[nodiscard]] Qt::Edge exclusionEdge() const noexcept {
		auto hasHEdge = this->mLeft ^ this->mRight;
		auto hasVEdge = this->mTop ^ this->mBottom;

		if (hasVEdge && !hasHEdge) {
			if (this->mTop) return Qt::TopEdge;
			if (this->mBottom) return Qt::BottomEdge;
		} else if (hasHEdge && !hasVEdge) {
			if (this->mLeft) return Qt::LeftEdge;
			if (this->mRight) return Qt::RightEdge;
		}

		return static_cast<Qt::Edge>(0);
	}

	[[nodiscard]] bool operator==(const Anchors& other) const noexcept {
		// clang-format off
		return this->mLeft == other.mLeft
			&& this->mRight == other.mRight
			&& this->mTop == other.mTop
			&& this->mBottom == other.mBottom;
		// clang-format on
	}

	bool mLeft = false;
	bool mRight = false;
	bool mTop = false;
	bool mBottom = false;
};

///! Panel exclusion mode
/// See @@PanelWindow.exclusionMode.
namespace ExclusionMode { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum : quint8 {
	/// Respect the exclusion zone of other shell layers and optionally set one
	Normal = 0,
	/// Ignore exclusion zones of other shell layers. You cannot set an exclusion zone in this mode.
	Ignore = 1,
	/// Decide the exclusion zone based on the window dimensions and anchors.
	///
	/// Will attempt to reseve exactly enough space for the window and its margins if
	/// exactly 3 anchors are connected.
	Auto = 2,
};
Q_ENUM_NS(Enum);

} // namespace ExclusionMode

///! Decorationless window attached to screen edges by anchors.
/// Decorationless window attached to screen edges by anchors.
///
/// #### Example
/// The following snippet creates a white bar attached to the bottom of the screen.
///
/// ```qml
/// PanelWindow {
///   anchors {
///     left: true
///     bottom: true
///     right: true
///   }
///
///   Text {
///     anchors.centerIn: parent
///     text: "Hello!"
///   }
/// }
/// ```
class PanelWindowInterface: public WindowInterface {
	// clang-format off
	Q_OBJECT;
	/// Anchors attach a shell window to the sides of the screen.
	/// By default all anchors are disabled to avoid blocking the entire screen due to a misconfiguration.
	///
	/// > [!INFO] When two opposite anchors are attached at the same time, the corrosponding dimension
	/// > (width or height) will be forced to equal the screen width/height.
	/// > Margins can be used to create anchored windows that are also disconnected from the monitor sides.
	Q_PROPERTY(Anchors anchors READ anchors WRITE setAnchors NOTIFY anchorsChanged);
	/// Offsets from the sides of the screen.
	///
	/// > [!INFO] Only applies to edges with anchors
	Q_PROPERTY(Margins margins READ margins WRITE setMargins NOTIFY marginsChanged);
	/// The amount of space reserved for the shell layer relative to its anchors.
	/// Setting this property sets @@exclusionMode to `ExclusionMode.Normal`.
	///
	/// > [!INFO] Either 1 or 3 anchors are required for the zone to take effect.
	Q_PROPERTY(qint32 exclusiveZone READ exclusiveZone WRITE setExclusiveZone NOTIFY exclusiveZoneChanged);
	/// Defaults to `ExclusionMode.Auto`.
	Q_PROPERTY(ExclusionMode::Enum exclusionMode READ exclusionMode WRITE setExclusionMode NOTIFY exclusionModeChanged);
	/// If the panel should render above standard windows. Defaults to true.
	///
	/// Note: On Wayland this property corrosponds to @@Quickshell.Wayland.WlrLayershell.layer.
	Q_PROPERTY(bool aboveWindows READ aboveWindows WRITE setAboveWindows NOTIFY aboveWindowsChanged);
	/// If the panel should accept keyboard focus. Defaults to false.
	///
	/// Note: On Wayland this property corrosponds to @@Quickshell.Wayland.WlrLayershell.keyboardFocus.
	Q_PROPERTY(bool focusable READ focusable WRITE setFocusable NOTIFY focusableChanged);
	// clang-format on
	QML_NAMED_ELEMENT(PanelWindow);
	QML_UNCREATABLE("No PanelWindow backend loaded.");
	QSDOC_CREATABLE;

public:
	explicit PanelWindowInterface(QObject* parent = nullptr): WindowInterface(parent) {}

	[[nodiscard]] virtual Anchors anchors() const = 0;
	virtual void setAnchors(Anchors anchors) = 0;

	[[nodiscard]] virtual Margins margins() const = 0;
	virtual void setMargins(Margins margins) = 0;

	[[nodiscard]] virtual qint32 exclusiveZone() const = 0;
	virtual void setExclusiveZone(qint32 exclusiveZone) = 0;

	[[nodiscard]] virtual ExclusionMode::Enum exclusionMode() const = 0;
	virtual void setExclusionMode(ExclusionMode::Enum exclusionMode) = 0;

	[[nodiscard]] virtual bool aboveWindows() const = 0;
	virtual void setAboveWindows(bool aboveWindows) = 0;

	[[nodiscard]] virtual bool focusable() const = 0;
	virtual void setFocusable(bool focusable) = 0;

signals:
	void anchorsChanged();
	void marginsChanged();
	void exclusiveZoneChanged();
	void exclusionModeChanged();
	void aboveWindowsChanged();
	void focusableChanged();
};
