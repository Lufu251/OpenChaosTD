# Enemy TOML Schema

All enemies are defined in `data/enemies.toml` as a TOML array of tables under `[[enemies]]`.

## Top-level fields

| Field          | Type   | Required | Description |
|----------------|--------|----------|-------------|
| `name`         | string | yes      | Unique identifier used in code and factory lookup |
| `description`  | string | yes      | Shown in the info panel |
| `maxHealth`    | float  | yes      | Starting and maximum hit points |
| `speed`        | float  | yes      | Movement speed in world units per second |
| `reward`       | int    | yes      | Gold granted to the player when the enemy is killed |
| `livesOnReach` | int    | no       | Lives lost if the enemy reaches the core (default 1) |
| `presentation` | object | yes      | Presentation-only data (sprite, death effect) — see below |
| `modules`      | array  | no       | The enemy's defensive and mechanical traits — see below |
| `upgrade`      | object | no       | A single stat-scaling step re-applied once per upgrade tier — see below |

The core stats above (`maxHealth`, `speed`, `reward`, `livesOnReach`) stay top-level but are
parsed into an internal core stats module at build time — the runtime analogue of the tower's
`Attack` module — so their names match the `upgrade` keys exactly.

An enemy with no `modules` is a plain target defined entirely by its core stats (`shade` and
`flicker` are examples). All special behavior — armor, regeneration, shields, splitting, and
status immunities — comes from the `modules` array.

---

## `presentation` object

All presentation-only data (visuals **and** audio) lives in a nested `presentation` object, separate
from the gameplay stats. This mirrors the `presentation` block used by towers (`towers.md`).

```toml
presentation = { texture = "enemy_shade", deathSound = "enemy_death", deathEmitter = "death_small" }
```

| Field          | Type   | Required | Description |
|----------------|--------|----------|-------------|
| `texture`      | string | yes      | Resource key for the enemy sprite |
| `deathSound`   | string | no       | Resource key for the sound played when the enemy dies (defaults to `enemy_death`) |
| `deathEmitter` | string | no       | Emitter preset spawned at the enemy's position on death |

Sound keys refer to audio files auto-loaded from `resources/sounds/` at startup; the key is the
filename without its extension (e.g. `resources/sounds/enemy_death.wav` → `"enemy_death"`).
Supported formats: `.wav`, `.ogg`, `.mp3`, `.flac`.

---

## `modules` array

Each entry is one module, identified by its `type`. All fields inside an entry are
module-specific. The available module types are `Armor`, `Regeneration`, `Shield`, `Split`,
`Immune`, `Resistance`, `Evasion`, `Barrier`, `Enrage`, `ShieldRegen`, `Adrenaline`, and
`Summoner`. An enemy may combine any number of them (the `sovereign` stacks four).

### Armor

```toml
{ type = "Armor", armor = 3.0 }
```

Reduces every incoming hit by a flat amount. The reduction is applied after any tower armor
pierce. Armor can never fully nullify an attack: when it meets or exceeds the hit's damage,
the hit still chips `min(damage, 1.0)` instead of dropping to 0 — so a heavily-armored enemy
always takes at least a sliver of damage and can never be healed by an absorbed attack.

| Field   | Type  | Description |
|---------|-------|-------------|
| `armor` | float | Flat damage subtracted from each incoming hit |

### Regeneration

```toml
{ type = "Regeneration", regenRate = 2.0 }
```

Restores health every frame while the enemy is alive. Healing is capped at the enemy's
maximum health.

| Field       | Type  | Description |
|-------------|-------|-------------|
| `regenRate` | float | Health restored per second |

### Shield

```toml
{ type = "Shield", shield = 15.0 }
```

A depletable damage pool that absorbs incoming damage before any is dealt to health. Each hit
drains the shield first; once it reaches 0 the remaining damage passes through to health. The
shield does not recharge on its own.

| Field    | Type  | Description |
|----------|-------|-------------|
| `shield` | float | Starting and maximum shield pool |

### Split

```toml
{ type = "Split", child = "golem", splitCount = 2, spacing = 12.0 }
```

On death, spawns `splitCount` child enemies of type `child` near the dying enemy's position. The
`child` value must be the `name` of another enemy defined in `enemies.toml`.

To stop the children from stacking into a single indistinguishable blob, they are fanned out
**backward along the path** (away from the core) — each successive child is offset by `spacing`
world units. Pushing them backward only means a split can never skip a child ahead or let it reach
the core early. Set `spacing` to `0` to spawn every child exactly on the death position (the old
stacking behavior).

