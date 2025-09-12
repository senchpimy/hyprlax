# Example Configurations

Ready-to-use hyprlax configurations for various themes and styles.

## Single Layer Examples

### Minimalist
```bash
# Simple, clean animation
hyprlax -d 1.0 -e sine -s 150 ~/Pictures/minimal.jpg
```

### Gaming Setup
```bash
# Fast, responsive for gaming
hyprlax -d 0.2 -e expo -s 100 --fps 144 ~/Pictures/gaming.jpg
```

### Relaxed Desktop
```bash
# Slow, smooth transitions
hyprlax -d 2.0 -e sine -s 250 ~/Pictures/nature.jpg
```

### Creative Workspace
```bash
# Bouncy, playful animation
hyprlax -d 0.8 -e elastic -s 200 ~/Pictures/abstract.jpg
```

## Multi-Layer Examples

### Cityscape Parallax

**Command line:**
```bash
hyprlax --layer ~/walls/city-sky.jpg:0.2:1.0:sine:0:1.5:4.0 \
        --layer ~/walls/city-buildings.png:0.5:0.9:expo:0.1:1.2:2.0 \
        --layer ~/walls/city-street.png:1.0:0.8:expo:0.2:1.0:0
```

**Config file** (`~/.config/hyprlax/city.conf`):
```bash
# Cityscape with depth effect
# Distant sky - heavy blur, slow movement
layer /home/user/walls/city-sky.jpg 0.2 1.0 4.0

# Buildings - moderate blur, medium speed
layer /home/user/walls/city-buildings.png 0.5 0.9 2.0

# Street level - sharp, normal speed
layer /home/user/walls/city-street.png 1.0 0.8 0.0

# Smooth urban feel
duration 1.2
shift 200
easing expo
```

### Forest Scene

**Config file** (`~/.config/hyprlax/forest.conf`):
```bash
# Layered forest depth

# Sky through trees - static, blurred
layer /home/user/walls/forest-sky.jpg 0.1 1.0 5.0

# Distant trees - very slow
layer /home/user/walls/forest-back.png 0.3 0.95 3.0

# Middle trees - medium movement
layer /home/user/walls/forest-mid.png 0.6 0.9 1.5

# Foreground foliage - fast, sharp
layer /home/user/walls/forest-front.png 1.2 0.85 0.0

# Natural, organic animation
duration 1.5
shift 220
easing sine
```

### Space Theme

**Command line:**
```bash
hyprlax --layer ~/walls/stars.jpg:0.0:1.0:linear:0:2.0:0 \
        --layer ~/walls/nebula.png:0.2:0.7:sine:0:2.0:3.0 \
        --layer ~/walls/planets.png:0.5:0.9:expo:0.5:1.5:1.0 \
        --layer ~/walls/asteroids.png:1.5:0.6:expo:0.3:1.0:0
```

### Underwater Scene

**Config file** (`~/.config/hyprlax/underwater.conf`):
```bash
# Deep sea parallax

# Dark depths - static background
layer /home/user/walls/ocean-deep.jpg 0.0 1.0 6.0

# Distant fish - slow drift
layer /home/user/walls/fish-back.png 0.3 0.7 3.0

# Coral reef - medium movement
layer /home/user/walls/coral.png 0.6 0.85 1.5

# Bubbles - fast movement
layer /home/user/walls/bubbles.png 1.3 0.5 0.0

# Floating feel
duration 2.0
shift 180
easing sine
fps 60
```

### Abstract Art

**Command line with staggered animations:**
```bash
hyprlax --layer ~/walls/gradient.jpg:0.0:1.0:linear:0:1.0:5.0 \
        --layer ~/walls/shapes1.png:0.4:0.6:back:0.0:1.5:2.0 \
        --layer ~/walls/shapes2.png:0.7:0.5:elastic:0.2:1.3:1.0 \
        --layer ~/walls/shapes3.png:1.1:0.4:expo:0.4:1.0:0 \
        --layer ~/walls/shapes4.png:1.4:0.3:sine:0.6:1.2:0
```

