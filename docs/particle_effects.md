# particle_effects.toml Schema

`data/particle_effects.toml` defines a named library of **emitter presets**. A preset captures
all parameters for a particle burst or continuous emitter; towers and enemies reference presets by
name rather than embedding the raw parameters inline.

All presets live under the `[presets]` table, one sub-table per preset:

```toml
[presets.<preset-name>]
# ... fields ...
```

Preset names are arbitrary strings. They are referenced from:
- Tower modules (`effect`, `muzzle`, `impact`, `critImpact` fields in `towers.toml`)
- Enemy presentation (`deathEmitter` field in `enemies.toml`)

An unknown preset name logs a warning at load time and produces a no-op (zero-count) emitter.

---

## Fields

Every field is optional; omitted fields take the default value shown below.

### Appearance

| Field      | Type      | Default               | Description |
|------------|-----------|-----------------------|-------------|
| `color`    | [R,G,B,A] | `[255, 255, 255, 255]`| Starting particle color (0–255 per channel) |
| `endColor` | [R,G,B,A] | `[255, 255, 255, 0]`  | Color at the end of the particle's lifetime; interpolated from `color` |
| `size`     | float     | `3.0`                 | Starting particle radius in pixels |
| `endSize`  | float     | `0.0`                 | Radius at end of lifetime; `0.0` keeps the size constant |

Color and size both interpolate linearly from their start to their end value over each
particle's lifetime.

### Emission count

| Field      | Type  | Default | Description |
|------------|-------|---------|-------------|
| `count`    | int   | `0`     | Particles spawned per burst (one-shot `Emit` call); `0` = disabled |
| `emitRate` | float | `0.0`   | Particles spawned per second when the emitter runs continuously (live emitter); `0.0` = burst only |

A preset used as a muzzle flash or impact burst only needs `count`; a preset used as a
persistent on-enemy effect (e.g. burn, slow) needs `emitRate`.

### Movement

| Field           | Type  | Default | Description |
|-----------------|-------|---------|-------------|
| `speed`         | float | `50.0`  | Base initial speed of each particle (world units / second) |
| `speedVariance` | float | `20.0`  | Random ± speed added per particle; actual speed ∈ `[speed − variance, speed + variance]` |
| `spread`        | float | `360.0` | Total arc of the emission cone in degrees; `360` = omnidirectional |
| `angle`         | float | `0.0`   | Base emission direction in degrees (`0` = right, `90` = down, `180` = left, `270` = up) |
| `lifetime`      | float | `0.2`   | Seconds each particle lives |

### Spawn shape

By default particles originate from a single **point**. The `shape` field places them on a
geometric primitive instead; the initial velocity is then optionally augmented by radial or
tangential components relative to the shape center.

| Field             | Type   | Default   | Description |
|-------------------|--------|-----------|-------------|
| `shape`           | string | `"Point"` | Spawn primitive — see shape types below |
| `shapeWidth`      | float  | `0.0`     | Width of a `Box` shape |
| `shapeHeight`     | float  | `0.0`     | Height of a `Box` shape |
| `shapeRadius`     | float  | `0.0`     | Radius of a `Circle` or `Ring` shape |
| `radialSpeed`     | float  | `0.0`     | Speed component away from the shape center (`+` = outward, `−` = inward) |
| `tangentialSpeed` | float  | `0.0`     | Speed component perpendicular to the radial direction — makes particles orbit the center |

**Shape types:**

| `shape`    | Spawn position | Shape params used |
|------------|----------------|-------------------|
| `"Point"`  | The emitter's origin | none |
| `"Line"`   | Random point on a line of length `shapeWidth`, oriented by `angle` | `shapeWidth` |
| `"Box"`    | Random point inside a rectangle | `shapeWidth`, `shapeHeight` |
| `"Circle"` | Random point inside a disk | `shapeRadius` |
| `"Ring"`   | Random point on the circumference of a circle | `shapeRadius` |

`radialSpeed` and `tangentialSpeed` are applied in addition to the directional speed from
`speed`/`spread`/`angle`. This lets you layer an outward burst (`radialSpeed > 0`) with a
rotation (`tangentialSpeed ≠ 0`) for effects like spinning rings or spirals.

---

## Complete example

```toml
# Continuous burn effect on an enemy (uses emitRate, shape, radial/tangential)
[presets.burn_effect]
color            = [94, 51, 163, 255]
endColor         = [255, 88, 24, 255]
count            = 5
emitRate         = 18.0
lifetime         = 0.28
size             = 2.5
endSize          = 0.8
speed            = 22.0
speedVariance    = 8.0
angle            = 270.0
spread           = 57.0
shape            = "Circle"
shapeRadius      = 5.0

# One-shot impact burst (uses count only)
[presets.sniper_impact]
color            = [220, 220, 255, 255]
endColor         = [180, 180, 200, 0]
count            = 16
lifetime         = 0.35
size             = 4.0
speed            = 100.0
speedVariance    = 35.0
spread           = 360.0
```

---

## Referencing presets

Presets are referenced by name wherever an emitter is configured:

```toml
# In towers.toml — presentation block
[presentation]
muzzle     = "sniper_muzzle"    # fired at the tower origin on each shot
impact     = "sniper_impact"    # fired at each target on hit
critImpact = "crit_impact"      # additional burst on critical hits

# In towers.toml — module block (status-effect on hit)
{ type = "Slow",  slowPercent = 50, slowDuration = 2.0, effect = "slow_effect"   }
{ type = "Burn",  burnDamage  = 1,  burnDuration = 4.0, effect = "burn_effect"   }
{ type = "Stun",  stunDuration = 1.0,                   effect = "stun_effect"   }

# In enemies.toml — presentation block
presentation = { texture = "enemy_titan", deathEmitter = "death_large" }
```
