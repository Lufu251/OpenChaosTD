# Map TOML Schema

Custom maps are stored as `map.toml` files inside a map subdirectory:
`maps/<map-name>/map.toml`. An optional `map.png` in the same directory is shown as the
preview image on the map selection screen.

The file has four top-level sections: `[meta]`, `[dimensions]`, `[geometry]`, and `[[tiles]]`.

---

## `[meta]`

Human-readable identifiers shown in the map selection screen.

| Field         | Type   | Description |
|---------------|--------|-------------|
| `name`        | string | Short display name for the map |
| `description` | string | One-line description shown in the selection list |

```toml
[meta]
name        = "default"
description = "the default map"
```

---

## `[dimensions]`

The grid size and the world-space size of each tile.

| Field      | Type | Description |
|------------|------|-------------|
| `cols`     | int  | Number of tile columns (width) |
| `rows`     | int  | Number of tile rows (height) |
| `tileSize` | int  | Side length of each square tile in world units (pixels) |

```toml
[dimensions]
cols     = 20
rows     = 15
tileSize = 32
```

`tileSize` controls the scale of the entire map: a 32 px tile on a 20×15 grid produces a
640×480 world. Pathfinding, tower range, and enemy speed all operate in these world units.

---

## `[geometry]`

Explicit coordinates for the core and every spawn nest. These are redundant with the tile
types in `[[tiles]]` — on load the geometry is re-derived from painted `Core` and `Nest` tiles —
but the section is written by the map editor as a human-readable summary and is read first as
a sanity reference.

| Field   | Type            | Description |
|---------|-----------------|-------------|
| `core`  | `[col, row]`    | Tile coordinate of the core (the goal enemies walk to) |
| `nests` | `[[col, row]]`  | Array of tile coordinates for enemy spawn nests |

```toml
[geometry]
core  = [7, 13]
nests = [[1, 1], [18, 1]]
```

Coordinates are zero-indexed from the top-left corner. `core` must match exactly one tile
painted as `Core` in `[[tiles]]`; each entry in `nests` must match a `Nest` tile.

---

## `[[tiles]]`

A flat, **row-major** array of all tiles in the map. The tile at column `x`, row `y` is at
index `y * cols + x`. The total number of entries must equal `cols × rows`.

Each tile entry is an inline table with the following fields:

| Field      | Type   | Required | Description |
|------------|--------|----------|-------------|
| `type`     | string | yes      | Tile type — see below |
| `walkable` | bool   | yes      | Whether enemies can walk through this tile |
| `buildable`| bool   | yes      | Whether the player can place a tower on this tile |
| `statKey`  | string | no       | Buff modifier stat name — `Buff` tiles only |
| `value`    | float  | no       | Buff modifier magnitude — `Buff` tiles only |
| `mul`      | bool   | no       | `true` = multiplicative modifier, `false` = additive — `Buff` tiles only |

### Tile types

| `type`  | Walkable | Buildable | Description |
|---------|----------|-----------|-------------|
| `Grass` | yes      | yes       | Plain ground tile; enemies walk on it and towers can be built on it |
| `Rock`  | no       | no        | Impassable obstacle; blocks movement and tower placement |
| `Core`  | yes      | no        | The goal tile enemies are heading to; exactly one per map |
| `Nest`  | yes      | no        | An enemy spawn tile; at least one per map, multiple allowed |
| `Buff`  | yes      | yes       | A buildable tile that applies a stat modifier to any tower built on it |

The `walkable` and `buildable` flags in the entry override the defaults shown above when the
loader reads them explicitly. In practice these should match the type's standard values; the
editor always writes them to keep the file self-describing.

### Buff tile fields

A `Buff` tile requires three additional fields that describe the modifier it applies to a tower
placed on it. The modifier reuses the same key-and-delta convention as tower upgrades
(`towers.md`):

| Field     | Type   | Description |
|-----------|--------|-------------|
| `statKey` | string | The tower stat to modify — any key accepted by the tower's modules (e.g. `"damage"`, `"range"`, `"shotsPerMinute"`) |
| `value`   | float  | Modifier magnitude |
| `mul`     | bool   | `true` = `stat = stat × value`; `false` = `stat = stat + value` |

```toml
# Additive +30 range buff
[[tiles]]
type      = "Buff"
walkable  = true
buildable = true
statKey   = "range"
value     = 30.0
mul       = false

# Multiplicative ×1.5 damage buff
[[tiles]]
type      = "Buff"
walkable  = true
buildable = true
statKey   = "damage"
value     = 1.5
mul       = true
```

The modifier is applied when a tower is placed on the tile and removed when the tower is sold.
It stacks with the tower's own upgrades; the ordering matches the upgrade pipeline (base stat,
then tile buff, then purchased upgrades in level order).

### Minimal tile examples

```toml
[[tiles]]
type      = "Grass"
walkable  = true
buildable = true

[[tiles]]
type      = "Rock"
walkable  = false
buildable = false

[[tiles]]
type      = "Nest"
walkable  = true
buildable = false

[[tiles]]
type      = "Core"
walkable  = true
buildable = false
```