## Seasonal Themes

### Spring

```bash
# Cherry blossoms
hyprlax --layer ~/walls/spring-sky.jpg:0.2:1.0:sine:0:2.0:3.0 \
        --layer ~/walls/spring-trees.png:0.5:0.9:sine:0.1:2.0:1.5 \
        --layer ~/walls/spring-petals.png:1.2:0.6:sine:0.2:2.5:0
```

### Summer

```bash
# Beach scene
hyprlax --layer ~/walls/beach-ocean.jpg:0.3:1.0:sine:0:1.5:2.5 \
        --layer ~/walls/beach-sand.png:0.7:0.95:expo:0.1:1.2:1.0 \
        --layer ~/walls/beach-umbrella.png:1.0:0.9:expo:0.2:1.0:0
```

### Autumn

```bash
# Fall foliage
hyprlax --layer ~/walls/autumn-sky.jpg:0.1:1.0:linear:0:2.0:4.0 \
        --layer ~/walls/autumn-trees.png:0.4:0.95:sine:0:1.8:2.0 \
        --layer ~/walls/autumn-leaves.png:0.8:0.7:back:0.2:1.5:0.5 \
        --layer ~/walls/autumn-ground.png:1.1:0.9:expo:0.3:1.2:0
```

### Winter

```bash
# Snowy landscape
hyprlax --layer ~/walls/winter-sky.jpg:0.15:1.0:linear:0:2.5:3.5 \
        --layer ~/walls/winter-mountains.png:0.35:0.95:sine:0:2.0:2.0 \
        --layer ~/walls/winter-trees.png:0.7:0.9:sine:0.1:1.8:1.0 \
        --layer ~/walls/winter-snow.png:1.3:0.4:linear:0.3:3.0:0
```

## Time of Day Configurations

### Sunrise

**Config file** (`~/.config/hyprlax/sunrise.conf`):
```bash
# Golden hour warmth
layer /home/user/walls/sunrise-sky.jpg 0.2 1.0 3.0
layer /home/user/walls/sunrise-clouds.png 0.4 0.6 2.0
layer /home/user/walls/sunrise-landscape.png 0.8 0.95 0.5
layer /home/user/walls/sunrise-foreground.png 1.0 1.0 0.0

duration 1.8
shift 200
easing sine
```

### Noon

```bash
# Bright, sharp daylight
hyprlax --layer ~/walls/noon-sky.jpg:0.1:1.0:linear:0:1.0:1.0 \
        --layer ~/walls/noon-landscape.png:0.6:1.0:expo:0:0.8:0 \
        --layer ~/walls/noon-foreground.png:1.0:1.0:expo:0:0.8:0
```

### Sunset

```bash
# Dramatic evening colors
hyprlax --layer ~/walls/sunset-sky.jpg:0.2:1.0:sine:0:2.0:4.0 \
        --layer ~/walls/sunset-clouds.png:0.4:0.7:sine:0.2:2.2:2.5 \
        --layer ~/walls/sunset-silhouette.png:0.8:1.0:expo:0.4:1.5:0.5 \
        --layer ~/walls/sunset-ground.png:1.0:1.0:expo:0.5:1.2:0
```

### Night

**Config file** (`~/.config/hyprlax/night.conf`):
```bash
# Starry night
layer /home/user/walls/night-stars.jpg 0.0 1.0 0.0
layer /home/user/walls/night-moon.png 0.1 0.8 2.0
layer /home/user/walls/night-clouds.png 0.3 0.4 3.0
layer /home/user/walls/night-landscape.png 0.7 0.9 1.0
layer /home/user/walls/night-foreground.png 1.0 1.0 0.0

duration 2.5
shift 150
easing sine
fps 30  # Lower FPS for subtle movement
```

