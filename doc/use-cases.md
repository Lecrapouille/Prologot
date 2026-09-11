# Use Cases

Practical examples of using Prologot in game development.

## Quick Start

### Basic Example

The simplest way to get started with Prologot:

```gdscript
extends Node

var prolog: Prologot

func _ready():
  prolog = Prologot.new()

  if not prolog.initialize():
    push_error("Failed to initialize Prologot")
    return

  # Load rules as Prolog source
  prolog.consult_string("""
    parent(tom, bob).
    parent(bob, ann).
    grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
  """)

  var parent = prolog.predicate("parent", 2)
  var grandparent = prolog.predicate("grandparent", 2)
  var ancestor = prolog.variable()
  var child = prolog.variable()

  if prolog.succeeds(grandparent.bind("tom", "ann")):
    print("Tom is Ann's grandparent!")

  for solution in prolog.solve(parent.bind(ancestor, child)):
    print("Parent: ", solution.get(ancestor), " -> ", solution.get(child))

func _exit_tree():
  if prolog:
    prolog.cleanup()
```

See [hello_world_prologot.gd](hello_world_prologot.gd) for a complete working example.

### Using the Global Singleton

When the plugin addons/prologot/prologot_singleton.gd is enabled, you can use the `PrologotEngine` autoload:

```gdscript
extends Node

func _ready():
  PrologotEngine.consult_string("""
    enemy(goblin, 10, low).
    enemy(dragon, 100, high).
    weak_enemy(Name) :- enemy(Name, HP, _), HP < 50.
  """)

  var weak_enemy = PrologotEngine.predicate("weak_enemy", 1)
  if PrologotEngine.succeeds(weak_enemy.bind("goblin")):
    print("Goblin is a weak enemy")

  var enemy = PrologotEngine.predicate("enemy", 3)
  var name = PrologotEngine.variable()
  var hp = PrologotEngine.variable()
  var threat = PrologotEngine.variable()
  for solution in PrologotEngine.solve(enemy.bind(name, hp, threat)):
    print("Enemy: ", solution.get(name), " HP: ", solution.get(hp), " Threat: ", solution.get(threat))
```

## Godot objects in facts

Pass a `Node` or `Resource` without turning it into a name string:

```gdscript
var player = PrologotEngine.object($Player)
var at = PrologotEngine.predicate("at", 2)
PrologotEngine.assert_fact(at.bind(player, "zone_1"))
if PrologotEngine.succeeds(at.bind($Player, "zone_1")):
    print("same instance")
```

If the node is freed, `player.is_valid()` becomes false. Retract stale facts after a scene change.

## Read Node / Resource fields from Prolog

Expose only the members your rules need, then write ordinary Prolog:

```gdscript
PrologotEngine.expose_property("Node", "name", "node_name")
PrologotEngine.consult_string("""
    hero(X) :- node_name(X, 'Hero').
""")
if PrologotEngine.succeeds(PrologotEngine.predicate("hero", 1).bind($Player)):
    print("the player node is named Hero")
```

Or drop a `PrologotNode` in the scene, assign a `PrologKnowledge` Resource, and query from a sibling script:

```gdscript
$Prologot.succeeds($Prologot.predicate("hero", 1).bind($Player))
```

## AI Decision Making

Use Prolog to make intelligent decisions for NPCs based on game state:

```gdscript
PrologotEngine.consult_string("""
    should_attack(Distance, Health) :- Distance < 5, Health > 30.
    should_flee(Health) :- Health < 20.

    decide(attack, Dist, Health) :- should_attack(Dist, Health), !.
    decide(flee, _, Health) :- should_flee(Health), !.
    decide(patrol, _, _).
""")

player_distance = 20
enemy_health = 10
var action = PrologotEngine.call_function("decide", [player_distance, enemy_health])
# Returns: "flee"
```

## Dialogue Systems

Create dynamic dialogue systems that respond to game state:

```gdscript
PrologotEngine.consult_string("""
    dialogue(guard, greeting, "Halt! Who goes there?").
    dialogue(guard, friendly, "Welcome, friend.") :- reputation(player, good).
    dialogue(guard, hostile, "You're not welcome here.") :- reputation(player, bad).
""")

var line = PrologotEngine.call_function("dialogue", ["guard", "greeting"])
# Returns: "Halt! Who goes there?"
```

## Rule-Based Systems

Implement crafting systems, inventory management, and other rule-based mechanics:

