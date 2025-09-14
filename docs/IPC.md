# hyprlax IPC Interface

hyprlax now supports dynamic layer management at runtime through an IPC (Inter-Process Communication) interface using Unix domain sockets.

## Overview

The IPC interface allows you to:
- Add new image layers without restarting hyprlax
- Remove existing layers
- Modify layer properties (opacity, scale, position, visibility)
- List all active layers
- Clear all layers
- Query hyprlax status

## Using hyprlax-ctl

The `hyprlax-ctl` command-line tool is provided to interact with the IPC interface.

### Commands

#### Add a layer
```bash
hyprlax-ctl add <image_path> [options]
```

Options:
- `scale=N` - Scale factor (default: 1.0)
- `opacity=N` - Opacity 0.0-1.0 (default: 1.0)
- `x=N` - X offset in pixels (default: 0)
- `y=N` - Y offset in pixels (default: 0)
- `z=N` - Z-index for layer ordering (default: auto)

Example:
```bash
hyprlax-ctl add /path/to/image.png scale=1.5 opacity=0.8
```

#### Remove a layer
```bash
hyprlax-ctl remove <layer_id>
# or
hyprlax-ctl rm <layer_id>
```

#### Modify a layer
```bash
hyprlax-ctl modify <layer_id> <property> <value>
# or
hyprlax-ctl mod <layer_id> <property> <value>
```

Properties:
- `scale` - Scale factor
- `opacity` - Opacity (0.0-1.0)
- `x` - X offset
- `y` - Y offset
- `z` - Z-index
- `visible` - true/false or 1/0

Example:
```bash
hyprlax-ctl modify 1 opacity 0.5
hyprlax-ctl mod 2 visible false
```

#### List layers
```bash
hyprlax-ctl list
# or
hyprlax-ctl ls
```

#### Clear all layers
```bash
hyprlax-ctl clear
```

#### Get status
```bash
hyprlax-ctl status
```

## Socket Location

The IPC socket is created at `/tmp/hyprlax-$USER.sock` where `$USER` is your username.

## Security

The socket is created with permissions 0600 (user read/write only) to prevent unauthorized access.

## Scripting Examples

### Slideshow
```bash
#!/bin/bash
# Simple slideshow script
images=(/path/to/images/*.jpg)
for img in "${images[@]}"; do
    hyprlax-ctl clear
    hyprlax-ctl add "$img"
    sleep 10
done
```

### Fade transition
```bash
#!/bin/bash
# Fade between two images
hyprlax-ctl add image1.jpg opacity=1.0
hyprlax-ctl add image2.jpg opacity=0.0 z=1

# Fade out image1, fade in image2
for i in {10..0}; do
    hyprlax-ctl modify 1 opacity "0.$i"
    hyprlax-ctl modify 2 opacity "0.$((10-i))"
    sleep 0.1
done
```

## Limitations

- Maximum 32 layers by default (configurable in source)
- Images must be accessible by hyprlax process
- Changes are applied immediately but may take a frame to become visible
- Memory usage increases with each loaded layer

## Troubleshooting

If `hyprlax-ctl` cannot connect:
1. Ensure hyprlax is running
2. Check the socket exists: `ls -la /tmp/hyprlax-*.sock`
3. Verify permissions on the socket file
4. Check hyprlax was built with IPC support (version 1.3.0+)