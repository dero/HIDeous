# HIDeous: Any keyboard is a macro keyboard

HID = [Human Interface Device](https://en.wikipedia.org/wiki/USB_human_interface_device_class)

Free Windows app that allows the keyboard lying in your drawer to be useful again.

- [Features](#features)
- [UI](#ui)
- [Usage](#usage)
- [Settings](#settings)
- [Key names](#key-names)
- [Similar Tools](#similar-tools)
- [Troubleshooting & Support](#troubleshooting--support)
- [Disclaimer](#disclaimer)
- [License](#license)

## Features

* **Key -> Combo**: Press a single key to send a combo like `Ctrl+C`.
* **Key -> Text**: Press a single key to send a text like `¯\_(ツ)_/¯`.
* **Any USB keyboard**: No matter how old or cheap.
* **Unicode support**: Emoji, math symbols, katakana? No problem.
* **No drivers required**: Just run the `.exe`.
* **Portable**: No installation required.
* **Minimal UI**: Just the list of keyboards connected to your computer.
* **Configuration in `.ini`**: Simple format, set and forget.

(Granted, the last two features can be viewed as _lack of features_ by the "glass half empty" folks, but I like to think of them as _simplicity_.)

## UI

![UI](./gui.png)

## Usage

0. Plug in a second (third, fourth, etc.) keyboard.
1. Download the latest release from [Releases](https://github.com/dero/HIDeous/releases) and extract it.
2. Run `HIDeous.exe`.
3. Press a key, a key code will appear in one of the rows.
4. Note down the "Device Hash" of that keyboard.
5. Edit `settings.ini` in the application directory.
6. **Restart the app** and you're done! 🎉

## Settings

All settings are stored in `settings.ini` in the same directory as the app.

This is my actual `settings.ini` that I use at home, I just added comments to explain each section.

```ini
; ❌ Don't touch Global settings, unless something is broken.
[Global]
; When `1`, it will create `debug.log` in this directory with debug info.
; ⚠ Can be a lot of data, keep this disabled when not needed.
Debug=0

; How long to give the app to register a key, in milliseconds.
; If the app keeps missing keys, try increasing this.
KeyWaitTime=30


; ✅ Edit this section to name your keyboards.
; Left side = Any name you want to give your keyboard.
; Right side = Its Device Hash, find it in the UI.
[Devices]
Main_Keyboard=9CC297
NumPad=1A5553

; ✅ Edit this section to set bindings for a specific keyboard.
; This section configures my main keyboard.
;
; `text:` prefix will send a text.
; `keys:` prefix will send a key combination.
;
; ⚠ Never include a comment in the same line as a key binding. ⚠
[Main_Keyboard]
MULTIPLY=text:×

; ✅ The same, but for my external numpad.
[NumPad]
Numpad0=keys:WIN+F10
Numpad1=keys:WIN+F1
Numpad2=keys:WIN+F2
Numpad3=keys:WIN+F3
DECIMAL=keys:WIN+F11
ENTER=keys:WIN+F12
Numpad7=keys:CTRL+SHIFT+F9
Numpad8=keys:CTRL+SHIFT+F10
Numpad9=keys:CTRL+SHIFT+F11
ADD=keys:CTRL+SHIFT+F12
MULTIPLY=text:×

; That's it! Save the file and restart the app.
```

## Key Names

You can mostly just use the intuitive name of the key, like `A`, `F1`, `SHIFT`, etc. To find out key codes for more obscure keys, you can use the UI. Press a key and its code will appear in one of the rows.

And if that's not enough, you can find the [long list of key codes baked into the app in the source code](https://github.com/dero/HIDeous/blob/a438428b1621a1244db15162217f867ebe90bb40/src/common/settings.cpp#L19-L131).

## Similar Tools

How's this different from AutoHotkey or HIDMacros?

It can tell apart individual keyboards. [AHK](https://www.autohotkey.com/) can't easily do that.

It can print Unicode strings, which is something [HIDMacros](https://www.hidmacros.eu/) struggles with.

Then there's [LuaMacros](https://www.hidmacros.eu/download.php), which is a great tool, but it can be a bit hard to wrap your head around.

And it's newer, lighter and simpler than all of them. ⚖

## Troubleshooting & Support

Try running the app as an administrator.

If that doesn't help, it'd be great if you took the time to [create an issue on GitHub](https://github.com/dero/HIDeous/issues). I'll do my best to help you out and you'll be helping make the app better for everyone.

## Uninstall

Make sure the "Run on startup" toggle in the UI is off, then just delete the app directory.

## Disclaimer

This is essentially a keylogger. It doesn't do any shenanigans with your data, but thanks to access to your system, it could. Same as any other software you download from the internet.

If you can read and compile C++, you can check the source code and produce the binaries yourself. If you can't, either trust me or don't use this.

Also, I'm actually a web engineer, not a C++ developer by trade, so the code might be a bit rough around the edges. Constructive criticism is welcome. 🙇‍♂️

## License

Do whatever you want with this. [MIT License](./LICENSE).

Attribution and nice words are appreciated, but not required. 🙂