```gdscript
PrologotEngine.consult_string("""
    can_craft(iron_sword) :- has_item(iron, 3), has_item(wood, 1).
    can_craft(health_potion) :- has_item(herb, 2), has_item(water, 1).
""")

# Add items to inventory
PrologotEngine.add_fact("has_item(iron, 5)")
PrologotEngine.add_fact("has_item(wood, 2)")
PrologotEngine.add_fact("has_item(herb, 3)")

# Check if we can craft items
var can_craft = PrologotEngine.predicate("can_craft", 1)
if PrologotEngine.succeeds(can_craft.bind("iron_sword")):
    craft_item("iron_sword")
    # Remove used items
    PrologotEngine.retract_fact("has_item(iron, 5)")
    PrologotEngine.add_fact("has_item(iron, 2)")  # 5 - 3 = 2
    PrologotEngine.retract_fact("has_item(wood, 2)")
    PrologotEngine.add_fact("has_item(wood, 1)")  # 2 - 1 = 1
```

## Pathfinding

Use Prolog for pathfinding algorithms:

```gdscript
PrologotEngine.consult_file("res://ai/pathfinding.pl")

var path_pred = PrologotEngine.predicate("path", 4)
var route = PrologotEngine.variable()
var cost = PrologotEngine.variable()
for solution in PrologotEngine.solve(path_pred.bind("a", "f", route, cost)):
    print("Path: ", solution.get(route), " Cost: ", solution.get(cost))
```

## State Machines

Implement AI state machines with Prolog:

```gdscript
PrologotEngine.consult_string("""
    state(idle).
    state(patrol).
    state(attack).
    state(flee).

    transition(idle, patrol) :- health > 50.
    transition(patrol, attack) :- enemy_nearby, health > 30.
    transition(attack, flee) :- health < 20.
    transition(flee, idle) :- safe_distance.
""")

var transition = PrologotEngine.predicate("transition", 2)
var next_state = PrologotEngine.variable()
for solution in PrologotEngine.solve(transition.bind("idle", next_state)):
    print(solution.get(next_state))
```

## Knowledge Base Management

Manage game knowledge dynamically:

```gdscript
# Load initial knowledge base
PrologotEngine.consult_file("res://game_rules.pl")

# Add dynamic facts during gameplay
PrologotEngine.add_fact("player_location(zone_1)")
PrologotEngine.add_fact("enemy_spotted(goblin, zone_2)")

var spotted = PrologotEngine.predicate("enemy_spotted", 2)
var type = PrologotEngine.variable()
var location = PrologotEngine.variable()
for solution in PrologotEngine.solve(spotted.bind(type, location)):
    print(solution.get(type), " at ", solution.get(location))

# Update facts
PrologotEngine.retract_fact("player_location(zone_1)")
PrologotEngine.add_fact("player_location(zone_2)")
```

## Multiple Knowledge Bases

Switch between different knowledge bases for different game modes or scenarios:

```gdscript
# Create knowledge base for combat mode
PrologotEngine.create_knowledge_base("combat", """
    enemy(goblin, 10, melee).
    enemy(archer, 5, ranged).
    should_attack(Distance, Health) :- Distance < 5, Health > 30.
    decide(attack, Dist, Health) :- should_attack(Dist, Health), !.
    decide(flee, _, Health) :- Health < 20, !.
    decide(patrol, _, _).
""")

# Create knowledge base for dialogue mode
PrologotEngine.create_knowledge_base("dialogue", """
    npc(guard, friendly, "Welcome, traveler!").
    npc(merchant, neutral, "What can I sell you?").
    npc(thief, hostile, "Your money or your life!").
    get_greeting(NPC, Reputation, Message) :-
        npc(NPC, Reputation, Message).
""")

# Create knowledge base for puzzle mode
PrologotEngine.create_knowledge_base("puzzle", """
    can_combine(red, blue, purple).
    can_combine(blue, yellow, green).
    can_combine(red, yellow, orange).
    solve_puzzle(Colors, Result) :-
        can_combine(Colors[0], Colors[1], Result).
""")

# List available knowledge bases
var available_bases = PrologotEngine.list_knowledge_bases()
print("Available knowledge bases: ", available_bases)
# Output: ["combat", "dialogue", "puzzle"]

# Switch to combat mode
PrologotEngine.switch_knowledge_base("combat")
var action = PrologotEngine.call_function("decide", [3, 50])
# Returns: "attack"

# Switch to dialogue mode
PrologotEngine.switch_knowledge_base("dialogue")
var greeting = PrologotEngine.call_function("get_greeting", ["guard", "friendly"])
# Returns: "Welcome, traveler!"

# Switch to puzzle mode
PrologotEngine.switch_knowledge_base("puzzle")
var result = PrologotEngine.call_function("solve_puzzle", [["red", "blue"]])
# Returns: "purple"
```

**Use Cases:**

- **Game Modes**: Different AI behaviors for combat, exploration, dialogue
- **Levels/Scenarios**: Different rule sets for different game levels
- **Character Classes**: Different knowledge bases for different character types
- **Dynamic Content**: Switch knowledge bases based on player choices or game state
