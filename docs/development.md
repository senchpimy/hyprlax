# Development Guide

Information for building, modifying, and contributing to hyprlax.

## Building from Source

### Prerequisites

Required tools:
- GCC or Clang (C compiler)
- GNU Make
- pkg-config
- Wayland scanner

Required libraries:
- Wayland client libraries
- Wayland protocols
- EGL (OpenGL ES context creation)
- OpenGL ES 2.0
- Math library (libm)

### Getting the Source

```bash
git clone https://github.com/sandwichfarm/hyprlax.git
cd hyprlax
```

### Build Commands

```bash
# Standard build
make

# Debug build
make debug

# Clean build
make clean && make

# Specific architecture
make ARCH=aarch64
```

### Build Options

The Makefile supports various options:

```bash
# Custom compiler
make CC=clang

# Custom optimization
make CFLAGS="-O2 -march=native"

# Custom installation prefix
make PREFIX=/opt/hyprlax install

# Verbose output
make VERBOSE=1
```

## Project Structure

```
hyprlax/
├── src/
│   ├── hyprlax.c          # Main source file
│   └── stb_image.h        # Image loading library (header-only)
├── protocols/
│   ├── wlr-layer-shell-unstable-v1.xml  # Layer shell protocol
│   └── (generated files)                 # Protocol implementations
├── docs/                  # Documentation
├── examples/              # Example configurations
├── Makefile              # Build configuration
├── install.sh            # Installation script
└── README.md             # Project documentation
```

## Architecture Overview

### Main Components

1. **Wayland Client**
   - Connects to compositor
   - Creates surface with layer-shell
   - Handles display events

2. **OpenGL Renderer**
   - EGL context creation
   - Shader compilation
   - Texture management
   - Frame rendering

3. **Animation System**
   - Easing functions
   - Per-layer timing
   - Frame scheduling

4. **Hyprland IPC**
   - Socket connection
   - Workspace change detection
   - Event handling

### Key Data Structures

```c
// Layer information
struct layer {
    GLuint texture;
    int width, height;
    float shift_multiplier;
    float opacity;
    char *image_path;
    float current_offset;
    float target_offset;
    float start_offset;
    easing_type_t easing;
    float animation_delay;
    float animation_duration;
    double animation_start;
    int animating;
    float blur_amount;
};

// Global state
struct {
    // Wayland objects
    struct wl_display *display;
    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layer_surface;
    
    // OpenGL state
    EGLDisplay egl_display;
    EGLContext egl_context;
    GLuint shader_program;
    GLuint blur_shader_program;
    
    // Animation state
    float current_offset;
    float target_offset;
    int animating;
    
    // Multi-layer support
    struct layer *layers;
    int layer_count;
} state;
```

## Adding Features

### Adding a New Easing Function

1. Add enum value in `hyprlax.c`:
```c
typedef enum {
    // ... existing easings ...
    EASE_BOUNCE_OUT,  // New easing
} easing_type_t;
```

2. Implement in `apply_easing()`:
```c
case EASE_BOUNCE_OUT: {
    if (t < 0.363636) {
        return 7.5625f * t * t;
    } else if (t < 0.727272) {
        t -= 0.545454f;
        return 7.5625f * t * t + 0.75f;
    }
    // ... more bounce logic
}
```

3. Add command-line parsing:
```c
else if (strcmp(optarg, "bounce") == 0) config.easing = EASE_BOUNCE_OUT;
```

### Adding a New Layer Property

1. Add to layer structure:
```c
struct layer {
    // ... existing fields ...
    float rotation;  // New property
};
```

2. Parse in command line/config:
```c
char *rotation_str = strtok(NULL, ":");
layer->rotation = rotation_str ? atof(rotation_str) : 0.0f;
```

3. Apply in rendering:
```c
// Add rotation matrix to vertex shader
// Update uniform values
```

### Adding a New Command-Line Option

1. Add to long_options array:
```c
{"brightness", required_argument, 0, 0},
```

2. Handle in option parsing:
```c
if (strcmp(long_options[option_index].name, "brightness") == 0) {
    config.brightness = atof(optarg);
}
```

3. Apply in appropriate location

## Debugging

### Debug Build

```bash
# Build with debug symbols
make debug

# Run with gdb
gdb ./hyprlax
(gdb) run wallpaper.jpg
```