| Field        | Type   | Description |
|--------------|--------|-------------|
| `child`      | string | `name` of the enemy type to spawn on death |
| `splitCount` | int    | Number of children to spawn |
| `spacing`    | float  | World-unit gap between consecutive children along the path (default `12.0`; `0` = stack) |

### Immune

```toml
{ type = "Immune", effect = "Stun" }
```

Blocks a single status-effect type from ever being applied to the enemy. Towers can still
hit and damage the enemy normally; only the named effect is ignored. Add one module per
effect to grant multiple immunities.

| Field    | Type   | Description |
|----------|--------|-------------|
| `effect` | string | Status effect to ignore — see effect types below |

**Effect types:** `Slow`, `Burn`, `ArmorShred`, `Stun`, `Weakness`

### Resistance

```toml
{ type = "Resistance", resistPercent = 40 }
```

Scales every incoming hit by `(1 - resistPercent/100)` — a **percentage** damage reduction, the
multiplicative complement to `Armor`'s flat subtraction. It composes with armor: armor is subtracted
first, then resistance scales whatever remains. Stack it with `Armor` for an enemy that is tough
against both small and large hits.

| Field           | Type  | Description |
|-----------------|-------|-------------|
| `resistPercent` | float | Percent of each hit's damage ignored (40 = 40% less damage) |

### Evasion

```toml
{ type = "Evasion", dodgeChance = 0.25 }
```

Each incoming hit has a `dodgeChance` probability of being **fully negated** (0 damage). The roll uses
the same global RNG stream as the tower crit roll, so a saved game replays identically.

| Field         | Type  | Description |
|---------------|-------|-------------|
| `dodgeChance` | float | Probability in `[0..1]` that a hit is completely dodged |

### Barrier

```toml
{ type = "Barrier", hitCount = 3 }
```

Fully blocks the next `hitCount` hits **regardless of their magnitude**, then expires and lets damage
through. Unlike `Shield` (which absorbs a damage *pool*), each Barrier charge eats one entire hit — a
single big hit and a single weak hit each consume one charge. Strong against fast, low-damage towers.

| Field      | Type | Description |
|------------|------|-------------|
| `hitCount` | int  | Number of hits fully blocked before the barrier expires |

### Enrage

```toml
{ type = "Enrage", healthThreshold = 0.3, speedBonus = 40 }
```

Once the enemy's current health drops to or below `healthThreshold` (a fraction of max health), it
speeds up by `speedBonus`. A counter to slow-damage strategies that let enemies linger at low health —
a wounded enemy rushes the core.

| Field             | Type  | Description |
|-------------------|-------|-------------|
| `healthThreshold` | float | Fraction of max health at/below which the enemy enrages (0.3 = 30%) |
| `speedBonus`      | float | Flat speed added to live speed while enraged |

### ShieldRegen

```toml
{ type = "ShieldRegen", shield = 20.0, rechargeRate = 5.0, rechargeDelay = 2.0 }
```

A shield pool that **recharges after a damage-free delay**. It absorbs incoming damage like `Shield`,
but every hit resets the recharge timer; once `rechargeDelay` seconds pass without a hit, the pool
refills at `rechargeRate` per second up to its maximum. Rewards focus-fire and punishes trickle damage.
Counts toward the `Most Shield` targeting mode.

| Field           | Type  | Description |
|-----------------|-------|-------------|
| `shield`        | float | Starting and maximum shield pool |
| `rechargeRate`  | float | Shield restored per second once recharging |
| `rechargeDelay` | float | Seconds without taking a hit before the pool starts recharging |

### Adrenaline

```toml
{ type = "Adrenaline", speedBonus = 30, duration = 1.5 }
```

Taking a hit grants a temporary burst of speed: each hit (re)arms a `duration`-second timer during
which the enemy's live speed is raised by `speedBonus`. Sustained fire keeps it permanently sped up.

| Field        | Type  | Description |
|--------------|-------|-------------|
| `speedBonus` | float | Flat speed added to live speed while the burst is active |
| `duration`   | float | Seconds the burst lasts after each hit |

### Summoner

```toml
{ type = "Summoner", child = "flicker", summonCount = 2, interval = 4.0, spacing = 12.0 }
```

Periodically spawns minions **while alive** — every `interval` seconds it spawns `summonCount` children
of type `child` at its current position, fanned backward along the path by `spacing` (same placement
rules as `Split`). The `child` value must be the `name` of another enemy in `enemies.toml`. Unlike
`Split` (which fires once on death), a Summoner keeps producing reinforcements until it is killed.

| Field         | Type   | Description |
|---------------|--------|-------------|
| `child`       | string | `name` of the enemy type to spawn |
| `summonCount` | int    | Number of children spawned each interval (default `1`) |
| `interval`    | float  | Seconds between summons (default `5.0`) |
| `spacing`     | float  | World-unit gap between consecutive children along the path (default `12.0`; `0` = stack) |

