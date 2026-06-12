# Tower TOML Schema

All towers are defined in `data/towers.toml` as a TOML array of tables under `[[towers]]`.

## Top-level fields

| Field         | Type   | Required | Description |
|---------------|--------|----------|-------------|
| `name`        | string | yes      | Unique identifier used in code and factory lookup |
| `description` | string | yes      | Shown in the info panel |
| `cost`        | int    | yes      | Gold cost to place |
| `modules`     | array  | yes      | The tower's behavior modules — see below |
| `presentation`| object | yes      | Sprite + attack presentation (incl. `texture`) — see below |

The sprite key lives inside the `presentation` block (mirroring `enemies.md`), not as a top-level field.

There is no role field. A tower's behavior is defined entirely by its `modules`: a tower with an
`Attack` module shoots; one with only a `Passive` module is a non-attacking wall (its `presentation`
block then carries only a `texture`, and it has no `upgrades`).

---

## `modules` array

Each entry is one module, identified by its `type`. The combat-defining modules are `Attack`
and `Passive`; the rest are status-effect modules. All fields inside an entry are module-specific.

### Attack

```toml
{ type = "Attack", damage = 2, shotsPerMinute = 180, range = 64, targetCount = 1, targetingMode = "First" }
```

The "shooter" module — owns the tower's core combat stats. A tower needs exactly one to attack.
All fields are optional and default to 0 / `"First"`.

| Field           | Type   | Description |
|-----------------|--------|-------------|
| `damage`        | float  | Damage per shot |
| `shotsPerMinute`| float  | Attack cadence; cooldown = 60 / shotsPerMinute |
| `range`         | float  | Radius in world units |
| `targetCount`   | int    | Max simultaneous targets; 0 = all in range |
| `targetingMode` | string | Priority rule — see targeting modes below |

**Targeting modes** — the `targetingMode` value selects which in-range enemy a tower prioritizes
(when `targetCount > 1`, the top N by this rule). The in-game label shown when cycling a tower's mode
is in parentheses.

| `targetingMode`         | Label      | Picks the enemy with… |
|-------------------------|------------|-----------------------|
| `First`                 | First      | the most path progress (closest to the core) — the default |
| `Last`                  | Last       | the least path progress (just spawned) |
| `MostHealth`            | Most HP    | the highest current health |
| `LowestHealth`          | Least HP   | the lowest current health (ignores shields) |
| `Fastest`               | Fastest    | the highest live speed |
| `Slowest`               | Slowest    | the lowest live speed |
| `MostArmor`             | Most Armor | the highest live armor |
| `MostShield`            | Most Shield| the largest total shield pool |
| `LowestEffectiveHealth` | Least Eff HP | the least health **+ shield** (effective HP) — fewest shots to kill |
| `MostEffectiveHealth`   | Most Eff HP  | the most health **+ shield** (effective HP) — the true tank |
| `MostMaxHealth`         | Most Max HP     | the highest **max** health — a stable lock that doesn't re-target as HP drops |
| `MostReward`            | Most Gold       | the highest gold reward |
| `MostLives`             | Most Lives      | the highest `livesOnReach` — the biggest core threat on leak |
| `Closest`               | Closest    | the smallest distance to this tower |
| `Farthest`              | Farthest   | the greatest distance to this tower (within range) |

Unknown or omitted values fall back to `First`.

### Passive

```toml
{ type = "Passive" }
```

Marks the tower as a non-attacking blocker (a wall). Carries no fields.

### ArmorPierce

```toml
{ type = "ArmorPierce", armorPierce = 5.0 }
```

Flat armor penetration — ignores up to `armorPierce` of the target's armor before damage reduction.
Composes onto any `Attack` tower.

| Field         | Type  | Description |
|---------------|-------|-------------|
| `armorPierce` | float | Flat armor value ignored before damage reduction |

**Chip-damage floor:** armor can never fully nullify a shot. When a target's (post-pierce) armor meets
or exceeds the hit's damage, the hit still lands `min(damage, 1.0)` rather than 0. Armor pierce simply
recovers more of the shot's damage before that floor is reached.

---

## Status-effect modules

