# Custom Container Usage Audit

Comprehensive audit of the three custom container types — `DenseSlotMap`, `Grid2D`, and
`ObjectPool` — across the entire OpenChaosTD engine codebase.

**Audit date:** 2026-06-10
**Scope:** All `src/` files. Static analysis via grep + manual review of every call site.

---

## 1. Executive Summary

All three containers are well-designed and correctly used. The audit found **zero bugs** and
**zero unsafe usage patterns**. One minor efficiency observation is noted in §3.2.

| Container | Definition | Instantiations | Files Touching | Issues |
|---|---|---|---|---|
| `DenseSlotMap` | `src/engine/lib/dense_slotmap.hpp` (147 lines) | 3 | ~20 | None |
| `Grid2D` | `src/engine/lib/grid2d.hpp` (94 lines) | 4 | ~9 | Minor: redundant `Get` calls in map setup |
| `ObjectPool` | `src/engine/lib/object_pool.hpp` (63 lines) | 1 | ~4 | None |

The codebase consistently applies safe patterns for working with these containers:

- **Deferred erase** — keys are collected in a scratch vector during iteration, then erased
  after the loop exits. Never in-place during iteration.
- **INVALID_KEY sentinel** — every key field is initialised to `INVALID_KEY` and checked
  against it before dereference.
- **nullptr guard** — `Get()` returns `nullptr` for stale keys; every caller checks the pointer.
- **Swap-and-pop manual loop** — `while` loop with conditional `i++` is the only pattern that
  releases elements mid-iteration.

---

## 2. DenseSlotMap

### 2.1 Container Design

`DenseSlotMap<T>` (`src/engine/lib/dense_slotmap.hpp`) is a slot map (sparse set with
generation counters). It provides:

- **Stable key handles** (`Key` with `index` + `generation`). A key survives insertion and
  removal of other elements; re-using a slot index increments the generation so old keys
  become stale.
- **O(1) insert** — appends to the dense array, recycles freed slot indices via a free list.
- **O(1) erase** — swap-and-pop on the dense array, then generation invalidation.
- **O(1) lookup** — `Get(Key)` dereferences the slot index, checks generation, returns pointer
  or `nullptr`.
- **Direct iteration** — `begin()`/`end()` iterate the dense `m_values` vector with zero
  indirection and no gaps.
- **KeyOf** — recovers the `Key` from a raw pointer into the dense array.
- **Serialization support** — `RawSlots()`, `RawValues()`, `RawErase()`, `RawFreeList()`, and
  `RawAssign()` let external serializers persist and restore the exact sparse bookkeeping so
  keys survive save/load cycles.

Internal storage uses four vectors:

| Vector | Role |
|---|---|
| `m_slots` | Sparse — indexed by `Key.index`. Holds generation, dense index, occupied flag. |
| `m_values` | Dense — the actual `T` objects. Iterated directly. |
| `m_erase` | Reverse map — `m_values[i]` → slot index. |
| `m_freeList` | Recycled slot indices for O(1) reuse. |

### 2.2 Instantiations

#### `DenseSlotMap<Tower>` — `GameData::m_towers`

| Property | Detail |
|---|---|
| Field | `GameData::m_towers` (`src/world/game_data.hpp:28`) |
| Purpose | Stores all player-placed towers with stable keys |
| Key stored in | `Tile::m_towerKey` (`src/world/tile.hpp:36`) — each tile knows which tower sits on it |
| Serialization | Full raw state persisted via `RawSlots`/`RawValues`/`RawErase`/`RawFreeList`; towers reconstructed through `TowerFactory` (`src/world/save_serialization.hpp:90`) |
| Remove pattern | `WorldSystem::RemoveTower` — single-key erase after clearing the tile key. No deferred erase needed. |
| Safety | `INVALID_KEY` checked before every dereference; generation counters make stale keys safe |

#### `DenseSlotMap<Enemy>` — `GameData::m_enemies`

