#pragma once

#include <qtmetamacros.h>

#include "doc.hpp"
#include "windowinterface.hpp"

class Anchors {
	Q_GADGET;
	Q_PROPERTY(bool left MEMBER mLeft);
	Q_PROPERTY(bool right MEMBER mRight);
	Q_PROPERTY(bool top MEMBER mTop);
	Q_PROPERTY(bool bottom MEMBER mBottom);

public:
	[[nodiscard]] bool horizontalConstraint() const noexcept { return this->mLeft && this->mRight; }
	[[nodiscard]] bool verticalConstraint() const noexcept { return this->mTop && this->mBottom; }

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

class Margins {
	Q_GADGET;
	Q_PROPERTY(qint32 left MEMBER mLeft);
	Q_PROPERTY(qint32 right MEMBER mRight);
	Q_PROPERTY(qint32 top MEMBER mTop);
	Q_PROPERTY(qint32 bottom MEMBER mBottom);

public:
	[[nodiscard]] bool operator==(const Margins& other) const noexcept {
		// clang-format off
		return this->mLeft == other.mLeft
			&& this->mRight == other.mRight
			&& this->mTop == other.mTop
			&& this->mBottom == other.mBottom;
		// clang-format on
	}

	qint32 mLeft = 0;
	qint32 mRight = 0;
	qint32 mTop = 0;
	qint32 mBottom = 0;
};

namespace ExclusionMode { // NOLINT
Q_NAMESPACE;
QML_ELEMENT;

enum Enum {
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
///     anchors.horizontalCenter: parent.horizontalCenter
///     anchors.verticalCenter: parent.verticalCenter
///     text: "Hello!"
///   }
/// }
/// ```
class PanelWindowInterface: public WindowInterface {
	QSDOC_NAMED_ELEMENT(PanelWindow);
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
	/// Setting this property sets `exclusionMode` to `Normal`.
	///
	/// > [!INFO] Either 1 or 3 anchors are required for the zone to take effect.
	Q_PROPERTY(qint32 exclusiveZone READ exclusiveZone WRITE setExclusiveZone NOTIFY exclusiveZoneChanged);
	/// Defaults to `ExclusionMode.Auto`.
	Q_PROPERTY(ExclusionMode::Enum exclusionMode READ exclusionMode WRITE setExclusionMode NOTIFY exclusionModeChanged);
	// clang-format on

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

signals:
	void anchorsChanged();
	void marginsChanged();
	void exclusiveZoneChanged();
	void exclusionModeChanged();
};
