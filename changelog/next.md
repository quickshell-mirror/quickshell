## Features

- Added shell translation catalogs in `i18n/qml_<language>.qm`, with runtime
  language switching through `Qt.uiLanguage`. Place compiled Qt Linguist catalogs
  beside the root QML file in `i18n/` and use `qsTr()` in translation bindings.
  An optional `i18n/qml.qm` catalog provides source-language translations, including
  plural forms, when the selected language or a translated message is unavailable.
  It also applies when `Qt.uiLanguage` is empty; without it, lookups return the source text.
  Reload the shell after updating catalogs.

## Other Changes

- `SystemTrayItem.activate()` now forwards Wayland activation tokens to applications.

## Bug Fixes

- Fixed ScriptModel not sending role updates on changed data that compares equal by key or structure.
- Fixed Hyprland workspaces keeping their old id after Hyprland changes it.
- Fixed desktop entries being parsed incorrectly with trailing whitespace or CRLF line endings.
- Fixed `WlSessionLock.locked` changes not being emitted on unlock.
- Fixed a crash when unplugging a screen being captured by ScreencopyView or trying to capture the fallback screen.
- Fixed most broken qml types in qmlls/qmllint for Qt 6.12.
- Fixed missing thread entry for Pipewire event loop resulting in Pipewire connection failing.
- Fixed Greetd treating responses for a cancelled session as responses for the next one.
