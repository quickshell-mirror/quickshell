#pragma once

// hide a property, function, or signal from typegen
#define QSDOC_HIDE

// override the base class as seen by typegen
#define QSDOC_BASECLASS(baseclass)

// make the type visible in the docs even if not a QML_ELEMENT
#define QSDOC_ELEMENT
#define QSDOC_NAMED_ELEMENT(name)

// overridden properties
#define QSDOC_PROPERTY_OVERRIDE(...)
