# Tile TOML Schema

All tile types are defined in `data/tiles.toml` as a TOML array of tables under `[[tile]]`. Each entry configures a single tile type that can be placed on the map via the editor or procedural generation.

## Fields

| Field              | Type   | Required | Description |
|--------------------|--------|----------|-------------|
| `id`               | string | yes      | Unique tile type identifier. Used as `Tile::m_tileId` at runtime and referenced by map files and save-games. Keep stable across datapack versions to avoid breaking existing maps. |
| `category`         | string | yes      | Semantic role. Must be one of: `"ground"`, `"obstacle"`, `"goal"`, `"spawn"`, `"buff"`. Determines how game systems (map generation, pathfinding, editor) treat this tile. |
| `walkable`         | bool   | no       | Whether enemies can walk through this tile. Default: `true`. |
| `buildable`        | bool   | no       | Whether towers can be placed on this tile. Default: `true`. |
| `textures`         | array  | yes      | Array of asset keys for tile textures, resolved through the resource system. One texture is randomly picked per tile instance when placed, giving each tile visual variety. The index is stored as `m_textureIndex` on the tile so it persists across saves. |
| `texture`          | string | no       | Legacy single-texture field. If `textures` is absent, `texture` is used as a single-element array. Prefer `textures` for new definitions. |
| `textureVariants`  | table  | no       | Optional stat-key → texture(s) overrides. When a tile has an active modifier whose `statKey` matches a variant key, the variant texture is used instead of the default. Each value can be a string or an array of strings. Only meaningful for category `"buff"`. |

## Categories

Categories are a controlled vocabulary — only these five values are recognized:

| Category     | Walkable | Buildable | Semantic |
|--------------|----------|-----------|----------|
| `"ground"`   | true     | true      | Default walkable terrain. Towers can be placed freely. |
| `"obstacle"` | false    | false     | Impassable, unbuildable. Blocks both enemy movement and tower placement. |
| `"goal"`     | true     | false     | Pathfinding target. Exactly one per map. Enemies reaching this tile cost the player lives. |
| `"spawn"`    | true     | false     | Enemy spawn point. One or more per map. Enemies enter the grid at these tiles. |
| `"buff"`     | true     | true      | Walkable and buildable like ground, but applies a `TileModifier` to any tower placed on it. |

The `walkable` and `buildable` columns above show the canonical defaults, but the actual values are read from the TOML definition — a mod could make a `"goal"` buildable or an `"obstacle"` walkable if the gameplay calls for it.

## textureVariants

For tiles with category `"buff"`, the `textureVariants` table maps stat keys to alternative texture asset keys. Each value can be a single string or an array of strings (for visual variety). When a tile instance has an active `TileModifier` whose `m_statKey` matches a variant key, the variant texture is used instead of the default. The tile's `m_textureIndex` is used to pick from the variant array.

```toml
# Single variant texture
textureVariants = { range = "tile_grass_range", damage = "tile_grass_damage" }

# Array variant textures
textureVariants = { range = ["tile_grass_range", "tile_grass_range2"] }
```

If no variant matches the modifier's stat key, or if `textureVariants` is absent, the default `textures` entry is used as a fallback.

## Example

```toml
[[tile]]
id = "grass"
category = "ground"
textures = ["tile_grass", "tile_grass1", "tile_grass2"]

[[tile]]
id = "rock"
category = "obstacle"
walkable = false
buildable = false
textures = ["tile_rock"]

[[tile]]
id = "core"
category = "goal"
walkable = true
buildable = false
textures = ["tile_core"]

[[tile]]
id = "nest"
category = "spawn"
walkable = true
buildable = false
textures = ["tile_nest"]

[[tile]]
id = "buff"
category = "buff"
textures = ["tile_grass"]
textureVariants = { range = ["tile_grass_range"], damage = ["tile_grass_damage"], shotsPerMinute = ["tile_grass_attackspeed"] }
```

## Map File Format

Map files (`maps/*/map.toml`) reference tiles by their `id`. Each tile entry in the `[[tiles]]` array has fields `tileId`, `category`, `walkable`, and `buildable`. For buff tiles, modifier fields (`statKey`, `value`, `mul`) are also present. See `docs/maps.md` for the full map TOML schema.
