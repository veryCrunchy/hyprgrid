# Hypr Grid

A modern window grid manager for Hyprland that allows for precise window positioning using a configurable grid system.

## Features

- Configurable grid system (rows, columns, gaps)
- Save and load window positions
- Keyboard shortcuts for quick window positioning
- Visual grid editor for creating custom layouts
- Rule-based window positioning
- Multiple monitor support

## Installation

### Dependencies

- Qt6 (Core, GUI, Widgets, Network)
- CMake (3.16+)
- GCC with C++17 support

### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/username/hyprgrid.git
cd hyprgrid
```

2. Build the application:
```bash
./build.sh
```

3. Install the application:
```bash
./build.sh --install
```

## Usage

### Command Line Interface

```
hypr-grid-manager [options]
```

Options:
- `-a, --apply <preset:position>`: Apply a window position from a preset
- `-r, --reset`: Reset window state and clear rules
- `-c, --config`: Print current configuration
- `-u, --ui`: Show the configuration UI

### Examples

Apply a position:
```bash
hypr-grid-manager -a default:top-left
```

Reset window state:
```bash
hypr-grid-manager -r
```

Open the UI:
```bash
hypr-grid-manager -u
```

### Integration with Hyprland

Add these bindings to your Hyprland configuration:

```
# Grid manager UI
bind = Super+Control, Return, exec, hypr-grid-manager -u

# Apply grid positions
bind = Super, KP_7, exec, hypr-grid-manager -a default:top-left
bind = Super, KP_8, exec, hypr-grid-manager -a default:top
bind = Super, KP_9, exec, hypr-grid-manager -a default:top-right
# ...more bindings...
```

## Configuration

The configuration file is stored at `~/.config/hypr/qt-grid-manager/config.json`. You can edit this file directly or use the UI to manage your grid layouts.

## License

MIT
