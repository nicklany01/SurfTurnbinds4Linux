# SurfTurnbinds4Linux

A lightweight Linux utility with a Qt GUI that emulates continuous mouse movement for surfing in Counter-Strike and other Source engine games.

---

## How It Works

1. **Virtual Mouse Emulation**: The app creates a virtual mouse driver using Linux kernel `/dev/uinput`.
2. **Exact Yaw Calculation**: You specify your desired turning speed directly in degrees per second (°/s). Using your in-game sensitivity and the standard Source yaw factor (`m_yaw = 0.022`), the app calculates the exact relative mouse movement required at 125 Hz.
3. **Global Input Listener**: It listens for your configured turn keys or mouse buttons across all connected input devices, sending smooth relative mouse movement while held.
4. **Smart Window Pause**: Turnbinds automatically pause whenever the configuration window is active or under the mouse, so clicking options with Mouse 1/2 works normally without unwanted camera drift.

---

## Features

- **Yawspeed Profiles**: Configure up to 5 speed slots with sliders and hotkeys to switch speeds on the fly.
- **Speed Cycling**: Set a single keybind to cycle sequentially through your yawspeed profiles.
- **Master Toggle**: Keybind to pause and unpause turnbinds globally.
- **Conflict Protection**: Alerts you if a key is assigned twice and unbinds the old action automatically.
- **Persistent Config**: Settings are automatically saved to `~/.config/surfturnbinds/config.ini`.

---

## Requirements & Permissions

Because this tool listens to raw input devices and creates a virtual mouse, it requires read/write access to `/dev/uinput` and `/dev/input/event*`.

To run without `sudo`, ensure your user belongs to the `input` group and has write permissions to `/dev/uinput` (via udev rules), or run the executable with appropriate permissions:

```bash
sudo setfacl -m u:$USER:rw /dev/uinput
sudo setfacl -m u:$USER:r /dev/input/event*
```

---

## Building & Running

### Requirements
- C++17 compiler (`g++` or `clang++`)
- Qt 6 (`Qt6Widgets`, `Qt6Gui`, `Qt6Core`)
- `pkg-config` / `pkgconf`

### Build
```bash
make
```

### Run
```bash
./surfturnbinds
```