### Debug Output

```bash
# Enable debug messages
hyprlax --debug wallpaper.jpg

# Or set environment variable
HYPRLAX_DEBUG=1 hyprlax wallpaper.jpg
```

### Common Debug Points

```c
// Add debug output
if (config.debug) {
    printf("Debug: current_offset=%.2f target=%.2f\n", 
           state.current_offset, state.target_offset);
}

// Check OpenGL errors
GLenum err = glGetError();
if (err != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL error: 0x%x\n", err);
}
```

### Valgrind Memory Check

```bash
valgrind --leak-check=full ./hyprlax wallpaper.jpg
```

## Testing

### Manual Testing

```bash
# Test single layer
./hyprlax test.jpg

# Test multi-layer
./hyprlax --layer bg.jpg:0.3:1.0 --layer fg.png:1.0:0.8

# Test animation
./hyprlax -e elastic -d 2.0 test.jpg
# Switch workspaces to see animation
```

### Automated Testing

Create `test.sh`:
```bash
#!/bin/bash

# Test build
make clean && make || exit 1

# Test single layer
timeout 5 ./hyprlax test.jpg &
PID=$!
sleep 2
kill $PID

# Test multi-layer
timeout 5 ./hyprlax --layer test.jpg:0.5:1.0 \
                    --layer test.jpg:1.0:0.5 &
PID=$!
sleep 2
kill $PID

echo "Tests passed!"
```

## Contributing

### Code Style

- Use 4 spaces for indentation
- Keep lines under 100 characters
- Use snake_case for functions and variables
- Use UPPER_CASE for constants
- Comment complex logic

### Commit Messages

Follow conventional commits:
```
feat: Add new blur shader
fix: Resolve memory leak in layer cleanup
docs: Update multi-layer guide
perf: Optimize texture loading
refactor: Simplify animation system
```

### Pull Request Process

1. Fork the repository
2. Create feature branch: `git checkout -b feature/my-feature`
3. Make changes and test
4. Commit with clear messages
5. Push to your fork
6. Open pull request with description

### Areas for Contribution

- **Features**
  - Multi-monitor support
  - Video wallpaper support
  - Dynamic layer loading ([#1](https://github.com/sandwichfarm/hyprlax/issues/1))
  
- **Performance**
  - Optimize blur shader
  - Reduce memory usage
  - Improve texture loading
  
- **Documentation**
  - Add more examples
  - Translate to other languages
  - Create video tutorials
  
- **Testing**
  - Add unit tests
  - Create integration tests
  - Test on different systems

## Release Process

### Version Numbering

Follow semantic versioning:
- MAJOR.MINOR.PATCH
- Example: 1.2.0

### Release Steps

1. Update version in `hyprlax.c`:
```c
#define HYPRLAX_VERSION "1.2.0"
```

2. Update CHANGELOG.md

3. Tag release:
```bash
git tag -a v1.2.0 -m "Release version 1.2.0"
git push origin v1.2.0
```

4. GitHub Actions will automatically:
   - Build binaries for x86_64 and aarch64
   - Create GitHub release
   - Upload artifacts

## Resources

### Documentation
- [Wayland Protocol](https://wayland.freedesktop.org/docs/html/)
- [Layer Shell Protocol](https://github.com/swaywm/wlr-protocols)
- [OpenGL ES 2.0 Reference](https://www.khronos.org/opengles/sdk/docs/man/)
- [EGL Reference](https://www.khronos.org/registry/EGL/sdk/docs/man/)

### Tools
- [Wayland Debug](https://github.com/wmww/wayland-debug) - Protocol debugging
- [RenderDoc](https://renderdoc.org/) - Graphics debugging
- [apitrace](https://github.com/apitrace/apitrace) - OpenGL tracing

### Similar Projects
- [swww](https://github.com/Horus645/swww) - Wayland wallpaper daemon
- [hyprpaper](https://github.com/hyprwm/hyprpaper) - Hyprland wallpaper utility
- [swaybg](https://github.com/swaywm/swaybg) - Sway wallpaper tool

## Contact

- GitHub Issues: https://github.com/sandwichfarm/hyprlax/issues
- Pull Requests: https://github.com/sandwichfarm/hyprlax/pulls