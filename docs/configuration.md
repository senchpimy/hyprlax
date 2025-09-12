# Configuration Guide

This guide covers all configuration options for hyprlax.

## Hyprland Integration

### Basic Setup

Add to your `~/.config/hypr/hyprland.conf`:

```bash
# Kill existing wallpaper daemons to avoid conflicts
exec-once = pkill swww-daemon; pkill hyprpaper; pkill hyprlax

# Start hyprlax with a single wallpaper
exec-once = hyprlax ~/Pictures/wallpaper.jpg
```

### Auto-start with Custom Settings

```bash
# With animation customization
exec-once = hyprlax -d 1.0 -s 200 -e expo ~/Pictures/wallpaper.jpg

# Multi-layer setup
exec-once = hyprlax --config ~/.config/hyprlax/parallax.conf
```

## Command-Line Options

### Global Options

| Option | Long Form | Description | Default |
|--------|-----------|-------------|---------|
| `-s` | `--shift` | Pixels to shift per workspace | 200 |
| `-d` | `--duration` | Animation duration in seconds | 1.0 |
| | `--delay` | Delay before animation starts | 0 |
| `-e` | `--easing` | Easing function type | expo |
| `-f` | `--scale` | Image scale factor | auto |
| `-v` | `--vsync` | Enable vsync (0 or 1) | 1 |
| | `--fps` | Target frame rate | 144 |
| | `--debug` | Enable debug output | off |
| | `--version` | Show version information | |
| `-h` | `--help` | Show help message | |

### Multi-Layer Options

| Option | Description |
|--------|-------------|
| `--layer` | Add a layer with specified parameters |
| `--config` | Load configuration from file |

## Configuration Files

### File Location

Hyprlax looks for configuration files in:
1. Path specified with `--config`
2. `~/.config/hyprlax/parallax.conf` (if using --config without path)

### File Format

Configuration files use a simple text format:

```bash
# Comments start with #
# Commands are: layer, duration, shift, easing, delay, fps

# Add layers (required for multi-layer mode)
layer <image_path> <shift> <opacity> [blur]

# Global settings (optional)
duration <seconds>
shift <pixels>
easing <type>
delay <seconds>
fps <rate>
```

### Example Configuration

`~/.config/hyprlax/parallax.conf`:
```bash
# Parallax wallpaper configuration

# Background - slow movement, heavy blur
layer /home/user/walls/sky.jpg 0.3 1.0 3.0

# Midground - medium speed, light blur
layer /home/user/walls/mountains.png 0.6 0.9 1.0

# Foreground - normal speed, no blur
layer /home/user/walls/trees.png 1.0 0.8 0.0

# Animation settings
duration 1.2
shift 250
easing expo
delay 0
fps 144
```

## Layer Specification Format

When using `--layer` on the command line or `layer` in config files:

### Command Line Format
```bash
--layer image:shift:opacity[:easing[:delay[:duration[:blur]]]]
```

### Config File Format
```bash
layer image shift opacity [blur]
```

### Parameters

| Parameter | Description | Range | Required |
|-----------|-------------|-------|----------|
| `image` | Path to image file | - | Yes |
| `shift` | Movement multiplier | 0.0-2.0 | Yes |
| `opacity` | Layer transparency | 0.0-1.0 | Yes |
| `blur` | Blur amount for depth | 0.0-10.0 | No |
| `easing` | Per-layer easing (CLI only) | See easing types | No |
| `delay` | Animation delay (CLI only) | 0.0+ seconds | No |
| `duration` | Animation time (CLI only) | 0.1+ seconds | No |

### Shift Multiplier Values
- `0.0` - Static (no movement)
- `0.1-0.3` - Very slow (far background)
- `0.4-0.6` - Slow (background)
- `0.7-0.9` - Medium (midground)
- `1.0` - Normal (standard parallax)
- `1.1-1.5` - Fast (foreground)
- `1.5-2.0` - Very fast (extreme foreground)

### Blur Recommendations
- `0.0` - No blur (sharp, foreground elements)
- `0.5-1.5` - Subtle blur (midground elements)
- `2.0-3.0` - Moderate blur (background elements)
- `3.0-5.0` - Heavy blur (distant background)
- `5.0+` - Extreme blur (atmospheric effects)

## Environment Variables

Hyprlax respects the following environment variables:

| Variable | Description | Default |
|----------|-------------|---------|
| `HYPRLAX_DEBUG` | Enable debug output | 0 |
| `HYPRLAX_CONFIG` | Default config file path | ~/.config/hyprlax/parallax.conf |

## Managing Hyprlax

### Starting and Stopping

```bash
# Start hyprlax
hyprlax ~/Pictures/wallpaper.jpg &

# Stop hyprlax
pkill hyprlax

# Restart with new wallpaper
pkill hyprlax && hyprlax ~/Pictures/new-wallpaper.jpg &
```

### Wallpaper Switching Script

Create `~/.config/hypr/scripts/wallpaper.sh`:

```bash
#!/bin/bash
# Wallpaper switcher for hyprlax

WALLPAPER="$1"
CONFIG="$2"

# Kill existing instance
pkill hyprlax

# Start with config file if provided
if [ -n "$CONFIG" ]; then
    hyprlax --config "$CONFIG" &
elif [ -n "$WALLPAPER" ]; then
    hyprlax "$WALLPAPER" &
else
    # Random wallpaper from directory
    WALLPAPER=$(find ~/Pictures/Wallpapers -type f \
                \( -name "*.jpg" -o -name "*.png" \) | shuf -n 1)
    hyprlax "$WALLPAPER" &
fi
```

### Hyprland Keybindings

```bash
# Switch to random wallpaper
bind = $mainMod, W, exec, ~/.config/hypr/scripts/wallpaper.sh

# Switch to specific config
bind = $mainMod SHIFT, W, exec, ~/.config/hypr/scripts/wallpaper.sh "" ~/.config/hyprlax/night.conf
```

## Performance Tuning

### For Low-End Systems

```bash
# Reduce FPS and disable vsync
hyprlax --fps 30 --vsync 0 wallpaper.jpg

# Use fewer layers with less blur
hyprlax --layer bg.jpg:0.3:1.0:expo:0:1.0:1.0 \
        --layer fg.png:1.0:0.8
```

### For High-End Systems

```bash
# Maximum quality with multiple layers
hyprlax --fps 144 --vsync 1 \
        --layer bg.jpg:0.2:1.0:expo:0:1.0:4.0 \
        --layer mg1.png:0.4:0.9:expo:0.05:1.0:2.5 \
        --layer mg2.png:0.7:0.8:expo:0.1:1.0:1.0 \
        --layer fg.png:1.0:0.7:expo:0.15:1.0:0
```

## Next Steps

- Learn about [multi-layer parallax](multi-layer.md)
- Explore [animation options](animation.md)
- See [example configurations](examples.md)