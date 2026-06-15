# map_generation.toml Schema

`data/map_generation.toml` controls the procedural map generator — the algorithm used when no
custom map is selected. All fields live in the `[map]` section; omitting any field falls back to
the default shown below.

```toml
[map]
minCluster       = 3
maxCluster       = 7
seedTries        = 64
growTries        = 16
tilesPerBuffTile = 44
```

| Field              | Type | Default | Description |
|--------------------|------|---------|-------------|
| `minCluster`       | int  | 3       | Minimum size (in tiles) of each generated obstacle cluster |
| `maxCluster`       | int  | 7       | Maximum size (in tiles) of each generated obstacle cluster |
| `seedTries`        | int  | 64      | Maximum attempts to find a free seed tile for each new cluster |
| `growTries`        | int  | 16      | Maximum attempts to extend a cluster by one tile before giving up |
| `tilesPerBuffTile` | int  | 44      | Map area (in tiles) divided by this value gives the number of buff tiles placed |

## How procedural generation works

Given a grid of the requested dimensions, the generator queries the active datapack's
`TileFactory` for tiles by category and then runs these steps in order:

1. **Core (goal category)** — placed near the horizontal center, two rows from the bottom.
   The tile ID and properties come from the first tile definition with category `"goal"`.
2. **Nests (spawn category)** — placed evenly spaced along the top row. The exact count is
   determined by the map size; the generator ensures at least one. The tile ID and properties
   come from the first tile definition with category `"spawn"`.
3. **Obstacle clusters** — grown until the requested obstacle count is reached. Each cluster
   seeds from a randomly chosen free ground tile (tried up to `seedTries` times), then expands
   tile-by-tile up to `growTries` attempts per step. The cluster's final size is clamped to
   `[minCluster, maxCluster]`. A candidate obstacle tile is only kept if all nests still have a
   valid path to the core after the placement; tiles that would cut off any path are skipped.
   Obstacle tile IDs and properties come from the first tile definition with category `"obstacle"`.
4. **Buff tiles (buff category)** — placed at `max(1, (cols × rows) / tilesPerBuffTile)` random
   buildable positions on ground-category tiles. Each is assigned one of the available stat
   modifier types (`damage`, `range`, or `shotsPerMinute`). See `maps.md` for the buff tile
   schema if you author buff tiles manually. The buff tile ID comes from the first tile
   definition with category `"buff"`.

The generator uses tile categories rather than hardcoded IDs, so any datapack that defines
tiles with the recognized categories (`"ground"`, `"obstacle"`, `"goal"`, `"spawn"`, `"buff"`)
is compatible. Tile definitions are loaded from `data/tiles.toml` — see `docs/tiles.md` for
the full schema.

The `tilesPerBuffTile` ratio keeps the number of buff tiles proportional to map area: a larger
map gets more buffs without needing separate configuration. Lower values produce more buff tiles;
higher values produce fewer.