| Property | Detail |
|---|---|
| Field | `GameData::m_enemies` (`src/world/game_data.hpp:29`) |
| Purpose | Stores all live enemies with stable keys for targeting |
| Key stored in | `AttackPayload::m_targetKeys` (`src/world/attack.hpp:17`) — snapshot at fire time |
| Serialization | Not persisted (enemies are transient per-wave) |
| Remove pattern | Deferred erase in `CheckEnemyReachedCore` and `CheckEnemyDead` — keys collected in `std::vector`, erased after the loop |
| Safety | Attack payloads snapshot keys at fire time; `Get` returns `nullptr` for dead enemies, handled gracefully in `TickAttacks` |

#### `DenseSlotMap<Emitter>` — `ParticleSystem::m_emitters`

| Property | Detail |
|---|---|
| Field | `ParticleSystem::m_emitters` (`src/engine/systems/particle_system.hpp:92`) |
| Purpose | Stores continuous particle emitters with stable handles |
| Handle alias | `EmitterHandle = DenseSlotMap<Emitter>::Key` (`particle_system.hpp:63`) |
| Key stored in | `Effect::m_emitter` (`src/world/effect.hpp:23`), `ParticleEditorState::m_liveEmitter` (`src/states/particle_editor_state.hpp:101`) |
| Serialization | Not persisted |
| Remove pattern | Deferred erase in `ParticleSystem::Tick` — expired emitter handles collected in `m_expiredEmitters`, erased after the loop |
| Safety | `UpdateEmitter` and `RemoveEmitter` are safe no-ops on stale handles; `INVALID_KEY` used as sentinel throughout |

### 2.3 Deferred Erase Pattern

The codebase consistently uses a deferred erase pattern when removing elements during
iteration. This is the correct approach for any container with swap-and-pop semantics:

```cpp
// WorldSystem::CheckEnemyDead (world_system.cpp:96-153)
std::vector<DenseSlotMap<Enemy>::Key> toRemove;
for (Enemy& enemy : gameData.m_enemies) {
    if (enemy.m_health <= 0) {
        toRemove.push_back(gameData.m_enemies.KeyOf(&enemy));
    }
}
for (auto key : toRemove)
    gameData.m_enemies.Erase(key);
```

The same pattern appears in `CheckEnemyReachedCore` and `ParticleSystem::Tick` for emitter
expiry. No range-for or index-based loop calls `Erase` during iteration — this invariant holds
across the entire codebase.

### 2.4 Serialization Strategy

`DenseSlotMap` is IO-agnostic — it exposes raw accessors but has no JSON dependency. The
serialization code lives in `src/world/save_serialization.hpp` and
`src/world/save_serialization.cpp`:

- **Save:** `RawSlots()`, `RawValues()`, `RawErase()`, and `RawFreeList()` are dumped verbatim
  to JSON. This preserves every key's `(index, generation)` pair so tile tower keys survive
  the round-trip.
- **Load:** The four vectors are reconstructed from JSON, consistency-validated, then
  committed atomically via `RawAssign()`. Towers themselves are rebuilt by name through
  `TowerFactory` (polymorphic module lists are not serialized).
- **Safety:** The load path constructs into a local `DenseSlotMap<Tower>` and only moves it
  into `GameData::m_towers` after validation succeeds. If any step fails, the live game state
  is untouched.

Games are only saved between waves, so enemies and active attacks are intentionally excluded
from the persisted state.

---

## 3. Grid2D

### 3.1 Container Design

`Grid2D<T>` (`src/engine/lib/grid2d.hpp`) is a flat-vector-backed 2D grid with row-major
indexing. It provides:

- **Row-major storage** — cell `(x, y)` is at `m_data[y * m_width + x]`.
- **Bounds checking** — every `Get`/`Set` call routes through private `CheckBounds`, which
  throws `std::out_of_range` on invalid coordinates in all build modes.
