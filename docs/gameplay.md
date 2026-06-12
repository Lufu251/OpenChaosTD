# gameplay.toml Schema

`data/gameplay.toml` sets the core economic and pacing constants for a datapack. All four fields
are top-level scalars; omitting any field leaves the built-in default in effect.

| Field             | Type  | Default | Description |
|-------------------|-------|---------|-------------|
| `startingLives`   | int   | 20      | Lives the player begins with; reaching 0 ends the game |
| `startingGold`    | int   | 150     | Gold the player begins with |
| `sellRefundRate`  | float | 0.5     | Fraction of a tower's total cost returned when selling (0.5 = 50%) |
| `autoSpawnDelay`  | float | 3.0     | Seconds after a wave ends before the next one auto-starts |

`startingLives` and `startingGold` are applied at game start and on every restart. They are not
re-read mid-run; modifying the file while a game is in progress has no effect until the next start.

`sellRefundRate` is applied to a tower's **total invested cost** — the base placement cost plus
the cumulative cost of all purchased upgrades. A rate of `0.0` gives no refund; `1.0` gives a
full refund. Values outside `[0.0, 1.0]` are not clamped by the engine.

`autoSpawnDelay` only applies while auto-spawn is active (toggled in-game). Setting it to `0.0`
makes waves start back-to-back the instant the previous one clears.

## Example

```toml
startingLives  = 20
startingGold   = 1000
sellRefundRate = 0.5
autoSpawnDelay = 3.0
```
