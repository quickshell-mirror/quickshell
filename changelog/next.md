## Features

- Added shell translation catalogs in `i18n/qml_<language>.qm`, with runtime
  language switching through `Qt.uiLanguage`. Place compiled Qt Linguist catalogs
  beside the root QML file in `i18n/` and use `qsTr()` in translation bindings.
  An optional `i18n/qml.qm` catalog provides source-language translations, including
  plural forms, when the selected language or a translated message is unavailable.
  It also applies when `Qt.uiLanguage` is empty; without it, lookups return the source text.
  Reload the shell after updating catalogs.

## Other Changes

- Added support for Hyprland's new workspace address format.

## Bug Fixes

- Fixed main process crashes on pam subprocess misbehavior.
- Fixed crashes when attempting to create or modify session locks reentrantly.
- Fixed networking state breaking after restarting NetworkManager.
- Fixed accessibility information not being provided for Quickshell windows.
- Fixed `WlSessionLockSurface.screen` being null and never updating with a real screen on monitor plug.
- Fixed `NotificationAction.text` not accepting text updates.
