# Getting started with Prologot

This guide is the **first stop** after [installation](installation.md). It explains what Prologot is, how the pieces fit together, and walks you through a working program step by step.

| If you are… | Read next |
|-------------|-----------|
| New to Prolog and Godot | Stay here; if *atom / term / goal* are unclear, read the [glossary](glossary.md) first |
| Already fluent in Prolog | Skim §2, then read [Notes for Prolog developers](prolog-developers.md) |
| Building a game feature | [Use cases](use-cases.md) once you understand §5–§7 |
| Looking up one method | [API reference](API.md) |

---

## 1. What Prologot does

Prologot embeds **SWI-Prolog** inside Godot 4. You write **rules and facts in Prolog** (`.pl` files or strings), and you **query from GDScript** using objects — not Prolog source strings.

Typical uses:

- NPC decision rules (“attack / flee / patrol”)
- Dialogue and quest logic
- Crafting, inventory, rule engines
- Pathfinding and graph search

Two worlds, one pipeline:

```mermaid
flowchart LR
  subgraph prolog_side [Prolog — knowledge]
    pl[".pl files / consult_string"]
    kb["Facts and rules in SWI"]
    pl --> kb
  end
  subgraph gdscript_side [GDScript — queries]
    pred["predicate(name)"]
    call["call(args...)"]
    goal["PrologGoal"]
    solve["solve(goal)"]
    query["PrologQuery"]
    pred --> call --> goal --> solve --> query
  end
  kb --> solve
  query --> sol["PrologSolution.get(variable)"]
```

**Rules stay in Prolog.** **Queries are built in GDScript.** That split is intentional: your game logic remains readable Prolog; your engine code stays typed GDScript.

---

## 2. Core ideas (read this once)

Atom, term, fact, rule, predicate, goal, `,` / `;` / cut: [glossary](glossary.md). These rules explain most “why doesn’t my query work?” moments.

### Queries are objects, not strings

There is no public `query("parent(tom, X)")` API. You build a goal, then solve it:

```gdscript
var parent = p.predicate("parent")
var child = p.variable("Child")
var goal = parent.call("tom", child)
var query = p.solve(goal)
```

The editor console *does* accept typed Prolog text — that path is internal (`_editor_query`). Game code uses `solve()`.

### `call()`, not `parent("tom", child)`

GDScript cannot call a stored predicate like a function. Use `parent.call("tom", child)` or `parent.callv(["tom", child])`.

### A `PrologQuery` is always “true” in `if`

In GDScript, any non-null object is truthy. **Never** write:

```gdscript
if p.solve(goal):  # wrong — always true when query is non-null
```

Use:

```gdscript
if p.solve(goal).has_solution():  # yes / no (one pull, then cuts)
var one = p.solve(goal).first()     # first answer or null (one pull, then cuts)
for solution in p.solve(goal):      # one answer per iteration; break commits
    ...
print(p.solve(goal).all().size())   # every answer (drains)
```

