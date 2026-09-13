# Mini Dungeon

A short top-down dungeon that shows **Godot nodes as first-class Prolog terms**. You start **unarmed**. Descend four floors, steal the treasure, then **climb back to the surface**.

## Run

```bash
make setup-demos
make run-mini-dungeon
```

## Controls

| Key | Action |
|-----|--------|
| WASD / arrows | Move |
| Space | Melee (sword) or shoot (bow + arrows) |
| Q | Drink a carried potion |
| G | Drop the sword (`can_shoot` needs `\+ holding_sword`) |
| R | Restart after GAME OVER / SUCCESS |

## Enemies (Prolog)

- **Unarmed goblin** — `unarmed/1` → `flee` the player, `seek_weapon` a sword/bow on the floor (`on_floor/1`).
- **Armed goblin** — `armed/1` → `chase` / `attack`.
- **Wizard** — `can_cast/2` → fireballs; `keep_distance` if you get close.

Arrows lie on the ground each floor. Pick them up for the bow.

## Loop

1. Floors 1–3: go **down** (key + door).
2. Floor 4: take the **treasure**.
3. Climb the **UP stairs** on each floor, back to 1.
4. On floor 1, the up stairs lead to the **surface** → **SUCCESS**.

## What it demonstrates

```gdscript
for solution in prolog.solve(prolog.predicate("best_action").call(unit, action, target)):
    print(solution.get(action), " -> ", solution.get(target))
```

Rules live in `prolog/ai.pl`, `rules.pl`, `combat.pl`.