## Performance Profiles

### High-End System

```bash
# Maximum quality
hyprlax --fps 144 --vsync 1 \
        --layer ~/walls/8k-bg.jpg:0.15:1.0:expo:0:1.0:5.0 \
        --layer ~/walls/4k-mg1.png:0.3:0.9:expo:0.05:1.0:3.5 \
        --layer ~/walls/4k-mg2.png:0.5:0.85:expo:0.1:1.0:2.0 \
        --layer ~/walls/4k-mg3.png:0.7:0.8:expo:0.15:1.0:1.0 \
        --layer ~/walls/4k-fg.png:1.0:0.75:expo:0.2:1.0:0
```

### Mid-Range System

```bash
# Balanced quality/performance
hyprlax --fps 60 \
        --layer ~/walls/1080p-bg.jpg:0.3:1.0:sine:0:1.2:2.0 \
        --layer ~/walls/1080p-mg.png:0.6:0.9:sine:0.1:1.2:0.5 \
        --layer ~/walls/1080p-fg.png:1.0:0.8:sine:0.2:1.2:0
```

### Low-End System

```bash
# Performance optimized
hyprlax --fps 30 --vsync 0 -d 0.5 -e linear \
        --layer ~/walls/720p-bg.jpg:0.5:1.0 \
        --layer ~/walls/720p-fg.png:1.0:0.9
```

## Hyprland Integration Examples

### Workspace-Specific Wallpapers

Add to `~/.config/hypr/hyprland.conf`:

```bash
# Different wallpaper configs for different workspaces
bind = $mainMod, 1, exec, hyprlax --config ~/.config/hyprlax/workspace1.conf
bind = $mainMod, 2, exec, hyprlax --config ~/.config/hyprlax/workspace2.conf
bind = $mainMod, 3, exec, hyprlax --config ~/.config/hyprlax/workspace3.conf
```

### Random Wallpaper Script

Create `~/.config/hypr/scripts/random-wallpaper.sh`:

```bash
#!/bin/bash

# Array of configurations
configs=(
    "~/.config/hyprlax/city.conf"
    "~/.config/hyprlax/forest.conf"
    "~/.config/hyprlax/space.conf"
    "~/.config/hyprlax/underwater.conf"
)

# Select random config
config=${configs[$RANDOM % ${#configs[@]}]}

# Kill existing and start new
pkill hyprlax
hyprlax --config "$config" &

# Notify
notify-send "Wallpaper Changed" "$(basename $config .conf)" -t 2000
```

### Time-Based Wallpaper

Create `~/.config/hypr/scripts/time-wallpaper.sh`:

```bash
#!/bin/bash

hour=$(date +%H)

if [ $hour -ge 6 ] && [ $hour -lt 12 ]; then
    config="sunrise.conf"
elif [ $hour -ge 12 ] && [ $hour -lt 17 ]; then
    config="noon.conf"
elif [ $hour -ge 17 ] && [ $hour -lt 20 ]; then
    config="sunset.conf"
else
    config="night.conf"
fi

pkill hyprlax
hyprlax --config ~/.config/hyprlax/$config &
```

Add to crontab:
```bash
# Update wallpaper every hour
0 * * * * ~/.config/hypr/scripts/time-wallpaper.sh
```

## Tips for Creating Your Own

1. **Start simple** - Test with 2-3 layers first
2. **Use transparency** - PNG with alpha channel for best results
3. **Consider depth** - Background = slow/blurred, Foreground = fast/sharp
4. **Test animations** - Try different easing functions
5. **Optimize images** - Resize and compress for performance
6. **Save configs** - Create reusable configuration files
7. **Experiment** - Mix and match parameters for unique effects

## Next Steps

- Learn about [multi-layer creation](multi-layer.md)
- Understand [animation options](animation.md)
- Review [troubleshooting](troubleshooting.md) if issues arise