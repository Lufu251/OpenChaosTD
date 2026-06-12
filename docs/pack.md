# pack.toml Schema

`pack.toml` is the **datapack manifest**. It must sit at the root of the datapack directory
(`datapacks/<pack-name>/pack.toml`). The selection screen reads it to enumerate installed packs;
a directory without a valid `pack.toml` is silently skipped.

All five fields are **required**. A pack with any missing or empty field is rejected and does not
appear in the selection list.

| Field         | Type   | Description |
|---------------|--------|-------------|
| `name`        | string | Display name shown in the selection screen |
| `author`      | string | Pack author's name or handle |
| `version`     | string | Version string (e.g. `"1.0.0"`); freeform, not parsed as semver |
| `description` | string | One-line summary shown below the pack name |
| `icon`        | string | Path to the pack's icon image, relative to the pack root |

The `icon` path is resolved relative to the pack directory. A missing icon file logs a warning
and shows a placeholder in the selection screen. Supported formats are those accepted by raylib
(PNG, BMP, TGA, JPG, GIF, QOI, PSD, DDS, HDR, KTX, ASTC, PKM, PVR).

## Example

```toml
name        = "My Custom Pack"
author      = "YourName"
version     = "0.1.0"
description = "A custom scenario with new enemies and towers."
icon        = "icon.png"
```

## Datapack layout

A complete datapack is a directory under `datapacks/` with this structure:

```
datapacks/<pack-name>/
├── pack.toml                     ← this file (required)
├── icon.png                      ← pack icon (path set by the icon key above)
├── data/
│   ├── towers.toml               ← tower definitions (towers.md)
│   ├── enemies.toml              ← enemy definitions (enemies.md)
│   ├── waves.toml                ← wave generator config (waves.md)
│   ├── gameplay.toml             ← economic/pacing constants (gameplay.md)
│   ├── map_generation.toml       ← procedural map parameters (map_generation.md)
│   └── particle_effects.toml     ← emitter preset library (particle_effects.md)
├── maps/
│   └── <map-name>/
│       ├── map.toml              ← map geometry (maps.md)
│       └── map.png               ← optional preview image (shown on the map selection screen)
└── resources/
    ├── textures/                 ← sprites referenced by towers and enemies
    ├── sounds/                   ← sound files (key = filename without extension)
    └── music/                    ← background music tracks
```

The `data/` files are all optional at the filesystem level — a missing file is skipped with a
warning and the factory for that subsystem stays empty. In practice a playable pack needs at
least `towers.toml`, `enemies.toml`, and `waves.toml`.

Datapacks in the selection list are sorted **alphabetically by `name`**, regardless of directory
order or creation time.