- **Public `InBounds` predicate** — lets callers guard access before paying the bounds-check
  cost twice.
- **`Resize`** — re-allocates and fills all cells.
- **`GetVector`** — exposes the raw `std::vector<T>` for bulk serialization.
- **Iterators** — `begin()`/`end()` iterate the flat vector in row-major order.
- **Copyable/movable** — inherits `std::vector`'s copy and move semantics.
- **Default constructor** — produces a 0×0 grid; `Get`/`Set` will throw on any coordinates.

### 3.2 Instantiations

#### `Grid2D<Tile>` — `Map::m_grid`

| Property | Detail |
|---|---|
| Field | `Map::m_grid` (`src/world/map.hpp:49`) |
| Purpose | The authoritative game map tile grid |
| Cell type | `Tile` — enum + booleans + `TileModifier` + `DenseSlotMap<Tower>::Key` (~48+ bytes) |
| Access pattern | `Map::Get(x, y)` forwards to `m_grid.Get(x, y)`, which goes through `CheckBounds` |
| Bounds safety | `Map::WorldToTile` guards with `InBounds` before using coordinates. All public tile access is bounds-checked. |
| Serialization | JSON via generic `to_json`/`from_json` templates in `save_serialization.hpp:58-73` |

#### `Grid2D<Node>` — `Map::m_pathMesh`

| Property | Detail |
|---|---|
| Field | `Map::m_pathMesh` (`src/world/map.hpp:47`) |
| Purpose | BFS result — every cell holds distance-to-core and predecessor coordinate |
| Cell type | `Node` — `int` distance + `std::pair<int,int>` predecessor |
| Access pattern | Read-only after `BuildPathMesh`. Consumed by `ConstructPaths` and `ValidatePathMesh`. |
| Bounds safety | `ConstructPaths` checks `InBounds` before `Get`. `ValidatePathMesh` reads nest coordinates that are structurally guaranteed valid. |
| Lifecycle | Produced by `Pathfinder::Solve` (returned by value, moved into place) |

#### `WalkableMask = Grid2D<unsigned char>`

| Property | Detail |
|---|---|
| Type alias | `using WalkableMask = Grid2D<unsigned char>` (`src/systems/pathfinder.hpp:17`) |
| Purpose | Boolean walkability input for the pathfinder — 1 = walkable, 0 = blocked |
| Design note | `unsigned char` instead of `bool` avoids `std::vector<bool>` specialization (which cannot return a reference, breaking `Get`/`Set`) |
| Scope | Produced by `Map::BuildPathMesh`, consumed by `Pathfinder::BuildGraph`/`BfsSolve` |

#### `Grid2D<std::vector<Edge>>` — `Graph::m_adj`

| Property | Detail |
|---|---|
| Field | `Graph::m_adj` (`src/systems/pathfinder.cpp:23`) |
| Purpose | Internal adjacency graph — each cell holds a vector of edges to walkable neighbours |
| Scope | File-local anonymous namespace in `pathfinder.cpp`. No external code touches it. |
| Usage | Built by `BuildGraph`, consumed by `BfsSolve`. Fully private to the pathfinder. |

### 3.3 Bounds Safety

Every `Get`/`Set` call goes through `CheckBounds` (verified via grep — no unchecked access
path exists in the public API). `InBounds` is used as a guard in three locations:

1. `Map::WorldToTile` (`map.cpp:20`) — gates coordinate conversion before any tile access.
2. `Pathfinder::BfsSolve` (`pathfinder.cpp:74`) — checks `graph.InBounds(start)` before
   proceeding; returns an empty 0×0 grid on failure.
3. `Map::ConstructPaths` (`map.cpp:101`) — guards `m_pathMesh.Get()` for nest coordinates.

`ValidatePathMesh` (`map.cpp:88`) calls `m_pathMesh.Get()` without an explicit `InBounds`
check. This is safe because nest coordinates are set by `AddNest` (which references the same
tile grid) and are structurally guaranteed in-bounds. If a bug ever placed a nest outside the
grid, `Get` would throw `std::out_of_range` — no silent corruption.

