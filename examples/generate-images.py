#!/usr/bin/env python3
"""
Generate test images for hyprlax parallax examples.
Creates multiple themed example sets with layered transparent PNGs.
"""

import os
import json
import math
import random
from PIL import Image, ImageDraw, ImageFilter

# Image dimensions
WIDTH, HEIGHT = 1920, 1080

def validate_path(path):
    """Validate path to prevent directory traversal."""
    # Get absolute path (do not resolve symlinks in user-supplied path for security)
    abs_path = os.path.abspath(path)
    # Base directory should have symlinks resolved for comparison
    base_dir = os.path.realpath(os.path.abspath(os.path.dirname(__file__)))
    
    # Use commonpath to ensure the path is within the examples directory
    try:
        common = os.path.commonpath([abs_path, base_dir])
        if common != base_dir:
            raise ValueError(f"Invalid path: {path} (outside of examples directory)")
    except ValueError:
        # Paths are on different drives (Windows) or invalid
        raise ValueError(f"Invalid path: {path} (outside of examples directory)")
    
    return abs_path

def ensure_dir(path):
    """Create directory if it doesn't exist."""
    validated_path = validate_path(path)
    os.makedirs(validated_path, exist_ok=True)
    return validated_path

def save_layer(img, path):
    """Save image as PNG with transparency."""
    validated_path = validate_path(path)
    img.save(validated_path, format="PNG")
    print(f"  Created: {os.path.basename(validated_path)}")

def gradient_bg(w, h, top_color, bottom_color):
    """Create a gradient background."""
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    for y in range(h):
        # Linear gradient
        r = int(top_color[0] + (bottom_color[0] - top_color[0]) * (y / h))
        g = int(top_color[1] + (bottom_color[1] - top_color[1]) * (y / h))
        b = int(top_color[2] + (bottom_color[2] - top_color[2]) * (y / h))
        draw.line([(0, y), (w, y)], fill=(r, g, b, 255))
    return img

def draw_mountains_layer(color, y_base, variance, scale=1.0, alpha=255, seed=0):
    """Draw a mountain range silhouette."""
    random.seed(seed)
    img = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Build polygon with noisy peaks
    points = []
    x = 0
    step = int(40 * scale)
    while x <= WIDTH + step:
        peak = y_base - variance + random.randint(0, int(variance * 2))
        points.append((x, peak))
        x += step
    
    # Create filled polygon
    poly = [(0, HEIGHT), (0, points[0][1])] + points + [(WIDTH, points[-1][1]), (WIDTH, HEIGHT)]
    draw.polygon(poly, fill=(*color, alpha))
    return img