The remaining module types (`Slow`, `Burn`, `ArmorShred`, `Weakness`, `Stun`, `RampUp`, or
`Crit`) sit alongside the `Attack` module in the same `modules` array. All fields inside each
entry are module-specific. Each numeric build key is the same name as the `add`/`mul` upgrade key
for that stat (e.g. `burnDamage`, `slowDuration`) — one canonical name per stat.

**Effect rules** (apply to every status effect a module inflicts):
- Effects never stack. Reapplying refreshes the timer (and value) only when the new effect is
  **equal or stronger** (by its magnitude; for Stun the magnitude is its duration).
- Every effect expires after its `duration`.
- `ArmorShred` floors armor at 0. `Stun` and `Weakness` are also cleared the moment the enemy
  is next hit (Stun frees movement; Weakness adds its bonus damage to that hit, then is gone).

### Slow
```toml
{ type = "Slow", slowPercent = 50, slowDuration = 2.0, effect = "slow_effect" }
```
| Field          | Type   | Description |
|----------------|--------|-------------|
| `slowPercent`  | float  | Slow strength as a percent (90 = 90% slower) |
| `slowDuration` | float  | Seconds the slow lasts |
| `effect`       | string | Emitter preset name for the on-enemy particle effect |

### Burn
```toml
{ type = "Burn", burnDamage = 0.5, burnDuration = 8.0, effect = "burn_effect" }
```
| Field          | Type   | Description |
|----------------|--------|-------------|
| `burnDamage`   | float  | Damage per second while burning |
| `burnDuration` | float  | Seconds the burn lasts |
| `effect`       | string | Emitter preset name for the on-enemy particle effect |

### ArmorShred
```toml
{ type = "ArmorShred", shredAmount = 2, shredDuration = 4.0, effect = "shred_effect" }
```
| Field           | Type   | Description |
|-----------------|--------|-------------|
| `shredAmount`   | float  | Flat armor removed while active (enemy armor floored at 0) |
| `shredDuration` | float  | Seconds the shred lasts |
| `effect`        | string | Emitter preset name for the on-enemy particle effect |

### Weakness
```toml
{ type = "Weakness", weaknessAmount = 8, weaknessDuration = 5.0, effect = "weakness_effect" }
```
| Field              | Type   | Description |
|--------------------|--------|-------------|
| `weaknessAmount`   | float  | Flat bonus damage the next hit deals; consumed on that hit |
| `weaknessDuration` | float  | Seconds the weakness lasts if not consumed |
| `effect`           | string | Emitter preset name for the on-enemy particle effect |

### Stun
```toml
{ type = "Stun", stunDuration = 1.0, effect = "stun_effect" }
```
| Field          | Type   | Description |
|----------------|--------|-------------|
| `stunDuration` | float  | Seconds the enemy can't move; also cleared the moment it's next hit |
| `effect`       | string | Emitter preset name for the on-enemy particle effect |

### RampUp
```toml
{ type = "RampUp", bonusPerStack = 18, maxStacks = 8, idleTime = 1.2 }
```
A self-buff on the tower (no enemy effect). Each shot adds a stack up to `maxStacks`; each stack
raises `shotsPerMinute` by `bonusPerStack`. All stacks clear after `idleTime` seconds of not firing.
Pair with a `mul = { shotsPerMinute = 0.5 }` on the same upgrade for a slow-start, ramping feel.

| Field           | Type  | Description |
|-----------------|-------|-------------|
| `bonusPerStack` | float | Flat shotsPerMinute added per stack |
| `maxStacks`     | int   | Stack cap |
| `idleTime`      | float | Seconds without firing before all stacks are lost |

### Crit
```toml
{ type = "Crit", critChance = 0.2, critMultiplier = 4.0 }
```
Supplies the attack's crit chance and multiplier (the roll happens during damage resolution).
No enemy effect. Pair with a `critImpact` visual for an on-crit particle burst.

| Field           | Type  | Description |
|-----------------|-------|-------------|
| `critChance`    | float | Probability of a critical hit (0.0–1.0) |
| `critMultiplier`| float | Damage multiplier on crit (default 1.0) |

---

## `presentation` block

Holds the sprite key, the attack sound, plus beam/ring appearance and particle bursts. Only
`texture` affects what is drawn; the rest is presentation (visuals and audio) and does not affect
gameplay.

