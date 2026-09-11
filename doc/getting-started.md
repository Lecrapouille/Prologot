# Getting started with Prologot

This guide starts from GDScript. Rules stay in Prolog files; queries are objects.

## 1. Minimal example

```gdscript
var p = Prologot.new()
if not p.initialize():
    push_error(p.get_last_error())
    return

p.consult_file("res://rules.pl")

var parent = p.predicate("parent", 2)
var child = p.variable()

if p.succeeds(parent.bind("tom", "ann")):
    print("ground check")

for solution in p.solve(parent.bind("tom", child)):
    print(solution.get(child))
```

`bind()` cannot be written `parent("tom", child)` in GDScript. `if []:` is true, so use `succeeds()` for a yes/no test. Use `solution.get(child)`, not `solution[child]`.

## 2. Variables

A `String` is always an atom. Only `p.variable()` creates a variable.

```gdscript
var child = p.variable("Child")   # optional debug name
var anon = p.anonymous()          # fresh `_`, omitted from solutions
```

Reuse the same object to share a variable across a conjunction:

```gdscript
var via = p.variable()
var chain = parent.bind("tom", via).conjunction(parent.bind(via, child))
```

## 3. Lists

A Godot `Array` is a Prolog list.

```gdscript
p.call_predicate("member", [2, [1, 2, 3]])
var items = p.variable()
var solution = p.solve_one(p.predicate("nums", 1).bind(items))
print(solution.get(items))  # [1, 2, 3]
```

## 4. Facts

Prefer a goal. `add_fact("parent(tom, bob)")` remains as a legacy string API.

```gdscript
var parent = p.predicate("parent", 2)
p.assert_fact(parent.bind("tom", "bob"))
p.retract_fact(parent.bind("tom", "bob"))
p.retract_all(parent.bind("tom", p.anonymous()))
```

## 5. Rules

Write rules in a `.pl` file and load them:

```prolog
grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
```

```gdscript
p.consult_file("res://family.pl")
p.consult_string("enemy(goblin, 10).")
```

## 6. Several solutions

```gdscript
for solution in p.solve(parent.bind("tom", child)):
    print(solution.get(child))

var first = p.solve_one(parent.bind("tom", child))  # PrologSolution or null
```

## 7. Atoms and strings

| GDScript | Prolog |
|----------|--------|
| `"tom"` or `p.atom("tom")` | atom `tom` |
| `p.string("tom")` | string `"tom"` |

A bound atom comes back as a Godot `String`. A bound Prolog string comes back as a `PrologTerm` with `is_string()`.

## 8. Godot objects

```gdscript
var player = p.object($Player)
var at = p.predicate("at", 2)
p.assert_fact(at.bind(player, "zone_1"))
p.succeeds(at.bind($Player, "zone_1"))  # same instance id
```

The handle stores the Godot instance id (SWI blob). If the node is freed, `player.is_valid()` is false. Use this from the main thread only.

## 9. Expose a Node or Resource to Prolog

Nothing of the Godot API is exported automatically. You list each property or method:

```gdscript
p.expose_property("Node", "name", "node_name")
p.expose_property("Node2D", "position")          # position/2, Vector2 → [x, y]
p.expose_method("Object", "get_class", "godot_class")

var who = p.variable()
if p.succeeds(p.predicate("node_name", 2).bind($Player, "Hero")):
    print("this node is Hero")
for solution in p.solve(p.predicate("godot_class", 2).bind($Player, who)):
    print(solution.get(who))
```

`node_name(Obj, Val)` is relational: an unbound `Val` reads the property; a ground `Val` succeeds only if it matches. There is no implicit setter. The first argument is a class filter (`Object.is_class()`); a `Node` will not satisfy a `Node2D` property.

Do not expose as `name/2` — that functor is already used by SWI-Prolog.

### Scene node and knowledge Resource

Drop a **PrologotNode** in the scene (Create Node) and assign a **PrologKnowledge** Resource (files + optional inline clauses). At runtime the node starts the engine (or reuses `PrologotEngine`) and consults that knowledge. The Resource is inspectable in the Inspector; the editor dock lists exposed members under **Exposed Godot members**.

```gdscript
# PrologKnowledge on the node, or:
$PrologotNode.expose_property("Node", "name", "node_name")
```

## Next

- [API reference](API.md)
- [Use cases](use-cases.md)
- Editor console: type `parent(tom, X).` and read `X = bob` (Up/Down for history)