Full reading API (two variables, no `query.values()`, `max_solutions`, cache vs cut): [§4](#4-the-query-pipeline-and-solve).

### Strings are atoms; variables are objects

| In GDScript | In Prolog |
|-------------|-----------|
| `"tom"` passed to `call()` | atom `tom` |
| bound atom from `solution.get(...)` | Godot `String` `"tom"` again |
| `p.variable("Child")` | logical variable |
| `"X"` passed to `call()` | atom `X`, **not** a variable |
| `p.string("hello")` | Prolog string `"hello"` → `PrologTerm` on the way out |

A bound atom is a `String` on purpose (`v == "bob"` works). A quoted Prolog string is not. Policy: [glossary](glossary.md#three-conversion-philosophies).

The name in `variable("Child")` is for debug only. **Solutions are keyed by the variable object**, not by the string `"Child"`.

### Arity comes from `call(...)`

`p.predicate("parent")` stores only the functor name. `parent.call("tom")` is `parent/1`; `parent.call("tom", child)` is `parent/2`. Same object, different arities.

### Composition uses method names

`and`, `or`, and `not` are GDScript keywords. Use:

```gdscript
goal1.conjunction(goal2)   # ,   AND — both must succeed
goal1.disjunction(goal2)   # ;   OR  — either may succeed
goal1.negated()            # \+  not provable
goal1.cut()                # goal1, !  — commit; no backtrack past here
```

See [glossary — conjunction, disjunction, cut](glossary.md#conjunction-disjunction-and-cut).

---

## 3. Your first program

Copy this into a Node script, or open the runnable **[Mini Dungeon](../demos/mini_dungeon/README.md)** (`make run-mini-dungeon`). A script-only version lives in [hello_world_prologot.gd](hello_world_prologot.gd).

```gdscript
extends Node

const PrologotBoot = preload("res://addons/prologot/prologot_boot.gd")

var p: Prologot

func _ready() -> void:
    p = PrologotBoot.create_engine()
    if p == null:
        push_error("Failed to initialize Prologot")
        return

    # 1. Load knowledge (Prolog source)
    p.consult_string("""
        parent(tom, bob).
        parent(tom, liz).
        parent(bob, ann).
        grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
    """)

    # 2. Build reusable handles
    var parent = p.predicate("parent")
    var grandparent = p.predicate("grandparent")
    var child = p.variable("Child")

    # 3. Yes / no on a ground fact
    if p.solve(grandparent.call("tom", "ann")).has_solution():
        print("Tom is Ann's grandparent")

    # 4. All solutions with a variable
    for solution in p.solve(parent.call("tom", child)):
        print("child of tom: ", solution.get(child))

func _exit_tree() -> void:
    if p:
        p.cleanup()
```

**What happened:**

1. `consult_string` added clauses to the SWI knowledge base.
2. `predicate("parent")` gave a reusable functor; `call(...)` built a `PrologGoal`.
3. `variable("Child")` created a Prolog variable object for bindings.
4. `solve(...).has_solution()` answered a boolean question.
5. `for solution in p.solve(...)` iterated every `PrologSolution`; `solution.get(child)` read each binding.

---

## 4. The query pipeline and `solve()`

Every query follows the same five steps:

| Step | GDScript | Result type |
|------|----------|-------------|
| 1 | `p.predicate("parent")` | `PrologPredicate` |
| 2 | `parent.call("tom", child)` | `PrologGoal` |
| 3 | `p.solve(goal)` | `PrologQuery` |
| 4 | `query.has_solution()` / `first()` / `for … in query` / `all()` | bool / `PrologSolution` / loop / Array |
| 5 | `solution.get(child)` | bound value (`String`, `Array`, …) |

`solve()` **opens** an SWI query and returns immediately. It does **not** collect every answer first. Each later call pulls at most one more `PL_next_solution`. That is why `first()` is a real solve-one, and why `break` in a `for` can stop Prolog.

Knowledge used below:

```text
parent(tom, bob). parent(tom, liz). parent(bob, ann).
```

```gdscript
var parent = p.predicate("parent")
var child = p.variable("Child")
var via = p.variable("Via")
```

### `has_solution()` — yes / no (at most one pull, then cut)

```gdscript
if p.solve(parent.call("tom", "bob")).has_solution():
    print("true")
```

A ground goal that holds still produces one (empty) `PrologSolution`, so this is true. Remaining choice points are **cut** so a temporary `solve(g).has_solution()` does not keep Prolog open until the function returns. After this call, `all()` / `for` on the **same** object only see that first cached answer. Open a new `solve()` to search again.

### `first()` — one `PrologSolution` or `null` (at most one pull, then cut)

```gdscript
var sol = p.solve(parent.call("tom", child)).first()
if sol != null:
    print(sol.get(child))   # bob
```

Same cut as `has_solution()`. Use `for` / `all()` when you need every answer.

### `for` — one solution per iteration; `break` commits

```gdscript
for sol in p.solve(parent.call("tom", child)):
    print(sol.get(child))   # bob, then liz
    if sol.get(child) == "bob":
        break               # remaining answers are not computed
```

GDScript does **not** unpack `for via, child in query`. Two variables still mean one `sol` per turn:

```gdscript
for sol in p.solve(parent.call("tom", via).conjunction(parent.call(via, child))):
    print(sol.get(via), "->", sol.get(child))   # bob -> ann
```

Relooping `for` on the **same** query replays solutions already pulled; it does not reopen Prolog. After a `break`, `all()` on that object continues and pulls the rest. Destroying the query (end of the expression, or the variable going out of scope) cuts leftover choice points.

### `all()` — Array of every `PrologSolution`

```gdscript
var sols = p.solve(parent.call("tom", child)).all()
print(sols.size())             # 2
print(sols[0].get(child))      # bob
```

Prefer `for` when you may stop early. Use `all()` when you need `size()` or random access.

### No `query.values()` — `get()` per column

There is no `query.values()`. `all()` + `sol.get(var)` is the API. A matrix (one row = one solution) is a wrapper **you** write, not something the engine returns:

```gdscript
func as_matrix(query: PrologQuery, cols: Array) -> Array:
    var rows := []
    for sol in query:
        var row := []
        for col in cols:
            row.append(sol.get(col))
        rows.append(row)
    return rows

print(as_matrix(p.solve(parent.call("tom", child)), [child]))
# [["bob"], ["liz"]]
```

### Optional cap: `solve(goal, max_solutions)`

`0` (default) means unlimited.

```gdscript
var n = p.variable("N")
p.solve(p.predicate("between").call(1, 1_000_000, n), 5).all()  # 5 answers, not a million
```

Even when lazy, `all()` or a `for` **without** `break` on an infinite goal (`between(1, inf, N)`) will not return. Cap it, or `break`, or use `first()`.

### Constraints

- Godot **main thread** only (`initialize()` / `solve()` / `consult_*`). Off-thread calls log an error and fail.
- Do not `cleanup()` the handle while a query is still open.
- `if p.solve(goal):` is always true (the object is non-null). Use `has_solution()`.

---

## 5. Loading knowledge

### From a file

```gdscript
p.consult_file("res://rules/family.pl")
```

Supports `res://` and `user://` paths.

### From a string

```gdscript
p.consult_string("enemy(goblin, 10).")
```

### Accumulation

Each `consult_file` / `consult_string` **adds** clauses. Nothing is removed unless you call `retract_fact` / `retract_all` or `cleanup()` + `initialize()`.

### Rules belong in Prolog

```prolog
% family.pl
grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
```

Load with `consult_file`, then query from GDScript as in §4.

---

## 6. Variables and composition

### Named variables (debug label only)

```gdscript
var child = p.variable("Child")   # optional name for as_text() / REPL
var anon = p.anonymous()          # fresh `_`, omitted from solutions
```

Two calls to `variable("X")` create **two different** variables.

### Sharing a variable in a chain

Reuse the **same object** so Prolog unifies both positions:

```gdscript
var parent = p.predicate("parent")
var via = p.variable("Via")
var child = p.variable("Child")

var chain = parent.call("tom", via).conjunction(parent.call(via, child))
for solution in p.solve(chain):
    print(solution.get(via), " -> ", solution.get(child))
```

---

## 7. Changing the knowledge base at runtime

```gdscript
var parent = p.predicate("parent")

p.assert_fact(parent.call("tom", "bob"))                    # add
p.retract_fact(parent.call("tom", "bob"))                   # remove one
p.retract_all(parent.call("tom", p.anonymous()))            # remove all parent(tom, _)
p.retract_all(parent.call(p.anonymous(), p.anonymous()))    # remove every parent/2
```

`retract_*` takes a `PrologGoal`, not a string pattern.

---

## 8. Data types: atoms, strings, lists

| You pass | Prolog gets |
|----------|-------------|
| GDScript `"hello"` in `call()` | atom `hello` |
| `p.string("hello")` | Prolog string `"hello"` |
| GDScript `[1, 2, 3]` in `call()` | list `[1, 2, 3]` |
| `p.integer(42)` / `42` in `call()` | integer |

Bound atoms come back as Godot `String`. Bound Prolog strings come back as `PrologTerm` (`is_string()`).

```gdscript
if p.solve(p.predicate("member").call(2, [1, 2, 3])).has_solution():
    print("2 is in the list")
```

Use `p.atom()`, `p.list()`, etc. when you need an explicit `PrologTerm` for inspection — see [API.md](API.md#term-factories).

---

## 9. Godot objects in Prolog

Pass a `Node` or `Resource` directly, or wrap it explicitly:

```gdscript
var player = p.object($Player)
var at = p.predicate("at")
p.assert_fact(at.call(player, "zone_1"))

# Same instance id if you pass $Player to call():
p.solve(at.call($Player, "zone_1")).has_solution()
```

The handle stores a Godot **instance id** (SWI blob). It does **not** keep the node alive. After `queue_free()`, `player.is_valid()` is false — retract stale facts. Main thread only.

---

## 10. Exposing Godot to Prolog (optional)

Nothing from the Godot API is visible to Prolog until you expose it:

```gdscript
p.expose_property("Node", "name", "node_name")
p.expose_property("Node2D", "position")       # Vector2 → [x, y]
p.expose_method("Object", "get_class", "godot_class")

var who = p.variable("Who")
if p.solve(p.predicate("node_name").call($Player, "Hero")).has_solution():
    print("this node is named Hero")

for solution in p.solve(p.predicate("godot_class").call($Player, who)):
    print(solution.get(who))
```

`node_name(Obj, Val)` is **relational**: unbound `Val` reads the property; ground `Val` checks equality. There is no implicit setter. Do not use functor `name/2` — SWI-Prolog already has it. Wrappers are process-global: the dock and `PrologotEngine` share them.

Full signatures: [API.md — Exposing Godot members](API.md#exposing-godot-members).

---

## 11. Plugin integration (optional)

Enable **Prologot** under **Project → Project Settings → Plugins**. Scripts live
in `addons/prologot/`. Full file map and exports: [API — Scene integration](API.md#scene-integration-addonsprologot).

### Autoload `PrologotEngine`

Created at runtime (Play). Same query API as `Prologot`:

```gdscript
PrologotEngine.consult_string("parent(tom, bob).")
var parent = PrologotEngine.predicate("parent")
PrologotEngine.solve(parent.call("tom", "bob")).has_solution()
```

Named bases: `create_knowledge_base` and `switch_knowledge_base` both **wipe**
the user KB then consult the (new or stored) source.

### Scene node + Resource

- **PrologotNode** — drop in a scene; `start()` loads a `PrologKnowledge` and
  extra `consult_files`. By default it reuses `PrologotEngine`.
- **PrologKnowledge** — Resource: `files` then inline `code`.

See [Use cases — Scene setup](use-cases.md#scene-setup).

### Editor console

A **second** `Prologot` handle, only for the dock. Same SWI process as the
game, so clauses loaded in the console are still there when you press Play.

Type `parent(tom, X).`; bindings print as `X = bob`. History: Up/Down.
Details: [Editor console](editor-console.md).

---

## 12. Common mistakes

| Symptom | Likely cause | Fix |
|---------|--------------|-----|
| `if p.solve(g):` always enters the branch | `PrologQuery` is truthy | Use `.has_solution()` |
| `"X"` never binds | String is an atom | Use `p.variable()` |
| `solution["Child"]` fails | Keys are objects, not strings | Use `solution.get(child)` |
| `parent("tom", x)` syntax error | GDScript limitation | Use `parent.call("tom", x)` |
| Second `variable("A")` shares bindings | Each call is a new variable | Reuse one object in the goal |
| Query returns nothing after scene change | Stale object blob | Retract facts or rebuild handles |
| `all()` / `for` never returns | Infinite (or huge) solution space | `first()`, `break`, or `solve(goal, n)` |
| `q.has_solution()` then `q.all()` has one row | `has_solution()` / `first()` cut the rest | Open a new `solve()` for every answer |

More pitfalls for Prolog users: [prolog-developers.md](prolog-developers.md).

---

## Where to go next

1. **[Glossary](glossary.md)** — Prolog names, connectors, conversion philosophies.
2. **[Notes for Prolog developers](prolog-developers.md)** — map SWI-Prolog names to Prologot; REPL vs object API.
3. **[Use cases](use-cases.md)** — recipes for AI, dialogue, crafting, pathfinding.
4. **[API reference](API.md)** — every class, method, and option.
5. **Mini Dungeon** — `make run-mini-dungeon`, [demos/mini_dungeon](../demos/mini_dungeon/README.md) (nodes as Prolog terms + goblin AI).
6. **Demos** — `make run-demo`, `make run-galactic` ([Galactic Customs](../demos/galactic_customs/README.md)).