| Field           | Type        | Description |
|-----------------|-------------|-------------|
| `texture`       | string      | Resource key for the tower sprite (required) |
| `attackSound`   | string      | Resource key for the sound played on each shot; omit for a silent tower |
| `style`         | string      | `"Line"` (beam to each target) or `"Ring"` (area pulse) |
| `attackDuration`| float       | Seconds the beam/ring is visible; also controls muzzle-flash fade |
| `color`         | [R,G,B,A]   | RGBA color of the beam or ring (0–255 per channel) |
| `muzzle`        | string      | Emitter preset fired at the tower origin on attack |
| `impact`        | string      | Emitter preset fired at each target on hit |
| `critImpact`    | string      | Additional emitter preset fired on critical hits |

Sound keys refer to audio files auto-loaded from `resources/sounds/` at startup; the key is the
filename without its extension (e.g. `resources/sounds/sniper.wav` → `"sniper"`). Supported
formats: `.wav`, `.ogg`, `.mp3`, `.flac`.

---

## `upgrades` array

Each entry is one purchasable upgrade level, applied in order (L1 first, then L2, etc.).

```toml
{ cost = 90, add = { ... }, mul = { ... }, modules = [ ... ] }
```

| Field     | Type   | Description |
|-----------|--------|-------------|
| `cost`    | int    | Gold required to purchase |
| `add`     | object | Additive deltas — key→value pairs |
| `mul`     | object | Multiplicative factors — key→value pairs |
| `modules` | array  | New status-effect modules to append (same schema as entries in the top-level `modules` array). |

All three optional fields are independent and can appear together in one upgrade level.

### `add` / `mul` keys

Every key is broadcast to all of the tower's modules; each applies only the keys it recognizes.

**Attack module fields** (the tower's core combat stats):

| Key             | Affects |
|-----------------|---------|
| `damage`        | Damage per shot |
| `shotsPerMinute`| Attack cadence |
| `range`         | Attack radius |
| `targetCount`   | Max simultaneous targets |

**Module parameters** (handled by the matching module; ignored by modules that don't handle the key):

| Key                | Module           | Affects |
|--------------------|------------------|---------|
| `armorPierce`      | ArmorPierceModule | Flat armor ignored before damage reduction |
| `slowPercent`      | SlowModule       | Slow strength % (higher = stronger slow) |
| `slowDuration`     | SlowModule       | Seconds the slow lasts |
| `burnDamage`       | BurnModule       | Damage per second while burning |
| `burnDuration`     | BurnModule       | Seconds the burn lasts |
| `shredAmount`      | ArmorShredModule | Flat armor removed |
| `shredDuration`    | ArmorShredModule | Seconds the shred lasts |
| `weaknessAmount`   | WeaknessModule   | Flat bonus damage on the next hit |
| `weaknessDuration` | WeaknessModule   | Seconds the weakness lasts |
| `stunDuration`     | StunModule       | Seconds the stun lasts |
| `bonusPerStack`    | RampUpModule     | shotsPerMinute added per stack |
| `maxStacks`        | RampUpModule     | Stack cap |
| `idleTime`         | RampUpModule     | Seconds idle before stacks clear |
| `critChance`       | CritModule       | Critical hit probability (requires a Crit module on the tower) |
| `critMultiplier`   | CritModule       | Critical damage multiplier (requires a Crit module on the tower) |

---

## Enemy upgrade tiers & the wave preview

Enemies reuse this same `add`/`mul` upgrade mechanism (see `enemies.md`), but they are not bought
individually — `WaveManager` applies the wave's **upgrade tier** to every enemy. To keep the preview
and the spawned enemies in sync, the tier is pre-applied to a small **prototype pool**: one fully
upgraded `Enemy` template per enemy type in the upcoming wave, rebuilt on each wave transition.

- The spawn path **clones** enemies from these prototypes instead of re-running the upgrade routine
  per spawn.
- The **WaveHUD** reads the same prototypes, so the card it shows for each enemy type displays the
  fully upgraded stats (`maxHealth`, `speed`, `armor`, `shield`, `regenRate`, `splitCount`) and the
  current upgrade **level** — i.e. the preview already reflects the active tier.
