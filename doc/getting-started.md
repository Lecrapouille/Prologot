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

## Next

- [API reference](API.md)
- [Use cases](use-cases.md)
- Editor console: type `parent(tom, X).` and read `X = bob` (Up/Down for history)
