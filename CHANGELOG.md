# HIDeous Changelog

## v0.15.1 (2025-12-20)

- 🐛 Fixed a bug where the app was unable to map the equals key. Use `EQUALS` instead of `=`.

## v0.15.0 (2025-12-20)

- 🔄 **Auto-Reload Settings**: Changes to `settings.ini` or profile files are now detected and applied automatically. No more restarting the app!

## v0.14.1 (2025-12-15)

- ✨ Fixed handling of composite emojis (e.g. 🔴) in `text:` command.
- 🐛 Fixed `settings.ini` not loading correctly if it contained a BOM or different line endings.
- ⚡ Internal refactoring of `intercept.cpp` for better maintainability.

## v0.14.0 (2025-12-14)

- ⚡ Optimized the hook to only intercept keys that are actually mapped, reducing system overhead.
- 🐛 Fixed startup initialization issues by explicitly passing window handle to the hook.
- ✨ `text:` command now supports control keys (e.g. `{ENTER}`) and escaping.
- ⚡ Macro execution is now asynchronous, preventing key blocking on long macros.

## v0.13.1 (2025-02-07)

- 🐛 Fixed a bug where the app would not run due to a missing registry key.

## v0.13.0 (2025-02-07)

- 👤 Added profiles. Switch between different sets of key bindinds easily.
- ❓ Added a "Help" button to the UI.

## v0.12.1 (2025-02-02)

- 🐛 Fixed a bug where the app would not accept hex codes for the keys, e.g. "0x60".
- 🐛 Fixed a memory leak in the "run" command.

## v0.12.0 (2025-01-28)

- 📝 Edit "settings.ini" using a button in the GUI or by right clicking a tray icon.

## v0.11.0 (2025-01-24)

- 🏃‍♀️ Added the "run:" command allowing you to run anything on a keypress.

## v0.10.1 (2025-01-24)

- 🐛 Fixed a bug where the app would address UI components wrong.

## v0.10.0 (2025-01-24)

- 🚀 "Run on startup" checkbox added to the UI.

## v0.9.0 (2025-01-23)

- 🚀 Initial release!
- ⌨ A single key can be mapped to a combo or text.
- 🎨 UI shows a list of keyboards.
