## Features

- Added shell translation catalogs in `i18n/qml_<language>.qm`, with runtime
  language switching through `Qt.uiLanguage`. Place compiled Qt Linguist catalogs
  beside the root QML file in `i18n/` and use `qsTr()` in translation bindings.
  Reload the shell after updating catalogs.

## Bug Fixes

- Fixed main process crashes on pam subprocess misbehavior.
- Fixed crashes when attempting to create or modify session locks reentrantly.
- Fixed networking state breaking after restarting NetworkManager.
- Fixed accessibility information not being provided for Quickshell windows.
- Fixed `WlSessionLockSurface.screen` being null and never updating with a real screen on monitor plug.