---

## 4. ObjectPool

### 4.1 Container Design

`ObjectPool<T>` (`src/engine/lib/object_pool.hpp`) is a fixed-capacity object pool. It
provides:

- **Pre-allocation** — memory is allocated once in the constructor; `Acquire`/`ReleaseAt`
  never allocate.
- **Contiguous active range** — active objects occupy `[0, m_size)` with no gaps. Suitable
  for index iteration, range-for, and cache-friendly access.
- **Swap-and-pop release** — `ReleaseAt(index)` overwrites the released slot with the last
  active element (`m_objects[--m_size]`). The caller must not advance the loop index after
  release.
- **Overflow returns `nullptr`** — `Acquire()` returns `nullptr` when at capacity; the caller
  decides policy (silent drop is the convention in this codebase).
- **O(1) `Clear`** — sets `m_size = 0` without touching contents.
- **`operator[]`** — with debug-build assertion on the index.

### 4.2 Instantiation

#### `ObjectPool<Particle>` — `ParticleSystem::m_particles`

| Property | Detail |
|---|---|
| Field | `ParticleSystem::m_particles` (`src/engine/systems/particle_system.hpp:91`) |
| Capacity | 2048 (`kMaxParticles`, line 85) |
| Cell type | `Particle` — plain struct with position, velocity, color, lifetime (~160 bytes). Trivially movable. |
| Purpose | Stores all active particles from burst emissions and continuous emitters |
| Overflow handling | `SpawnParticle` checks `Acquire()` return — `if (!p) return;` (line 44). Particles are silently dropped at capacity. |
| Tick iteration | Manual `while` loop with conditional `i++` — textbook swap-and-pop pattern (lines 118-139) |
| Draw iteration | Range-for loop — safe because `Draw` is `const` and does not mutate the pool |
| Clear | `Clear()` at line 153 resets `m_size = 0` — O(1), no per-particle destructor needed |

### 4.3 Swap-and-Pop Pattern Verification

The pool is iterated in exactly two places:

**1. Tick (mutation allowed):**

```cpp
// particle_system.cpp:118-139
size_t i = 0;
while (i < m_particles.Size()) {
    Particle& p = m_particles[i];
    // ... update physics, decrement lifetime ...
    if (p.lifetime <= 0.0f) {
        m_particles.ReleaseAt(i);
        // i unchanged — the former last particle is now at this index
    } else {
        i++;
    }
}
```

When `ReleaseAt(i)` fires, the element from `m_size - 1` moves into position `i`, and
`m_size` decrements. The loop re-examines position `i`, which now holds the formerly-last
particle. This is correct.

**2. Draw (read-only):**

```cpp
// particle_system.cpp:142-149
void ParticleSystem::Draw() const {
    for (const auto& p : m_particles) {
        // ... read fields, draw circle ...
    }
}
```

The method is `const`, so no structural mutation occurs during iteration. Safe.

No other iteration patterns exist in the codebase. There are no external callers iterating the
pool, no `for (size_t i = 0; i < pool.Size(); ++i)` loops that release, and no range-for
loops that call `ReleaseAt`.

### 4.4 Contiguity Benefits

The contiguous `[0, Size())` layout is leveraged in three ways:

1. **Index iteration in Tick** — `m_particles[i]` is direct vector indexing with no
   indirection.
2. **Range-for iteration in Draw** — `begin()`/`end()` point to `data()` and `data() +
   size()`, standard contiguous iterator semantics.
3. **O(1) Clear** — `m_size = 0` is sufficient; no per-element cleanup or gap compaction
   needed.

At 2048 × ~160 bytes ≈ 328 KB total, the entire particle set fits comfortably in L2/L3 cache,
making per-frame iteration cache-friendly.

---

