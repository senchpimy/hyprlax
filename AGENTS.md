# AGENTS.md

Instructions for AI coding agents working on hyprlax.

## Project Overview

Hyprlax is a smooth parallax wallpaper animation system for Hyprland (Wayland compositor). It creates depth effects by moving multiple image layers at different speeds when switching workspaces.

### Core Technologies
- **Language**: C (C99 standard)
- **Graphics**: OpenGL ES 2.0 with EGL
- **Windowing**: Wayland (layer-shell protocol)
- **Build System**: GNU Make
- **Image Loading**: stb_image (header-only library)

### Architecture
- Single-binary daemon that runs in background
- Connects to Hyprland via IPC socket for workspace events
- GPU-accelerated rendering with custom shaders
- Multi-layer support with per-layer animation properties

## Development Environment

### Required Tools
```bash
# Install dependencies (Arch Linux)
sudo pacman -S base-devel wayland wayland-protocols mesa

# Clone and build
git clone https://github.com/sandwichfarm/hyprlax.git
cd hyprlax
make
```

### Build Commands
```bash
make            # Standard optimized build
make debug      # Debug build with symbols
make clean      # Clean build artifacts
make install    # Install to /usr/local/bin
```

### Testing
```bash
# Test single layer
./hyprlax test.jpg

# Test multi-layer with debug output
./hyprlax --debug --layer bg.jpg:0.3:1.0:expo:0:1.0:3.0 \
                  --layer fg.png:1.0:0.8

# Check for memory leaks
valgrind --leak-check=full ./hyprlax test.jpg
```

## Code Style Guidelines

### C Code Conventions
- **Indentation**: 4 spaces (no tabs)
- **Line Length**: Max 100 characters
- **Braces**: K&R style
- **Naming**:
  - Functions: `snake_case`
  - Variables: `snake_case`
  - Constants: `UPPER_SNAKE_CASE`
  - Structs: `snake_case`
  - Enums: `snake_case_t`

### Code Structure
```c
// Good function example
int load_layer(struct layer *layer, const char *path, 
               float shift_multiplier, float opacity) {
    // Input validation first
    if (!layer || !path) {
        return -1;
    }
    
    // Core logic
    // ...
    
    // Resource cleanup
    // ...
    
    return 0;
}
```

### Error Handling
- Return -1 for errors, 0 for success
- Always check malloc/calloc returns
- Free resources in reverse order of allocation
- Use early returns for error conditions

### Comments
- Use `//` for single-line comments
- Document complex algorithms
- Add TODO comments for future work
- No commented-out code in commits

## Contribution Guidelines

### DO's
- ✅ Maintain backward compatibility
- ✅ Add debug output for new features (behind `config.debug`)
- ✅ Update documentation when adding features
- ✅ Test with both single and multi-layer modes
- ✅ Check for memory leaks with valgrind
- ✅ Follow existing code patterns
- ✅ Add examples for new features
- ✅ Keep performance in mind (144 FPS target)

### DON'Ts
- ❌ Break existing command-line interface
- ❌ Add dependencies without discussion
- ❌ Use C++ features (keep it pure C)
- ❌ Ignore compiler warnings
- ❌ Add blocking operations in render loop
- ❌ Use global variables unnecessarily
- ❌ Mix tabs and spaces
- ❌ Leave debug prints in production code

## File Organization

```
hyprlax/
├── src/
│   ├── hyprlax.c         # Main implementation (single file)
│   └── stb_image.h       # Image loading library
├── protocols/            # Wayland protocol files
├── docs/                 # User documentation
├── examples/             # Example configurations
├── Makefile             # Build configuration
└── AGENTS.md            # This file
```

### Adding Features

When adding new features:
1. Update the `struct layer` or `struct config` as needed
2. Add command-line parsing in `main()`
3. Implement logic in appropriate function
4. Update shaders if needed
5. Add debug output
6. Update documentation
7. Add example usage

## Commit Message Format

Use conventional commits:
```
type: description

[optional body]

[optional footer]
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `perf`: Performance improvement
- `docs`: Documentation only
- `refactor`: Code restructuring
- `test`: Adding tests
- `chore`: Maintenance

Examples:
```
feat: add per-layer blur effects for depth perception
fix: resolve memory leak in texture cleanup
docs: update multi-layer guide with blur examples
perf: optimize shader for better frame rates
```

## Testing Checklist

Before submitting changes:
- [ ] Code compiles without warnings
- [ ] Single-layer mode works
- [ ] Multi-layer mode works
- [ ] No memory leaks (valgrind clean)
- [ ] Animation is smooth (60+ FPS)
- [ ] Debug mode provides useful output
- [ ] Documentation updated
- [ ] Examples work

## Performance Considerations

### Critical Path
The render loop must maintain 144 FPS:
- `render_frame()` - Keep under 7ms
- Texture operations - Use power-of-2 sizes
- Shader complexity - Minimize calculations
- Layer count - Test with 5+ layers

### Memory Management
- Free textures when switching wallpapers
- Clean up layers on exit
- Avoid allocations in render loop
- Use static buffers where possible

## Security Considerations

- Validate all file paths before loading
- Check image dimensions against GPU limits
- Sanitize config file inputs
- Never execute shell commands from input
- Handle IPC errors gracefully

## Common Tasks

### Adding a new easing function
1. Add enum value to `easing_type_t`
2. Implement in `apply_easing()`
3. Add parsing in command-line handler
4. Document in `docs/animation.md`

### Adding a shader uniform
1. Declare uniform in shader source
2. Get location with `glGetUniformLocation()`
3. Set value in render loop
4. Update state structure if needed

### Supporting new image format
1. Check stb_image support
2. Add format validation
3. Update documentation
4. Add test image

## Debug Helpers

```bash
# Enable debug output
export HYPRLAX_DEBUG=1

# Check OpenGL capabilities
glxinfo | grep -E "OpenGL|version"

# Monitor GPU usage
nvidia-smi -l 1  # NVIDIA
radeontop        # AMD

# Trace Wayland protocol
WAYLAND_DEBUG=1 ./hyprlax test.jpg
```

## Project Priorities

1. **Stability** - No crashes, no memory leaks
2. **Performance** - Smooth 144 FPS animation
3. **Compatibility** - Works on all Hyprland setups
4. **Simplicity** - Easy to use and understand
5. **Beauty** - Stunning visual effects

## Version Management

- Version format: MAJOR.MINOR.PATCH
- Update version in `src/hyprlax.c` (#define HYPRLAX_VERSION)
- Tag releases: `git tag -a v1.2.0 -m "Release v1.2.0"`
- Update changelog in README.md

## Resources

- [Wayland Protocol Docs](https://wayland.freedesktop.org/docs/html/)
- [OpenGL ES 2.0 Reference](https://www.khronos.org/opengles/sdk/docs/man/)
- [Layer Shell Protocol](https://github.com/swaywm/wlr-protocols)
- [Hyprland IPC](https://wiki.hyprland.org/IPC/)

## Questions?

- Check existing issues first
- Read documentation thoroughly
- Test your assumptions
- Ask in GitHub issues if stuck

---

Remember: Keep it simple, keep it fast, keep it beautiful.