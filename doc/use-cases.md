# Use cases

Recipes for **applying** Prologot in a game. This page assumes you have read [Getting started](getting-started.md) §1–§7 (pipeline, variables, lazy `solve()`).

| Level | Document |
|-------|----------|
| Concepts & first program | [Getting started](getting-started.md) |
| SWI-Prolog ↔ Prologot mapping | [Prolog developers](prolog-developers.md) |
| Method signatures & types | [API reference](API.md) |

Runnable game with nodes as Prolog terms: [demos/mini_dungeon](../demos/mini_dungeon/README.md). Script-only sample: [hello_world_prologot.gd](hello_world_prologot.gd).

---

## How to read each recipe

Every example follows the same shape:

1. **Load** — `consult_file` / `consult_string` (Prolog rules).
2. **Build** — `predicate(...).call(...)` (GDScript goal).
3. **Solve** — `has_solution()` / `first()` / `for … in` / `all()` (see [Getting started §4](getting-started.md#4-the-query-pipeline-and-solve)).
4. **Act** — use bindings in your game code (`sol.get(var)`).

Examples use the `PrologotEngine` autoload when the plugin is enabled. Replace with a local `Prologot.new()` instance if you prefer — the API is identical.

---

## Scene setup

Minimal integration without repeating the full tutorial.

### Autoload (global engine)

Enable the plugin → `PrologotEngine` autoload is available in every scene.

```gdscript
func _ready() -> void:
    PrologotEngine.consult_file("res://ai/rules.pl")
```

### PrologotNode (per-scene knowledge)

1. Add a **PrologotNode** to the scene.
2. Assign a **PrologKnowledge** Resource (`.pl` paths + optional inline code).
3. Query from siblings:

```gdscript
@onready var pl = $Prologot

func _ready() -> void:
    var hero = pl.predicate("hero")
    if pl.solve(hero.call($Player)).has_solution():
        print("Player matches hero/1 rule")
```

### Multiple knowledge bases (modes / levels)

The singleton can store named bases and switch between them:

```gdscript
PrologotEngine.create_knowledge_base("combat", """
    decide(attack, _, H) :- H > 30, !.
    decide(flee, _, H) :- H < 20, !.
    decide(patrol, _, _).
""")

PrologotEngine.create_knowledge_base("dialogue", """
    line(guard, "Halt!").
""")

PrologotEngine.switch_knowledge_base("combat")
# ... later ...
PrologotEngine.switch_knowledge_base("dialogue")
```

See [API — PrologotEngine](API.md#prologotengine-singleton-autoload) for `list_knowledge_bases()`.

---

## AI decision making

**When to use Prolog:** many rules, priority, easy to extend without recompiling GDScript.

**Pattern:** encode conditions in Prolog; GDScript passes **ground numbers** and reads one **action atom**.

```gdscript
PrologotEngine.consult_string("""
    should_attack(D, H) :- D < 5, H > 30.
    should_flee(H) :- H < 20.

    decide(attack, D, H) :- should_attack(D, H), !.
    decide(flee, _, H) :- should_flee(H), !.
    decide(patrol, _, _).
""")

var decide = PrologotEngine.predicate("decide")
var action = PrologotEngine.variable("Action")
var sol = PrologotEngine.solve(
    decide.call(action, player_distance, enemy_health)
).first()

match sol.get(action):
    "attack": _start_attack()
    "flee": _start_flee()
    _: _patrol()
```

**Tip:** use cut (`!`) in Prolog for deterministic priority; keep numeric state in GDScript variables passed into `call()`.

---

## Dialogue systems

**When to use Prolog:** branching lines keyed on NPC id, mood, and world facts.

```gdscript
PrologotEngine.consult_string("""
    dialogue(guard, greeting, "Halt! Who goes there?").
    dialogue(guard, friendly, "Welcome, friend.") :- reputation(player, good).
    dialogue(guard, hostile, "You're not welcome here.") :- reputation(player, bad).
""")

var dialogue = PrologotEngine.predicate("dialogue")
var line = PrologotEngine.variable("Line")
var sol = PrologotEngine.solve(
    dialogue.call("guard", "greeting", line)
).first()
show_line(sol.get(line))
```

Add facts at runtime when reputation changes:

```gdscript
var rep = PrologotEngine.predicate("reputation")
PrologotEngine.retract_all(rep.call(PrologotEngine.anonymous(), PrologotEngine.anonymous()))
PrologotEngine.assert_fact(rep.call("player", "good"))
```

---

## Rule-based crafting & inventory

**When to use Prolog:** declarative “can craft if …” rules; multiple ingredients.

```gdscript
PrologotEngine.consult_string("""
    can_craft(iron_sword) :- has_item(iron, 3), has_item(wood, 1).
    can_craft(health_potion) :- has_item(herb, 2), has_item(water, 1).
""")

var has_item = PrologotEngine.predicate("has_item")
PrologotEngine.assert_fact(has_item.call("iron", 5))
PrologotEngine.assert_fact(has_item.call("wood", 2))

var can_craft = PrologotEngine.predicate("can_craft")
if PrologotEngine.solve(can_craft.call("iron_sword")).has_solution():
    _craft("iron_sword")
    _consume_materials({"iron": 3, "wood": 1})
```

Sync inventory by retracting old `has_item/2` facts and asserting new counts after each change.

---

## Pathfinding & graphs

**When to use Prolog:** relation `edge/3`, cycle-safe `path/4`, multiple routes.

Keep the graph in a `.pl` file for large maps:

```gdscript
PrologotEngine.consult_file("res://ai/pathfinding.pl")
```

Query:

```gdscript
var path_pred = PrologotEngine.predicate("path")
var route = PrologotEngine.variable("Route")
var cost = PrologotEngine.variable("Cost")

for solution in PrologotEngine.solve(
    path_pred.call("a", "f", route, cost)
):
    print(solution.get(route), " cost=", solution.get(cost))
```

Demo reference: [05_pathfinding.pl](../demos/showcases/examples/05_pathfinding.pl).

---

## State machines

**When to use Prolog:** explicit `transition/2` rules driven by sensors you assert each frame.

```gdscript
PrologotEngine.consult_string("""
    transition(idle, patrol) :- health > 50.
    transition(patrol, attack) :- enemy_nearby, health > 30.
    transition(attack, flee) :- health < 20.
""")

# Each tick: assert sensor facts, query, retract sensors
PrologotEngine.assert_fact(PrologotEngine.predicate("enemy_nearby").call())
var trans = PrologotEngine.predicate("transition")
var next_st = PrologotEngine.variable("Next")
var sol = PrologotEngine.solve(trans.call(current_state, next_st)).first()
if sol != null:
    current_state = sol.get(next_st)
```

Demo reference: [06_ai_behavior.pl](../demos/showcases/examples/06_ai_behavior.pl).

---

## Godot objects in the knowledge base

**When to use Prolog:** “which entity is in zone X?” without stringly-typed paths.

```gdscript
var at = PrologotEngine.predicate("at")
PrologotEngine.assert_fact(at.call($Player, "zone_1"))

var who = PrologotEngine.variable("Who")
for solution in PrologotEngine.solve(at.call(who, "zone_1")):
    var obj = solution.get(who)
    if obj is PrologObject and obj.is_valid():
        print(obj.get_object())
```

On scene change: `retract_all(at.call(PrologotEngine.anonymous(), PrologotEngine.anonymous()))`.

---

## Reading Godot properties from Prolog

**When to use Prolog:** rules mention node state by name without GDScript glue per rule.

```gdscript
PrologotEngine.expose_property("Node", "name", "node_name")
PrologotEngine.consult_string("""
    hero(X) :- node_name(X, 'Hero').
""")

if PrologotEngine.solve(
    PrologotEngine.predicate("hero").call($Player)
).has_solution():
    print("Player is the Hero node")
```

Expose only what rules need. Details: [Getting started §10](getting-started.md#10-exposing-godot-to-prolog-optional).

---

## Dynamic world state

**When to use Prolog:** many facts that change every turn (locations, sightings).

```gdscript
PrologotEngine.consult_file("res://game_rules.pl")

var loc = PrologotEngine.predicate("player_location")
var spot = PrologotEngine.predicate("enemy_spotted")

PrologotEngine.assert_fact(loc.call("zone_1"))
PrologotEngine.assert_fact(spot.call("goblin", "zone_2"))

var type = PrologotEngine.variable("Type")
var where = PrologotEngine.variable("Where")
for solution in PrologotEngine.solve(spot.call(type, where)):
    _alert(solution.get(type), solution.get(where))

PrologotEngine.retract_fact(loc.call("zone_1"))
PrologotEngine.assert_fact(loc.call("zone_2"))
```

---

## Choosing an integration style

| Situation | Suggested approach |
|-----------|-------------------|
| One global ruleset for the whole game | `PrologotEngine` autoload |
| Rules per level / character | `PrologKnowledge` on `PrologotNode` |
| Combat vs dialogue vs puzzle modes | `create_knowledge_base` / `switch_knowledge_base` |
| Prototyping in editor | Editor console, then port to `solve()` |
| Heavy graph / AI rules | `.pl` files + `consult_file` |

---

## Demos and games in this repo

| Project | Command | Shows |
|---------|---------|-------|
| Mini Dungeon | `make run-mini-dungeon` | Nodes as Prolog terms, dynamic facts, goblin `best_action/3` |
| Showcases | `make run-demo` | Queries, rules, dynamic facts, pathfinding, AI |
| Galactic Customs | `make run-galactic_customs` | Full game loop, taxes, `assert_fact`, `predicate_exists` |

---

## Next step

Look up signatures, initialization options, and type conversion tables in the [API reference](API.md).
