# Mountain Scene

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
