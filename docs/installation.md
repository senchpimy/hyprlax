# Installation Guide

This guide covers all installation methods for hyprlax.

## Quick Install (Recommended)

The easiest way to install hyprlax is using the installation script:

```bash
git clone https://github.com/sandwichfarm/hyprlax.git
cd hyprlax
./install.sh        # Install for current user (~/.local/bin)
```

For system-wide installation:
```bash
./install.sh -s     # Requires sudo, installs to /usr/local/bin
```

The installer will:
- Build hyprlax with optimizations
- Install the binary to the appropriate location
- Set up your PATH if needed
- Restart hyprlax if it's already running (for upgrades)

## Installing from Release

Download pre-built binaries from the [releases page](https://github.com/sandwichfarm/hyprlax/releases):

### For x86_64:
```bash
wget https://github.com/sandwichfarm/hyprlax/releases/latest/download/hyprlax-x86_64
chmod +x hyprlax-x86_64
sudo mv hyprlax-x86_64 /usr/local/bin/hyprlax
```

### For ARM64/aarch64:
```bash
wget https://github.com/sandwichfarm/hyprlax/releases/latest/download/hyprlax-aarch64
chmod +x hyprlax-aarch64
sudo mv hyprlax-aarch64 /usr/local/bin/hyprlax
```

## Building from Source

### Dependencies

#### Arch Linux
```bash
sudo pacman -S base-devel wayland wayland-protocols mesa
```

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential libwayland-dev wayland-protocols \
                 libegl1-mesa-dev libgles2-mesa-dev pkg-config
```

#### Fedora
```bash
sudo dnf install gcc make wayland-devel wayland-protocols-devel \
                 mesa-libEGL-devel mesa-libGLES-devel pkg-config
```

#### openSUSE
```bash
sudo zypper install gcc make wayland-devel wayland-protocols-devel \
                     Mesa-libEGL-devel Mesa-libGLES-devel pkg-config
```

#### Void Linux
```bash
sudo xbps-install base-devel wayland wayland-protocols \
                  MesaLib-devel pkg-config
```

### Build Process

```bash
git clone https://github.com/sandwichfarm/hyprlax.git
cd hyprlax
make
```

### Installation Options

#### User Installation (no sudo required)
```bash
make install-user   # Installs to ~/.local/bin
```

Make sure `~/.local/bin` is in your PATH:
```bash
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

#### System Installation
```bash
sudo make install   # Installs to /usr/local/bin
```

#### Custom Installation
```bash
make PREFIX=/custom/path install
```

## Verifying Installation

Check that hyprlax is installed correctly:

```bash
hyprlax --version
```

You should see:
```
hyprlax 1.2.0
Smooth parallax wallpaper animations for Hyprland
```

## Upgrading

If you already have hyprlax installed, the installer will detect it and perform an upgrade:

```bash
cd hyprlax
git pull
./install.sh  # Will backup existing installation and upgrade
```

## Uninstallation

### If installed via script
```bash
# User installation
rm ~/.local/bin/hyprlax

# System installation
sudo rm /usr/local/bin/hyprlax
```

### If installed via make
```bash
cd hyprlax
make uninstall-user  # For user installation
# OR
sudo make uninstall  # For system installation
```

## Next Steps

- [Configure hyprlax](configuration.md) in your Hyprland config
- Learn about [multi-layer parallax](multi-layer.md)
- Explore [animation options](animation.md)