def draw_clouds_layer(count, y_min, y_max, max_radius=140, alpha=200, seed=0):
    """Draw fluffy clouds."""
    random.seed(seed)
    img = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    for _ in range(count):
        x = random.randint(0, WIDTH)
        y = random.randint(y_min, y_max)
        r = random.randint(int(max_radius * 0.4), max_radius)
        
        # Draw 3-5 overlapping circles to form a cloud
        for i in range(random.randint(3, 5)):
            ox = x + random.randint(-r // 2, r // 2)
            oy = y + random.randint(-r // 3, r // 3)
            rr = int(r * random.uniform(0.6, 1.0))
            draw.ellipse((ox - rr, oy - rr, ox + rr, oy + rr), fill=(255, 255, 255, alpha))
    
    # Apply slight blur for softness
    img = img.filter(ImageFilter.GaussianBlur(2))
    return img

def draw_trees_layer(count, y_base, spread_y, alpha=255, seed=0):
    """Draw a forest of simple trees."""
    random.seed(seed)
    img = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    for _ in range(count):
        x = random.randint(0, WIDTH)
        base = y_base + random.randint(-spread_y, spread_y)
        
        # Draw trunk
        draw.rectangle((x - 5, base - 60, x + 5, base), fill=(80, 50, 30, alpha))
        
        # Draw canopy (overlapping ellipses)
        draw.ellipse((x - 35, base - 95, x + 35, base - 35), fill=(40, 110, 55, alpha))
        draw.ellipse((x - 25, base - 115, x + 25, base - 65), fill=(34, 100, 50, alpha))
    
    return img

def draw_city_layer(rows, alpha=255, seed=0, y_offset=0, color=(50, 50, 60)):
    """Draw a city skyline."""
    random.seed(seed)
    img = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    base_y = int(HEIGHT * 0.65) + y_offset
    
    for r in range(rows):
        # Draw buildings
        for x in range(0, WIDTH, random.randint(60, 130)):
            building_width = random.randint(40, 120)
            building_height = random.randint(80, 260) + r * 30
            x0 = x + random.randint(-10, 10)
            y0 = base_y - building_height - r * 20
            
            # Building silhouette
            draw.rectangle((x0, y0, x0 + building_width, base_y), fill=(*color, alpha))
            
            # Add lit windows for depth
            if r >= 1:
                for wy in range(y0 + 20, base_y - 10, 24):
                    for wx in range(x0 + 10, x0 + building_width - 10, 24):
                        if random.random() < 0.25:
                            draw.rectangle((wx, wy, wx + 8, wy + 12), 
                                         fill=(220, 220, 180, int(alpha * 0.7)))
    return img

def draw_abstract_shapes(count, size_range, color_palette, alpha=255, seed=0):
    """Draw abstract geometric shapes."""
    random.seed(seed)
    img = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    for _ in range(count):
        x = random.randint(0, WIDTH)
        y = random.randint(0, HEIGHT)
        size = random.randint(*size_range)
        color = random.choice(color_palette)
        shape_type = random.choice(['circle', 'rectangle', 'triangle'])
        
        if shape_type == 'circle':
            draw.ellipse((x - size, y - size, x + size, y + size), 
                        fill=(*color, alpha))
        elif shape_type == 'rectangle':
            rotation = random.randint(0, 45)
            draw.rectangle((x - size, y - size, x + size, y + size), 
                          fill=(*color, alpha))
        else:  # triangle
            points = [
                (x, y - size),
                (x - size, y + size),
                (x + size, y + size)
            ]
            draw.polygon(points, fill=(*color, alpha))
    
    return img

def create_mountains_example():
    """Create a mountain scene example."""
    print("\nCreating Mountains Example...")
    scene_dir = ensure_dir("mountains")
    
    # Layer 0: Sky gradient (background)
    sky = gradient_bg(WIDTH, HEIGHT, (30, 60, 120), (170, 210, 255))
    save_layer(sky, os.path.join(scene_dir, "layer0_sky.png"))
    
    # Layer 1: Distant mountains
    far_mountains = draw_mountains_layer((60, 80, 120), int(HEIGHT * 0.7), 80, 
                                        scale=1.8, alpha=210, seed=1)
    save_layer(far_mountains, os.path.join(scene_dir, "layer1_far_mountains.png"))
    
    # Layer 2: Clouds
    clouds = draw_clouds_layer(12, int(HEIGHT * 0.1), int(HEIGHT * 0.5), 
                              alpha=180, seed=2)
    save_layer(clouds, os.path.join(scene_dir, "layer2_clouds.png"))
    
    # Layer 3: Mid-range mountains
    mid_mountains = draw_mountains_layer((40, 70, 90), int(HEIGHT * 0.75), 120, 
                                        scale=1.2, alpha=230, seed=3)
    save_layer(mid_mountains, os.path.join(scene_dir, "layer3_mid_mountains.png"))
    
    # Layer 4: Trees (positioned lower, at base of foreground)
    trees = draw_trees_layer(80, int(HEIGHT * 0.92), 30, alpha=255, seed=4)
    save_layer(trees, os.path.join(scene_dir, "layer4_trees.png"))
    
    # Layer 5: Foreground hill
    foreground = draw_mountains_layer((30, 120, 60), int(HEIGHT * 0.9), 40, 
                                     scale=0.8, alpha=255, seed=5)
    save_layer(foreground, os.path.join(scene_dir, "layer5_foreground.png"))
    
    # Create config file
    config = """# Mountain Scene - Hyprlax Configuration
# Natural landscape with depth

# Sky background - static
layer layer0_sky.png 0.0 1.0 0.0

# Distant mountains - very slow movement, heavy blur
layer layer1_far_mountains.png 0.2 1.0 4.0

# Clouds - slow drift
layer layer2_clouds.png 0.3 0.7 2.0

# Mid-range mountains - medium movement
layer layer3_mid_mountains.png 0.5 1.0 2.0

# Trees - faster movement
layer layer4_trees.png 0.8 1.0 0.5

# Foreground - normal parallax speed
layer layer5_foreground.png 1.0 1.0 0.0

# Animation settings
duration 1.5
shift 200
easing sine
"""
    with open(os.path.join(scene_dir, "parallax.conf"), "w") as f:
        f.write(config)
    
    # Create README
    readme = """# Mountain Scene

A natural landscape with layered depth, featuring mountains, clouds, and trees.

## Layers

1. **Sky** (layer0) - Gradient background, static
2. **Far Mountains** (layer1) - Distant peaks with heavy blur
3. **Clouds** (layer2) - Drifting clouds with transparency
4. **Mid Mountains** (layer3) - Medium distance mountains
5. **Trees** (layer4) - Forest layer
6. **Foreground** (layer5) - Near hill

## Usage

```bash
hyprlax --config mountains/parallax.conf
```

## Customization

Adjust the shift multipliers in `parallax.conf` to change the parallax intensity.
The blur amounts create a depth-of-field effect.
"""
    with open(os.path.join(scene_dir, "README.md"), "w") as f:
        f.write(readme)

def create_city_example():
    """Create a city skyline example."""
    print("\nCreating City Example...")
    scene_dir = ensure_dir("city")
    
    # Layer 0: Night sky gradient
    sky = gradient_bg(WIDTH, HEIGHT, (10, 10, 40), (60, 60, 120))
    save_layer(sky, os.path.join(scene_dir, "layer0_sky.png"))
    
    # Layer 1: Stars
    stars = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(stars)
    random.seed(7)
    for _ in range(600):
        x = random.randint(0, WIDTH - 1)
        y = random.randint(0, int(HEIGHT * 0.6))
        r = random.choice([1, 1, 2])
        draw.ellipse((x - r, y - r, x + r, y + r), fill=(255, 255, 255, 220))
    save_layer(stars, os.path.join(scene_dir, "layer1_stars.png"))
    
    # Layer 2: Far skyline
    far_skyline = draw_city_layer(1, alpha=180, seed=8, y_offset=-80, 
                                  color=(70, 80, 110))
    save_layer(far_skyline, os.path.join(scene_dir, "layer2_far_skyline.png"))
    
    # Layer 3: Mid skyline
    mid_skyline = draw_city_layer(1, alpha=210, seed=9, y_offset=-20, 
                                  color=(60, 70, 95))
    save_layer(mid_skyline, os.path.join(scene_dir, "layer3_mid_skyline.png"))
    
    # Layer 4: Near buildings
    near_skyline = draw_city_layer(1, alpha=255, seed=10, y_offset=20, 
                                   color=(40, 50, 70))
    save_layer(near_skyline, os.path.join(scene_dir, "layer4_near_skyline.png"))
    
    # Layer 5: Street/foreground
    street = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    draw = ImageDraw.Draw(street)
    y0 = int(HEIGHT * 0.85)
    draw.rectangle((0, y0, WIDTH, HEIGHT), fill=(35, 35, 40, 255))
    # Street markings
    cy = y0 + (HEIGHT - y0) // 2
    for x in range(0, WIDTH, 100):
        draw.rectangle((x + 20, cy - 8, x + 60, cy + 8), fill=(230, 230, 230, 255))
    save_layer(street, os.path.join(scene_dir, "layer5_street.png"))
    
    # Create config file
    config = """# City Skyline - Hyprlax Configuration
# Urban nightscape with depth

# Night sky - static background
layer layer0_sky.png 0.0 1.0 0.0

# Stars - very slow drift
layer layer1_stars.png 0.1 0.8 0.0

# Far skyline - slow movement, heavy blur
layer layer2_far_skyline.png 0.3 1.0 3.5

# Mid skyline - medium movement, moderate blur
layer layer3_mid_skyline.png 0.5 1.0 2.0

# Near buildings - faster movement, slight blur
layer layer4_near_skyline.png 0.8 1.0 0.8

# Street foreground - normal speed, sharp
layer layer5_street.png 1.0 1.0 0.0

# Animation settings
duration 1.2
shift 180
easing expo
"""
    with open(os.path.join(scene_dir, "parallax.conf"), "w") as f:
        f.write(config)
    
    # Create README
    readme = """# City Skyline

An urban nightscape with layered buildings and atmospheric depth.

## Layers

1. **Sky** (layer0) - Night gradient, static
2. **Stars** (layer1) - Twinkling stars with slow drift
3. **Far Skyline** (layer2) - Distant buildings with blur
4. **Mid Skyline** (layer3) - Mid-range buildings
5. **Near Buildings** (layer4) - Foreground buildings
6. **Street** (layer5) - Road with lane markings

## Usage

```bash
hyprlax --config city/parallax.conf
```

## Features

- Lit windows in buildings for realism
- Atmospheric perspective with blur
- Street-level foreground for depth
"""
    with open(os.path.join(scene_dir, "README.md"), "w") as f:
        f.write(readme)

def create_abstract_example():
    """Create an abstract geometric example."""
    print("\nCreating Abstract Example...")
    scene_dir = ensure_dir("abstract")
    
    # Color palettes
    palette1 = [(255, 100, 100), (100, 255, 100), (100, 100, 255)]
    palette2 = [(255, 200, 100), (200, 100, 255), (100, 255, 200)]
    palette3 = [(255, 150, 200), (150, 255, 200), (200, 150, 255)]
    
    # Layer 0: Gradient background
    bg = gradient_bg(WIDTH, HEIGHT, (20, 20, 40), (80, 40, 100))
    save_layer(bg, os.path.join(scene_dir, "layer0_gradient.png"))
    
    # Layer 1: Large background shapes
    shapes1 = draw_abstract_shapes(15, (100, 200), palette1, alpha=120, seed=10)
    shapes1 = shapes1.filter(ImageFilter.GaussianBlur(5))
    save_layer(shapes1, os.path.join(scene_dir, "layer1_bg_shapes.png"))
    
    # Layer 2: Medium shapes
    shapes2 = draw_abstract_shapes(25, (50, 150), palette2, alpha=150, seed=11)
    shapes2 = shapes2.filter(ImageFilter.GaussianBlur(3))
    save_layer(shapes2, os.path.join(scene_dir, "layer2_mid_shapes.png"))
    
    # Layer 3: Small shapes
    shapes3 = draw_abstract_shapes(40, (30, 80), palette3, alpha=180, seed=12)
    shapes3 = shapes3.filter(ImageFilter.GaussianBlur(1))
    save_layer(shapes3, os.path.join(scene_dir, "layer3_small_shapes.png"))
    
    # Layer 4: Foreground elements
    shapes4 = draw_abstract_shapes(20, (40, 100), palette1, alpha=220, seed=13)
    save_layer(shapes4, os.path.join(scene_dir, "layer4_foreground.png"))
    
    # Create config file
    config = """# Abstract Geometric - Hyprlax Configuration
# Colorful geometric shapes with motion blur

# Gradient background - static
layer layer0_gradient.png 0.0 1.0 0.0

# Large background shapes - very slow, heavy blur
layer layer1_bg_shapes.png 0.2 1.0 4.0

# Medium shapes - slow movement, moderate blur
layer layer2_mid_shapes.png 0.5 0.8 2.5

# Small shapes - medium movement, light blur
layer layer3_small_shapes.png 0.8 0.7 1.0

# Foreground shapes - normal speed, sharp
layer layer4_foreground.png 1.2 0.6 0.0

# Animation settings for smooth, dreamy effect
duration 2.0
shift 250
easing sine
"""
    with open(os.path.join(scene_dir, "parallax.conf"), "w") as f:
        f.write(config)
    
    # Create README
    readme = """# Abstract Geometric

A colorful abstract composition with floating geometric shapes.

## Layers

1. **Gradient** (layer0) - Color gradient background
2. **Large Shapes** (layer1) - Background geometric forms
3. **Medium Shapes** (layer2) - Mid-layer elements
4. **Small Shapes** (layer3) - Detailed shapes
5. **Foreground** (layer4) - Front layer shapes

## Usage

```bash
hyprlax --config abstract/parallax.conf
```

## Design

- Multiple shape types (circles, rectangles, triangles)
- Vibrant color palettes
- Blur effects for depth perception
- Overlapping transparency for complexity
"""
    with open(os.path.join(scene_dir, "README.md"), "w") as f:
        f.write(readme)

def main():
    """Generate all example sets."""
    print("Hyprlax Example Image Generator")
    print("================================")
    
    # Check for PIL/Pillow
    try:
        from PIL import Image
    except ImportError:
        print("ERROR: Pillow library not installed!")
        print("Install with: pip install Pillow")
        return 1
    
    # Use the script's directory as the examples directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    examples_dir = script_dir  # We're already in the examples directory
    os.chdir(examples_dir)
    
    # Generate each example set
    create_mountains_example()
    create_city_example()
    create_abstract_example()
    
    print("\n✅ All examples created successfully!")
    print(f"📁 Location: {examples_dir}")
    print("\nTo test an example:")
    print("  hyprlax --config examples/mountains/parallax.conf")
    print("  hyprlax --config examples/city/parallax.conf")
    print("  hyprlax --config examples/abstract/parallax.conf")
    
    return 0

if __name__ == "__main__":
    exit(main())