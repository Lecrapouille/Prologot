# Note for Prolog developers

You already know SWI-Prolog. This page explains **how Prologot reshapes the surface** when Prolog is called from GDScript — not how to write Prolog itself.

| Read first | Then |
|------------|------|
| New to Prolog vocabulary | [Glossary](glossary.md) — atom, term, goal |
| New to Prologot entirely | [Getting started](getting-started.md) §1–§4 |
| Ready to wire rules into a game | [Use cases](use-cases.md) |
| Need one signature | [API reference](API.md) |

---

## 1. What stays the same

- **Engine:** SWI-Prolog 8.0+. Standard syntax, modules, CLP, tabling, etc.
- **Knowledge:** Facts and rules live in `.pl` files or `consult_string` — normal Prolog source.
- **Semantics:** Unification, backtracking, cut, `:-`, dynamic predicates — unchanged once clauses are in the KB.

If it runs in SWI outside Godot, the Prolog **text** is almost certainly fine inside Prologot.

---

## 2. What changes: two layers

```mermaid
flowchart TB
  subgraph layer_a [Layer A — Prolog text]
    consult["consult_file / consult_string"]
    clauses["Facts, rules, directives"]
    consult --> clauses
  end
  subgraph layer_b [Layer B — GDScript objects]
    pred["predicate(name)"]
    call["call(...) → PrologGoal"]
    solve["solve(goal) → PrologQuery"]
    pred --> call --> solve
  end
  clauses --> solve
```

| Layer | You write | Example |
|-------|-----------|---------|
| A — Knowledge | Prolog | `grandparent(X,Z) :- parent(X,Y), parent(Y,Z).` |
| B — Queries | GDScript objects | `solve(parent.call("tom", Child)).first()` |

**There is no public string query API** (`query_text`, `call_predicate`, etc.). The editor console parses `parent(tom, X).` internally; game code uses Layer B.

---

## 3. Name mapping (SWI ↔ Prologot)

| Prologot (GDScript) | Traditional Prolog | Notes |
|---------------------|-------------------|--------|
| `consult_file(path)` | `consult/1` | `res://` paths resolved |
| `consult_string(code)` | loading + `assertz/1` | multi-line via bootstrap |
| `assert_fact(goal)` | `assertz/1` | goal is a `PrologGoal` object |
| `retract_fact(goal)` | `retract/1` | |
| `retract_all(goal)` | `retractall/1` | use `anonymous()` for `_` |
| `predicate(name)` | functor symbol | **no stored arity** |
| `predicate.call(...)` | compound term | builds `PrologGoal` |
| `variable(name)` / `anonymous()` | variable / `_` | **never** a GDScript `String` |
| `goal.conjunction(other)` | `,(G1,G2)` | not `.and()` (keyword) |
| `goal.disjunction(other)` | `;(G1,G2)` | |
| `goal.negated()` | `\+ G` | |
| `solve(goal).has_solution()` | success / `once/1` | one pull, then cut |
| `solve(goal)` (iterate) | `PL_next_solution` on demand | `break` cuts the query |
| `solve(goal).first()` | first solution | one pull, then cut |
| `solve(goal, max)` | cap on pulls | `0` = unlimited |
| `solution.get(Var)` | binding lookup | **object** key, not name |
| `object(node)` | custom blob | instance id in SWI |

Removed from the public API (no compatibility aliases): `succeeds`, `solve_one`, `solve_all`, `bind`/`bindv`, `add_fact`, `call_predicate`, `call_function`, string retractions.

---

## 4. Mental model: one query in two notations

**SWI-Prolog (interactive):**

```prolog
?- parent(tom, X).
X = bob ;
X = liz.
```

**Prologot (GDScript):**

```gdscript
var parent = prolog.predicate("parent")
var x = prolog.variable("X")
for solution in prolog.solve(parent.call("tom", x)):
    print(solution.get(x))  # bob, then liz
```

Same predicate, same search space. Different **construction** of the goal and **reading** of bindings.

---

## 5. Variables: the biggest trap

In Prolog source, `X` in `parent(tom, X)` is a variable.

In GDScript:

```gdscript
parent.call("tom", "X")   # parent(tom, 'X') — atom X
parent.call("tom", x)     # only a variable if x is prolog.variable()
```

Rules:

1. **`String` → atom.** Always, in `call()`.
2. **`prolog.variable("X")` → variable.** The string `"X"` is a debug label only.
3. **`solution.get(v)`** uses **object identity**. Two `variable("A")` calls are two variables.
4. **`anonymous()`** → `_`. Omitted from `PrologSolution`.

Named variables (option B): arguments stay **positional**; the name helps `as_text()` and the editor REPL, not lookup.

---

## 6. Atoms vs Prolog strings

| GDScript | Prolog term |
|----------|-------------|
| `"hello"` in `call()` | atom `hello` |
| `prolog.string("hello")` | string `"hello"` |

