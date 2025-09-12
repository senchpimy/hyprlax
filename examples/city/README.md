# City Skyline

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