## 5. Assessment and Recommendations

### 5.1 Keep / Modify / Replace

| Container | Decision | Rationale |
|---|---|---|
| `DenseSlotMap` | **Keep** | Stable keys with generation counters are the right choice for entities that need external references (`Tile→Tower`, `Attack→Enemy`, `Effect→Emitter`). Serialization via raw accessors is clean and keeps the template IO-free. |
| `Grid2D` | **Keep** | Flat row-major layout is the standard choice for dense 2D grids. Bounds checking on every access is a minor performance cost but a significant safety benefit in a game where coordinates come from player input and pathfinding. |
| `ObjectPool` | **Keep** | Fixed-capacity pre-allocated pool is the right choice for particles — hard cap with deterministic performance. `nullptr` on overflow lets the caller decide policy. |

No container needs to be replaced with a standard library equivalent. Each container fills a
specific need that `std::vector` or `std::unordered_map` alone would not satisfy as cleanly:

- `DenseSlotMap` provides both stable handles **and** contiguous iteration — `std::unordered_map`
  gives stable references but non-contiguous iteration; `std::vector` gives contiguous iteration
  but index-based references that break on removal.
- `Grid2D` provides 2D-indexed access with bounds checking — `std::vector<std::vector<T>>`
  would add per-row allocation overhead and lose the single-contiguous-block property.
- `ObjectPool` provides O(1) acquire/release with contiguous active range — `std::vector` with
  erase would need O(n) compaction or leave gaps.

### 5.2 Minor Efficiency Observation

**Location:** `src/world/map.cpp:27-32` (`SetCore`) and `map.cpp:41-43` (`AddNest`)

Both functions call `m_grid.Get(cols, rows)` three times in succession to set `m_type`,
`m_buildable`, and `m_walkable` on the same tile:

```cpp
// SetCore (lines 27-32)
m_grid.Get(cols, rows).m_type = TileType::Core;
m_grid.Get(cols, rows).m_buildable = false;
m_grid.Get(cols, rows).m_walkable = true;
```

Caching the reference would be cleaner:

```cpp
Tile& tile = m_grid.Get(cols, rows);
tile.m_type = TileType::Core;
tile.m_buildable = false;
tile.m_walkable = true;
```

Note that `SetBuff` (`map.cpp:47-48`) already does this correctly. Impact is trivial — these
functions run once during map setup, not in a hot loop. This is a style consistency note, not
a performance concern.

### 5.3 Safety Patterns to Preserve

These patterns are used correctly throughout the codebase. Future development should continue
them:

1. **Deferred erase for DenseSlotMap** — keys are collected in a scratch vector during
   iteration, then erased after the loop. Never call `Erase` inside a range-for or index-based
   loop over the same container.

2. **INVALID_KEY sentinel** — every key field is initialised to `INVALID_KEY` and checked
   against it before dereference (`if (key != INVALID_KEY)`). All key dereferences through
   `Get()` are guarded by `INVALID_KEY` or an explicit nullptr check.

3. **nullptr from Get** — `Get()` returns `nullptr` for invalid/deleted keys. Every caller
   checks the pointer before use (`if (auto* ptr = map.Get(key))`).

4. **Swap-and-pop manual loop** — when releasing elements from `ObjectPool` during iteration,
   use the `while`-loop pattern with conditional `i++`. Do not use range-for or
   auto-incrementing `for` loops when `ReleaseAt` may be called in the body.

---

## 6. File Inventory

### 6.1 DenseSlotMap — All Touching Files

