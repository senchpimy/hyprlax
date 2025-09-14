# Changelog

All notable changes to hyprlax will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.1] - 2025-09-14

### Fixed
- 🐛 Config-loaded layers now properly integrate with IPC system
- 🔧 `hyprlax-ctl list` now shows config-loaded layers instead of returning "No layers"
- 🎮 Background no longer disappears when using IPC commands
- 🖼️ Parallax animation now works on all workspaces (was breaking after workspace 5)
- 🔄 Config layers can now be dynamically managed via hyprlax-ctl

### Changed
- Improved workspace detection to always assume at least 10 workspaces
- sync_ipc_layers() now preserves config-loaded layers instead of destroying them

## [1.3.0] - 2025-09-14

### Added
- 🎮 **Dynamic Layer Management via IPC** - Add, remove, and modify wallpaper layers at runtime without restarting
- 🔧 **hyprlax-ctl command-line tool** - Control hyprlax layers dynamically via IPC
- 📚 Comprehensive IPC documentation in `docs/IPC.md`
- 🧹 Local linting tools and pre-commit hooks for code quality
- 🔍 `make lint` and `make lint-fix` targets for code formatting
- 📦 hyprlax-ctl binary now included in releases and install scripts
- ✅ Extensive test suite for IPC functionality

### Changed
- 📝 Updated README with dynamic layer management documentation
- 🔧 Enhanced install scripts to handle both hyprlax and hyprlax-ctl binaries
- 🏗️ Improved build system with CI-specific flags to avoid architecture issues
- 📦 Release workflow now packages both binaries

### Fixed
- 🐛 Fixed CI build failures by removing architecture-specific flags in CI environment
- 🔧 Resolved static analysis warnings and formatting issues
- 📝 Fixed missing newlines at end of files
- ⚠️ Fixed sign comparison warnings in hyprlax-ctl

### Technical Details
- IPC implementation uses Unix domain sockets at `/tmp/hyprlax-$USER.sock`
- Secure socket permissions (0600) for user-only access
- Support for up to 32 concurrent layers
- Real-time layer synchronization with OpenGL textures
- Memory-safe dynamic layer allocation and deallocation

## [1.2.0] - Previous Release

### Added
- 🌌 Multi-layer parallax support - Create depth with multiple independent layers
- 🔍 Blur effects - Per-layer blur for realistic depth-of-field
- 🎨 Per-layer animation controls - Individual easing, delays, and durations
- 📝 Configuration file support - Load complex setups from config files
- ⚡ Phase 3 optimizations - Improved rendering pipeline for multiple layers

## [1.1.0] - Initial Release

### Added
- 🎬 Initial release with smooth parallax animations
- ⚡ GPU-accelerated rendering
- 🎨 Multiple easing functions
- 🔄 Seamless animation interruption

[1.3.1]: https://github.com/sandwichfarm/hyprlax/releases/tag/v1.3.1
[1.3.0]: https://github.com/sandwichfarm/hyprlax/releases/tag/v1.3.0
[1.2.0]: https://github.com/sandwichfarm/hyprlax/releases/tag/v1.2.0
[1.1.0]: https://github.com/sandwichfarm/hyprlax/releases/tag/v1.1.0