**Retained policy:** bound atoms → Godot `String` (so `== "bob"` and `match` work). Bound Prolog strings → `PrologTerm`. `prolog.atom()` is input-only; `get()` never returns that wrapper for an atom. Alternatives considered: [glossary — conversion philosophies](glossary.md#three-conversion-philosophies).

`,` `;` and cut: [glossary](glossary.md#conjunction-disjunction-and-cut). In GDScript, `,` is `conjunction()`, `;` is `disjunction()`. Cut stays in `.pl` source.

---

## 7. Arity without `name/2` in the API

Old style (removed): `predicate("parent", 2)`.

Current style:

```gdscript
var parent = prolog.predicate("parent")
parent.call("tom")           # parent/1
parent.call("tom", child)    # parent/2
```

`PrologPredicate.as_text()` returns the functor only (`parent`), not `parent/2`. Check `goal.get_arity()` on the `PrologGoal` if needed.

---

## 8. Success tests and multiple solutions

`solve()` maps to `PL_open_query` + `PL_next_solution` **on demand**. It is not `findall/3`. `first()` is a real solve-one; `break` in `for` is a real cut of the remaining choice points.

| Intent | Prologot idiom | Do **not** use |
|--------|----------------|----------------|
| Does it succeed? (`once/1`) | `solve(g).has_solution()` | `if solve(g):` |
| One answer | `solve(g).first()` | (removed) `solve_one` |
| Stream answers | `for s in solve(g):` — `break` cuts | collecting first, then looping |
| All answers | `solve(g).all()` | (removed) `solve_all` |
| Cap the stream | `solve(g, n)` (`0` = unlimited) | hoping `all()` stops alone |
| Column matrix | your wrapper over `all()` + `get` | `query.values()` (does not exist) |

```gdscript
var parent = prolog.predicate("parent")
var child = prolog.variable("Child")
var via = prolog.variable("Via")

if prolog.solve(parent.call("tom", "bob")).has_solution():
    print("true")

var sol = prolog.solve(parent.call("tom", child)).first()
if sol != null:
    print(sol.get(child))   # bob

for sol in prolog.solve(parent.call("tom", child)):
    print(sol.get(child))   # bob, then liz
    if sol.get(child) == "bob":
        break

for sol in prolog.solve(parent.call("tom", via).conjunction(parent.call(via, child))):
    print(sol.get(via), "->", sol.get(child))   # bob -> ann

print(prolog.solve(parent.call("tom", child)).all().size())  # 2
prolog.solve(between.call(1, 1_000_000, n), 5).all()
```

`has_solution()` / `first()` pull at most one answer then **cut** (a GDScript temporary must not keep the query open until the function ends). After that, `all()` on the **same** object only has the cached first row. After a `for`+`break`, `all()` continues and pulls the rest. Relooping `for` on the same query replays the cache.

`PrologQuery` is RefCounted — always truthy when non-null. Same thread as `initialize()`. Do not `cleanup()` while a query is open. An unbounded `all()` / `for` on `between(1, inf, N)` still diverges.

Full narrative: [Getting started §4](getting-started.md#4-the-query-pipeline-and-solve).

---

## 9. Dynamic database

Same as SWI, but patterns are goals:

```gdscript
var score = prolog.predicate("score")
prolog.assert_fact(score.call("player", 100))
prolog.retract_fact(score.call("player", 100))
prolog.retract_all(score.call(prolog.anonymous(), prolog.anonymous()))
```

Multiple `consult_*` calls **accumulate**. Scope cleanup is your responsibility (`retract_all`, or per-entity reset between turns).

---

## 10. Godot objects as terms

`prolog.object($Node)` (or passing the Node to `call()`) stores a **blob** keyed by Godot instance id.

- Unification is by id, not by node path or name.
- Freeing the node does not remove the blob from old facts — retract or invalidate.
- Not thread-safe; call from the main thread.

Expose properties/methods explicitly — Prolog does not see Godot until you `expose_property` / `expose_method`. See [Getting started §10](getting-started.md#10-exposing-godot-to-prolog-optional).

---

## 11. Editor console vs game API

| | Editor dock | Game code |
|---|-------------|-----------|
| Input | `parent(tom, X).` text | `parent.call("tom", x)` |
| Bindings | Dictionary `{"X": "bob"}` | `solution.get(x)` |
| API | `_editor_query` (internal) | `solve()` |

Use the console to **probe** the KB; use the object API in **shipping** code.

---

## 12. Composition and operators

You cannot overload `&`, `|`, `!` on GDExtension classes. You cannot name methods `and`, `or`, `not` in GDScript.

```gdscript
var g = p1.conjunction(p2)    # (p1, p2)
var g = p1.disjunction(p2)      # (p1 ; p2)
var g = p1.negated()            # \+ p1
```

Cut and meta-predicates in **Prolog source** work as usual. Only GDScript-side goal syntax is restricted.

---

## 13. Quick checklist before you commit game code

- [ ] Rules in `.pl` or `consult_string`, not duplicated as giant GDScript strings unless dynamic.
- [ ] Every logical variable is `prolog.variable()` / `anonymous()`, never `"X"`.
- [ ] Same variable object reused wherever Prolog should unify.
- [ ] Boolean tests use `.has_solution()`, not `if solve(...)`.
- [ ] Huge domains use `first()`, `break`, or `solve(goal, n)` — not unbounded `all()`.
- [ ] Bindings read with `.get(var)`, not string keys.
- [ ] Object facts retracted when nodes leave the tree.
- [ ] Functor `name/2` not used for expose (SWI reserved); use `node_name/2`.

---

## Further reading

- [Getting started](getting-started.md) — tutorial path for GDScript authors.
- [Use cases](use-cases.md) — game patterns built on this mapping.
- [API reference](API.md) — complete method list and types.