| File | Role | Key Lines |
|---|---|---|
| `src/engine/lib/dense_slotmap.hpp` | Definition | 1–147 |
| `src/world/game_data.hpp` | Ownership: `m_towers`, `m_enemies` | 28–29 |
| `src/world/game_data.cpp` | Construction, reset, save deserialization | 30–31, 74, 79, 85 |
| `src/world/tile.hpp` | Key storage: `Tile::m_towerKey` | 36 |
| `src/world/attack.hpp` | Key storage: `AttackPayload::m_targetKeys` | 17 |
| `src/world/effect.hpp` | Key storage: `Effect::m_emitter` | 23 |
| `src/engine/systems/particle_system.hpp` | Ownership: `m_emitters`; handle alias | 63, 92–93 |
| `src/engine/systems/particle_system.cpp` | Emitter insert/update/remove/expire | 75–115 |
| `src/systems/world_system.hpp` | Parameter type: `RemoveEnemy` | 17 |
| `src/systems/world_system.cpp` | Tower insert/remove, enemy insert, deferred erase | 21, 32–38, 73, 76–153 |
| `src/systems/tower_system.hpp` | Parameter and return types | 14–16, 21, 25 |
| `src/systems/tower_system.cpp` | Tower/enemy iteration, targeting, fire, tick attacks | 15–204 |
| `src/systems/enemy_system.hpp` | Parameter types | 10–11 |
| `src/systems/enemy_system.cpp` | Enemy iteration, emitter handle check | 6, 34, 60, 89 |
| `src/systems/render_system.hpp` | Parameter types | 18–23 |
| `src/systems/render_system.cpp` | Read-only iteration for drawing | 67, 97–115, 124, 157 |
| `src/states/play_state.hpp` | Selection context: `towerKey` | 57 |
| `src/states/play_state.cpp` | Selection tracking, tower lookups | 135, 171, 225, 247, 254, 260, 280, 285 |
| `src/states/particle_editor_state.hpp` | Preview emitter handle | 101 |
| `src/states/particle_editor_state.cpp` | Emitter state management | 431, 473, 480, 482 |
| `src/world/save_serialization.hpp` | JSON serialization declarations | 75–111 |
| `src/world/save_serialization.cpp` | Deserialization with integrity checks | 6–97 |

### 6.2 Grid2D — All Touching Files

| File | Role | Key Lines |
|---|---|---|
| `src/engine/lib/grid2d.hpp` | Definition | 1–94 |
| `src/world/map.hpp` | Ownership: `m_grid`, `m_pathMesh` | 47, 49 |
| `src/world/map.cpp` | Map creation, tile access, path mesh construction | 20, 24, 29–31, 41–43, 48, 55–58, 73–122 |
| `src/systems/pathfinder.hpp` | Type alias `WalkableMask`, `Solve` return type | 17, 24 |
| `src/systems/pathfinder.cpp` | Internal `Graph::m_adj`, `BfsSolve` | 23, 27–102 |
| `src/world/game_data.cpp` | Grid deserialization from save | 66 |
| `src/world/save_serialization.hpp` | Generic JSON serialization templates | 58–73 |

### 6.3 ObjectPool — All Touching Files

| File | Role | Key Lines |
|---|---|---|
| `src/engine/lib/object_pool.hpp` | Definition | 1–63 |
| `src/engine/systems/particle_system.hpp` | Ownership: `m_particles`; capacity constant | 85, 91 |
| `src/engine/systems/particle_system.cpp` | Acquire, swap-and-pop release, clear | 42–44, 118–139, 143, 153 |
| `src/game.hpp` | Owning class: `Game::m_particles` | 81 |

---

## 7. Methodology

- **Static analysis:** Grep across all `src/` files for `DenseSlotMap`, `Grid2D`, and
  `ObjectPool`, plus nested types (`::Key`, `::INVALID_KEY`, `::Slot`, `EmitterHandle`,
  `WalkableMask`).
- **Manual review:** Every call site inspected for pattern correctness — deferred erase,
  swap-and-pop iteration, bounds checks, null pointer guards, INVALID_KEY sentinel
  discipline.
- **Serialization trace:** Save/load paths traced through `save_serialization.hpp` and
  `save_serialization.cpp` for both `DenseSlotMap<Tower>` and `Grid2D<Tile>`.
