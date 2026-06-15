# map_generation.toml Schema

`data/map_generation.toml` controls the procedural map generator — the algorithm used when no
custom map is selected. All fields live in the `[map]` section; omitting any field falls back to
the default shown below.

```toml
[map]
# Tile IDs resolved from the TileFactory on generation.
groundId    = "grass"
obstacleId  = "rock"
coreId      = "core"
nestId      = "nest"

# Map geometry and content counts.
cols          = 15
rows          = 19
nestCount     = 3
obstacleCount = 40

# Cluster tuning.
minCluster       = 3
maxCluster       = 7
seedTries        = 64
growTries        = 16
tilesPerBuffTile = 44
```

| Field              | Type   | Default  | Description |
|--------------------|--------|----------|-------------|
| `groundId`         | string | `"grass"`   | Tile ID used for the initial ground fill |
| `obstacleId`       | string | `"rock"`    | Tile ID used for obstacle (rock) clusters |
| `coreId`           | string | `"core"`    | Tile ID used for the player's core |
| `nestId`           | string | `"nest"`    | Tile ID used for enemy spawn nests |
| `cols`             | int    | 15          | Map width in tiles |
| `rows`             | int    | 19          | Map height in tiles |
| `nestCount`        | int    | 3           | Number of enemy spawn nests placed across the top |
| `obstacleCount`    | int    | 40          | Fixed target number of obstacle tiles to place |
| `minCluster`       | int    | 3           | Minimum size (in tiles) of each generated obstacle cluster |
| `maxCluster`       | int    | 7           | Maximum size (in tiles) of each generated obstacle cluster |
| `seedTries`        | int    | 64          | Maximum attempts to find a free seed tile for each new cluster |
| `growTries`        | int    | 16          | Maximum attempts to extend a cluster by one tile before giving up |
| `tilesPerBuffTile` | int    | 44          | Map area (in tiles) divided by this value gives the number of buff tiles placed |

## How procedural generation works

Given a grid of `cols` × `rows`, the generator resolves tile IDs from the active datapack's
`TileFactory` using the IDs configured above, then runs these steps in order:

1. **Ground fill** — the entire grid is filled with the `groundId` tile.
2. **Core** — placed near the horizontal center, two rows from the bottom, using `coreId`.
3. **Nests** — evenly spaced along the top row using `nestId`. The count is `nestCount`, clamped
   so every nest fits within the grid.
4. **Obstacle clusters** — grown until `obstacleCount` tiles have been placed. Each cluster seeds
   from a randomly chosen free ground tile (tried up to `seedTries` times), then expands
   tile-by-tile up to `growTries` attempts per step. The cluster's final size is clamped to
   `[minCluster, maxCluster]`. A candidate obstacle tile is only kept if all nests still have a
   valid path to the core after the placement; tiles that would cut off any path are reverted.
5. **Buff tiles** — placed at `max(1, (cols × rows) / tilesPerBuffTile)` random ground-tile
   positions. Buff types are not hardcoded; the generator calls `TileFactory::GetBuffIds()`,
   which scans every tile definition and returns the IDs of any with an active `[tile.modifier]`
   block. Adding a new buff type only requires a new `[[tile]]` entry in `tiles.toml`. Each buff
   cycles through the available IDs evenly so the map gets a balanced mix. Buff tiles are
   walkable and buildable, so they never affect pathing and need no validation.

The generator uses configurable tile IDs, so any datapack that defines tiles with matching IDs
(and supplies a valid obstacle tile that blocks movement) is compatible. Tile definitions are
loaded from `data/tiles.toml` — see `docs/tiles.md` for the full schema.

The `tilesPerBuffTile` ratio keeps the number of buff tiles proportional to map area: a larger
map gets more buffs without needing separate configuration. Lower values produce more buff tiles;
higher values produce fewer.
