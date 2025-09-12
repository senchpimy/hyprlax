# hyprlax

Smooth parallax wallpaper animations for Hyprland.

<p align="center">
  <img src="https://img.shields.io/badge/Hyprland-Compatible-blue" alt="Hyprland Compatible">
  <img src="https://img.shields.io/badge/Wayland-Native-green" alt="Wayland Native">
  <img src="https://img.shields.io/badge/GPU-Accelerated-orange" alt="GPU Accelerated">
</p>

## Features

- 🎬 **Buttery smooth animations** - GPU-accelerated rendering at up to 144 FPS
- 🖼️ **Parallax effect** - Wallpaper shifts as you switch workspaces
- 🌌 **Multi-layer parallax** - Create depth with multiple layers moving at different speeds
- 🔍 **Depth-of-field blur** - Realistic depth perception with per-layer blur effects
- ⚡ **Lightweight** - Native Wayland client using layer-shell protocol
- 🎨 **Customizable** - Per-layer easing functions, delays, and animation parameters
- 🔄 **Seamless transitions** - Interrupts and chains animations smoothly
- 🎯 **Phase 3 features** - Advanced per-layer controls for professional effects

## Installation

### Quick Install (Recommended)

```bash
git clone https://github.com/yourusername/hyprlax.git
cd hyprlax
./install.sh        # Install for current user
# OR
./install.sh -s     # Install system-wide (requires sudo)
```

### From Release

