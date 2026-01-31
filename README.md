# QMLGreet | [![License](https://img.shields.io/badge/License-BSD_3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

QML-based greeter for greetd and wlr-based compositors.

![QMLGreet](https://nxos.org/wp-content/uploads/2026/01/screenshot-20260130-005834.png)
> QMLGreet, a QML-based greeter.

# Introduction

QMLGreet is a modern, lightweight greeter (login screen) designed for `greetd`. Built with **[MauiKit](https://mauikit.org/)** to deliver a polished, consistent user interface.

QMLGreet runs natively on Wayland compositors (such as Hyprland or Sway) using the Layer Shell protocol.

> [!WARNING]
> QMLGreet does not support X11. QMLGreet's main target is Nitrux OS, and using it in other distributions is not within its scope. Please do not open issues regarding this use case; they will be closed.

## Features

- Wayland-native: Integrates seamlessly with wlroots-based compositors via the `wlr-layer-shell-unstable-v1` protocol.
- Configurable: 
    * Customize the look and feel via `/etc/qmlgreet/qmlgreet.conf`.
    * Supports standard `.colors` schemes (KDE style).
    * Configurable font family and base font size.
    * Support for custom icon themes.
    * Set custom wallpapers with automatic blur effects.
- Session Management:
    * Automatic discovery of Wayland sessions from XDG data directories.
    * Filters out hidden sessions.
    * Direct D-Bus integration with `logind || elogind`.
    * Dynamically hides system actions unsupported by the host hardware.
    * Native battery monitor showing percentage and charging status (visible only when a battery is detected).
- Performance:
    * Optimized C++ backend.
    * Built with `x86-64-v3` optimizations for modern hardware.

### Runtime Requirements

```
mauikit (>= 4.0.2)
qt6 (>= 6.9.2)
qt6-wayland (>= 6.9.2)
greetd
wayland
```

# Licensing

The license for this repository and its contents is **BSD-3-Clause**.

# Issues

If you find problems with the contents of this repository, please create an issue and use the **🐞 Bug report** template.

## Submitting a bug report

Before submitting a bug, you should look at the [existing bug reports](https://github.com/Nitrux/qmlgreet/issues) to verify that no one has reported the bug already.

©2026 Nitrux Latinoamericana S.C.
