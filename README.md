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
- ⚡ **Lightweight** - Native Wayland client using layer-shell protocol
- 🎨 **Customizable** - Multiple easing functions and animation parameters
- 🔄 **Seamless transitions** - Interrupts and chains animations smoothly

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

### Basic Usage

```bash
hyprlax /path/to/your/wallpaper.jpg
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

| Option | Description | Default |
|--------|-------------|---------|
| `-s, --shift <pixels>` | Pixels to shift per workspace | 200 |
| `-d, --duration <seconds>` | Animation duration | 1.0 |
| `--delay <seconds>` | Delay before animation starts | 0 |
| `-e, --easing <type>` | Easing function (see below) | expo |
| `-f, --scale <factor>` | Image scale factor | auto |
| `--fps <rate>` | Target frame rate | 144 |
| `--debug` | Enable debug output | off |

#### Easing Functions

- `linear` - Constant speed
- `quad`, `cubic`, `quart`, `quint` - Power curves (2-5)
- `sine` - Smooth sine wave
- `expo` - Exponential (default)
- `circ` - Circular
- `back` - Slight overshoot
- `elastic` - Bouncy
- `snap` - Custom snappy curve

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
# With custom animation settings
exec-once = hyprlax -d 1.0 -s 200 -e expo ~/Pictures/wallpaper.jpg

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

- **For best results**: Use high-resolution images (at least 2x your screen width)
- **Performance**: Lower `--fps` if you experience high GPU usage
- **Multiple monitors**: Currently supports single monitor setups (multi-monitor support coming soon)

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

## License

MIT

## Contributing

Pull requests are welcome! Please read [RELEASE.md](RELEASE.md) for the release process.