> **Wave-tier scaling:** children spawned by both `Split` and `Summoner` are scaled to the **current
> wave's upgrade tier** (see the `upgrade` section), so late-wave reinforcements stay as threatening as
> the enemies the wave spawns directly.

---

## `upgrade` object

An optional single upgrade step that scales an enemy beyond its base definition (e.g. for
elite or late-wave variants). Applying it broadcasts its deltas through the enemy's
stat-patching pipeline and appends any new modules.

```toml
upgrade = { add = { armor = 2, regenRate = 2 }, mul = { maxHealth = 1.5 } }
```

| Field     | Type   | Description |
|-----------|--------|-------------|
| `add`     | object | Map of stat key → **flat** delta added to the current value |
| `mul`     | object | Map of stat key → **multiplier** applied to the current value |
| `modules` | array  | Additional module entries appended to the enemy **once**, regardless of tier (the `add`/`mul` deltas stack per tier, but modules are added a single time). |

### How the upgrade scales over tiers

Waves carry an **upgrade tier** (configured in `data/waves.toml` via `upgrade_interval` — the tier
increments every Nth wave). The wave manager applies this one upgrade definition **once per tier**:
tier 1 applies it once, tier 2 applies it twice, and so on, with no upper bound (endless mode keeps
stacking). Because the deltas are re-applied on top of the already-upgraded stats, the effect
compounds — `add` accumulates linearly while `mul` compounds exponentially. For example, the
`add = { armor = 2 }`, `mul = { maxHealth = 1.5 }` step above yields at tier *n*: `+2n` armor and
`maxHealth × 1.5ⁿ`. An enemy with no `upgrade` key never scales and ignores the wave tier entirely.

`add` and `mul` accept the same keys, routed to either the enemy's base stats or the matching
module:

| Key            | Target | Notes |
|----------------|--------|-------|
| `maxHealth`    | core   | Also refills current health, so a scaled enemy spawns at full HP |
| `speed`        | core   | Recomputed into the live stat on the next tick |
| `reward`       | core   | Rounded to the nearest integer |
| `livesOnReach` | core   | Rounded to the nearest integer |
| `armor`           | module | Requires an `Armor` module |
| `regenRate`       | module | Requires a `Regeneration` module |
| `shield`          | module | Requires a `Shield` or `ShieldRegen` module; also tops up the live shield pool |
| `splitCount`      | module | Requires a `Split` module; rounded to the nearest integer |
| `resistPercent`   | module | Requires a `Resistance` module |
| `dodgeChance`     | module | Requires an `Evasion` module |
| `hitCount`        | module | Requires a `Barrier` module; rounded to the nearest integer; also refills the charges |
| `healthThreshold` | module | Requires an `Enrage` module |
| `speedBonus`      | module | Requires an `Enrage` or `Adrenaline` module |
| `rechargeRate`    | module | Requires a `ShieldRegen` module |
| `rechargeDelay`   | module | Requires a `ShieldRegen` module |
| `duration`        | module | Requires an `Adrenaline` module |
| `summonCount`     | module | Requires a `Summoner` module; rounded to the nearest integer |
| `interval`        | module | Requires a `Summoner` module |

The core stats (`maxHealth`/`speed`/`reward`/`livesOnReach`) route to the enemy's core stats module;
the rest route to the matching trait module. A key with no matching field or module is silently ignored.

---

## Damage resolution order

When a tower hits an enemy, damage is resolved in this order:

1. **Armor** subtracts its `armor` value (after the attacker's armor pierce). If that leaves the hit at
   0 or below, the hit instead deals `min(damage, 1.0)` — armor never zeroes out (or heals from) an attack.
2. **Weakness** (a tower-applied status effect) adds its flat bonus on top, if present.
3. **Interception chain** — every module that intercepts damage gets a pass at the remaining amount,
   **in the order the modules are declared** in the `modules` array. This includes `Resistance`
   (scales it down), `Evasion` (may negate it entirely), `Barrier` (consumes a charge to negate it),
   `Shield` and `ShieldRegen` (absorb from their pools), and `Adrenaline` (lets it through but arms its
   speed burst). Declaration order matters: putting `Evasion` or `Barrier` **before** `Shield` means a
   dodged or blocked hit never touches the shield pool, whereas the reverse order drains the shield first.
4. Whatever is left is deducted from the enemy's health.

`Regeneration` and `ShieldRegen` run independently each frame, `Enrage`/`Adrenaline` adjust live speed,
and `Immune` simply prevents matching status effects from being applied at all.