Download the latest binary from the [releases page](https://github.com/yourusername/hyprlax/releases):

```bash
wget https://github.com/yourusername/hyprlax/releases/latest/download/hyprlax-x86_64
chmod +x hyprlax-x86_64
sudo mv hyprlax-x86_64 /usr/local/bin/hyprlax
```

### Manual Build

#### Dependencies

```bash
# Arch Linux
sudo pacman -S base-devel wayland wayland-protocols mesa

# Ubuntu/Debian
sudo apt install build-essential libwayland-dev wayland-protocols libegl1-mesa-dev libgles2-mesa-dev pkg-config

# Fedora
sudo dnf install gcc make wayland-devel wayland-protocols-devel mesa-libEGL-devel mesa-libGLES-devel pkg-config
```

#### Build & Install

```bash
git clone https://github.com/yourusername/hyprlax.git
cd hyprlax
make
make install-user   # Install to ~/.local/bin (no sudo)
# OR
sudo make install   # Install to /usr/local/bin
```

## Usage

### Basic Usage (Single Layer)

```bash
hyprlax /path/to/your/wallpaper.jpg
```

### Multi-Layer Parallax

```bash
# Basic multi-layer with different speeds
hyprlax --layer background.jpg:0.3:1.0 \
        --layer midground.png:0.6:0.8 \
        --layer foreground.png:1.0:0.5

# Advanced with blur for depth perception
hyprlax --layer background.jpg:0.3:1.0:expo:0:1.0:3.0 \
        --layer midground.png:0.6:0.8:expo:0.1:1.0:1.5 \
        --layer foreground.png:1.0:0.5:expo:0.2:1.0:0

# Using a configuration file
hyprlax --config ~/.config/hyprlax/parallax.conf
```

### With Custom Settings

```bash
# Slower, more dramatic animation
hyprlax -d 1.5 -s 250 /path/to/wallpaper.jpg

# Bouncy effect
hyprlax -e elastic -d 0.8 /path/to/wallpaper.jpg

# Fast and snappy
hyprlax -e expo -d 0.3 -s 150 /path/to/wallpaper.jpg
```

### Options

#### Global Options
| Option | Description | Default |
|--------|-------------|---------|
| `-s, --shift <pixels>` | Pixels to shift per workspace | 200 |
| `-d, --duration <seconds>` | Animation duration | 1.0 |
| `--delay <seconds>` | Delay before animation starts | 0 |
| `-e, --easing <type>` | Easing function (see below) | expo |
| `-f, --scale <factor>` | Image scale factor | auto |
| `--fps <rate>` | Target frame rate | 144 |
| `--debug` | Enable debug output | off |

#### Multi-Layer Options
| Option | Description |
|--------|-------------|
| `--layer <spec>` | Add layer: `image:shift:opacity[:easing[:delay[:duration[:blur]]]]` |
| `--config <file>` | Load layers from configuration file |

**Layer specification format:**
- `image` - Path to image file (required)
- `shift` - Movement multiplier: 0.0=static, 1.0=normal, 2.0=double speed
- `opacity` - Layer transparency: 0.0-1.0
- `easing` - Per-layer easing function (optional)
- `delay` - Animation start delay in seconds (optional)
- `duration` - Animation duration (optional)
- `blur` - Blur amount for depth: 0.0-10.0 (optional)

#### Easing Functions

- `linear` - Constant speed
- `quad`, `cubic`, `quart`, `quint` - Power curves (2-5)
- `sine` - Smooth sine wave
- `expo` - Exponential (default)
- `circ` - Circular
- `back` - Slight overshoot
- `elastic` - Bouncy
- `snap` - Custom snappy curve

## Multi-Layer Configuration File

Create a configuration file for complex multi-layer setups:

`~/.config/hyprlax/parallax.conf`:
```bash
# Multi-layer parallax configuration
# Format: layer <image_path> <shift> <opacity> [blur]

# Sky/background - moves slowly, heavily blurred for depth
layer /path/to/sky.jpg 0.2 1.0 3.0

# Mountains - medium distance, slightly blurred
layer /path/to/mountains.png 0.5 0.9 1.5

# Trees - foreground, no blur
layer /path/to/trees.png 1.0 0.8 0.0

# Global animation settings
duration 1.2
shift 250
easing expo
```

Then use it:
```bash
hyprlax --config ~/.config/hyprlax/parallax.conf
```

## Hyprland Configuration

### Basic Setup

Add to your `~/.config/hypr/hyprland.conf`:

```bash
# Kill any existing wallpaper daemons to avoid conflicts
exec-once = pkill swww-daemon; pkill hyprpaper; pkill hyprlax

# Start hyprlax with your wallpaper
exec-once = hyprlax ~/Pictures/your-wallpaper.jpg
```

### Advanced Configuration

```bash
# Single layer with custom animation settings
exec-once = hyprlax -d 1.0 -s 200 -e expo ~/Pictures/wallpaper.jpg

# Multi-layer parallax effect
exec-once = hyprlax --layer ~/Pictures/bg.jpg:0.3:1.0:expo:0:1.0:2.0 \
                    --layer ~/Pictures/mg.png:0.6:0.8:expo:0.1:1.0:1.0 \
                    --layer ~/Pictures/fg.png:1.0:0.6

# Using a config file
exec-once = hyprlax --config ~/.config/hyprlax/parallax.conf

# Different settings for different moods:
# Smooth and relaxed
exec-once = hyprlax -d 1.5 -s 250 -e sine ~/Pictures/wallpaper.jpg

# Fast and snappy
exec-once = hyprlax -d 0.3 -s 150 -e quint ~/Pictures/wallpaper.jpg

# Bouncy and playful
exec-once = hyprlax -d 0.8 -e elastic ~/Pictures/wallpaper.jpg
```

### Managing hyprlax

```bash
# Restart hyprlax (useful for changing wallpapers)
pkill hyprlax && hyprlax ~/Pictures/new-wallpaper.jpg

# Stop hyprlax
pkill hyprlax

# Check if hyprlax is running
pgrep -x hyprlax
```

### Integration with Wallpaper Switchers

Create a script `~/.config/hypr/scripts/wallpaper.sh`:

```bash
#!/bin/bash
# Script to switch wallpapers with hyprlax

WALLPAPER="$1"
if [ -z "$WALLPAPER" ]; then
    WALLPAPER=$(find ~/Pictures/Wallpapers -type f \( -name "*.jpg" -o -name "*.png" \) | shuf -n 1)
fi

pkill hyprlax
hyprlax "$WALLPAPER" &
```

Then bind it in Hyprland:
```bash
bind = $mainMod, W, exec, ~/.config/hypr/scripts/wallpaper.sh
```

## Tips

### Single Layer Mode
- **For best results**: Use high-resolution images (at least 2x your screen width)
- **Performance**: Lower `--fps` if you experience high GPU usage
- **Multiple monitors**: Currently supports single monitor setups (multi-monitor support coming soon)

### Multi-Layer Mode
- **Creating layers**: Use PNG images with transparency for best results
- **Depth perception**: 
  - Background layers: Use shift values 0.1-0.3 and blur 2.0-4.0
  - Midground layers: Use shift values 0.4-0.7 and blur 0.5-2.0
  - Foreground layers: Use shift values 0.8-1.2 and blur 0.0-0.5
- **Performance optimization**:
  - Limit to 3-4 layers for smooth performance
  - Use lower resolution for heavily blurred background layers
  - Reduce opacity on layers that don't need to be fully opaque
- **Animation staggering**: Use small delays (0.05-0.2s) between layers for natural movement
- **Layer preparation**: Tools like GIMP or Photoshop can help separate images into layers

## Troubleshooting

### Black screen
- Ensure your image path is correct
- Check that the image is a supported format (JPEG, PNG)

### Animation stuttering
- Try disabling vsync: `hyprlax --vsync 0 /path/to/image.jpg`
- Lower the target FPS: `hyprlax --fps 60 /path/to/image.jpg`

### Not starting
- Check if another wallpaper daemon is running: `pkill swww-daemon hyprpaper`
- Ensure Hyprland is running

## Changelog

### v1.2.0 (Latest)
- 🌌 **Multi-layer parallax support** - Create depth with multiple independent layers
- 🔍 **Blur effects** - Per-layer blur for realistic depth-of-field
- 🎨 **Per-layer animation controls** - Individual easing, delays, and durations
- 📝 **Configuration file support** - Load complex setups from config files
- ⚡ **Phase 3 optimizations** - Improved rendering pipeline for multiple layers

### v1.1.0
- 🎬 Initial release with smooth parallax animations
- ⚡ GPU-accelerated rendering
- 🎨 Multiple easing functions
- 🔄 Seamless animation interruption

## License

MIT

## Contributing

Pull requests are welcome! Please read [RELEASE.md](RELEASE.md) for the release process.

## Roadmap

- [ ] Dynamic layer loading/unloading ([#1](https://github.com/sandwichfarm/hyprlax/issues/1))
- [ ] Multi-monitor support
- [ ] Video wallpaper support
- [ ] Integration with wallpaper managers