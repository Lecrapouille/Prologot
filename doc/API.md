# Prologot API reference

Complete reference for the Prologot GDExtension. For concepts and tutorials, read [Getting started](getting-started.md) first; for game recipes, see [Use cases](use-cases.md).

---

## Table of contents

1. [Overview](#overview)
2. [Class `Prologot`](#class-prologot)
   - [Initialization](#initialization-and-cleanup)
   - [Loading knowledge](#loading-knowledge)
   - [Term factories](#term-factories)
   - [Query execution](#query-execution)
   - [Dynamic facts](#dynamic-facts)
   - [Exposing Godot members](#exposing-godot-members)
   - [Introspection](#introspection)
3. [Class `PrologPredicate`](#class-prologpredicate)
4. [Class `PrologGoal`](#class-prologgoal)
5. [Class `PrologQuery`](#class-prologquery)
6. [Class `PrologSolution`](#class-prologsolution)
7. [Class `PrologVariable`](#class-prologvariable)
8. [Class `PrologTerm`](#class-prologterm)
9. [Class `PrologObject`](#class-prologobject)
10. [Scene integration](#scene-integration)
11. [PrologotEngine singleton](#prologotengine-singleton-autoload)
12. [Type conversion](#type-conversion)

---

## Overview

### Minimal lifecycle

```gdscript
var prolog = Prologot.new()
prolog.initialize({"home": "res://bin/linux/swipl"})
prolog.consult_file("res://rules.pl")

var parent = prolog.predicate("parent")
var child = prolog.variable("Child")
prolog.solve(parent.call("tom", "bob")).has_solution()
for solution in prolog.solve(parent.call("tom", child)):
    print(solution.get(child))

prolog.cleanup()
```

### Object graph

```mermaid
flowchart LR
  Prologot --> PrologPredicate
  PrologPredicate -->|call| PrologGoal
  PrologGoal -->|conjunction / disjunction / negated| PrologGoal
  Prologot -->|solve| PrologQuery
  PrologQuery --> PrologSolution
  PrologSolution -->|get| PrologVariable
  Prologot --> PrologVariable
  Prologot --> PrologTerm
  Prologot --> PrologObject
```

### Rules of thumb

| Topic | Rule |
|-------|------|
| Goals | `predicate(name).call(...)` — not string queries |
| Success test | `solve(goal).has_solution()` — not `if solve(goal):` |
| Variables | `variable()` / `anonymous()` — not `"X"` strings |
| Bindings | `solution.get(var_object)` — not `solution["X"]` |
| Arity | From `call()` argument count, not `predicate(name, n)` |
| Composition | `conjunction` / `disjunction` / `negated` |

---

## Class `Prologot`

Main entry point. SWI-Prolog is process-global; each `Prologot` is a handle.

### Initialization and cleanup

#### `initialize(options: Dictionary = {}) -> bool`

Starts SWI-Prolog the first time in this process, then attaches the handle.
Idempotent per handle. Bootstraps helpers for `consult_string()` on first start.

**Returns:** `true` on success; on failure call `get_last_error()`.

**Common options:**

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `"home"` | String | `""` | SWI-Prolog home (`res://bin/.../swipl`) |
| `"quiet"` | bool | `true` | Suppress startup messages |
| `"stack limit"` | String | `""` | e.g. `"1g"`, `"512m"` |
| `"table space"` | String | `""` | SLG table space |
| `"optimized"` | bool | `false` | Optimized compilation |
| `"threads"` | bool | `true` | Allow threads |
| `"on error"` | String | `"print"` | `"print"`, `"halt"`, `"status"` |
| `"on warning"` | String | `"print"` | Same values |
| `"goal"` | String / Array | — | Run at startup |
| `"script file"` | String | — | Load script at startup |
| `"prolog flags"` | Dictionary | `{}` | Flag overrides |
| `"file search paths"` | Dictionary | `{}` | File search paths |

```gdscript
prolog.initialize({"home": "res://bin/linux/swipl", "on error": "print"})
```

#### `cleanup() -> void`

Detaches this handle. Safe to call multiple times. Does **not** call `PL_cleanup()`
(that happens when the GDExtension unloads). The last attached handle resets the
user knowledge base. Must `initialize()` again before use.

#### `is_initialized() -> bool`

#### `get_last_error() -> String`

Last stored error string. Does not call `push_error()`.

---

### Loading knowledge

#### `consult_file(filename: String) -> bool`

Loads a `.pl` file via SWI `consult/1`. Supports `res://` and `user://`.

**Note:** Clauses **accumulate** across calls.

#### `consult_string(code: String) -> bool`

Parses multi-line Prolog (facts, rules, directives) via bootstrap predicate.

---

### Term factories

Optional explicit terms. Most queries pass plain Variants to `call()` directly.

| Method | Returns | Notes |
|--------|---------|-------|
| `atom(name: String)` | `PrologTerm` | Atom |
| `integer(value: int)` | `PrologTerm` | Integer |
| `real(value: float)` | `PrologTerm` | Float |
| `string(value: String)` | `PrologTerm` | Prolog string `"…"`, not atom |
| `nil()` | `PrologTerm` | `[]` |
| `list(items: Array)` | `PrologTerm` | Prolog list |
| `compound(functor: String, args: Array)` | `PrologTerm` | Compound |
| `variable(name: String = "")` | `PrologVariable` | Logical variable |
| `anonymous()` | `PrologVariable` | `_` |
| `predicate(name: String)` | `PrologPredicate` | Reusable functor |
| `object(value: Object)` | `PrologObject` | Node / Resource handle |

**`call()` accepts:** atoms (String), numbers, arrays (lists), `PrologVariable`, `PrologObject`, Nodes/Resources (auto-wrapped).

---

### Query execution

#### `solve(goal: PrologGoal) -> PrologQuery`

Solves `goal` eagerly (all solutions collected). Returns a non-null `PrologQuery`; check `.has_solution()`.

```gdscript
var q = prolog.solve(parent.call("tom", child))
q.has_solution()
q.first()           # PrologSolution or null
q.all()             # Array of PrologSolution
for s in q: ...     # iteration protocol
```

There is **no** public string query API. Editor dock uses internal `_editor_query`.

---

### Dynamic facts

| Method | SWI equivalent | Description |
|--------|----------------|-------------|
| `assert_fact(goal: PrologGoal) -> bool` | `assertz/1` | Add clause |
| `retract_fact(goal: PrologGoal) -> bool` | `retract/1` | Remove first match |
| `retract_all(goal: PrologGoal) -> bool` | `retractall/1` | Remove all matches |

Pattern arguments: use `anonymous()` for `_`.

```gdscript
prolog.retract_all(parent.call(prolog.anonymous(), prolog.anonymous()))
```

---

### Exposing Godot members

Godot API is **not** auto-exported.

#### `expose_property(class_name: String, property: String, predicate: String = "") -> bool`

Installs relational wrapper `predicate(Object, Value)`.

- Unbound `Value` → read property.
- Ground `Value` → succeed only if equal (no setter).
- `class_name` filters with `Object.is_class()`. Empty = any live object.
- Default predicate for property `"name"` is `node_name` (avoid SWI `name/2`).
- `Vector2` / `Vector3` → `[x, y]` / `[x, y, z]`.

#### `expose_method(class_name: String, method: String, predicate: String = "") -> bool`

Arity = 1 (object) + required args + 1 if return value ≠ void.

#### `unexpose(predicate: String, arity: int) -> bool`

#### `list_exposed() -> Array`

Each element: `{kind, class, member, predicate, arity}`.

Main thread only.

---

### Introspection

#### `predicate_exists(name: String, arity: int) -> bool`

#### `list_predicates() -> Array`

Dictionaries `{functor, args}` with `args` like `["/", 2]` for `name/2`.

---

## Class `PrologPredicate`

Reusable functor. Created with `prolog.predicate("parent")`.

| Method | Returns | Description |
|--------|---------|-------------|
| `get_name() -> String` | Functor name | |
| `as_text() -> String` | Functor only | No `/arity` suffix |
| `call(...)` | `PrologGoal` | Vararg; arity = arg count |
| `callv(args: Array) -> PrologGoal` | Goal | Array form |

```gdscript
var p = prolog.predicate("route")
var g: PrologGoal = p.call("a", "b", "c")
```

---

## Class `PrologGoal`

Built by `call()` or composition. Passed to `solve()` / `assert_fact()` / `retract_*()`.

| Method | Prolog | Description |
|--------|--------|-------------|
| `get_functor() -> String` | | Outermost functor |
| `get_args() -> Array` | | Outermost arguments |
| `get_arity() -> int` | | `get_args().size()` |
| `as_text() -> String` | | Debug text |
| `to_term() -> PrologTerm` | | Inspection only |
| `conjunction(other: PrologGoal) -> PrologGoal` | `,/2` | Both must succeed |
| `disjunction(other: PrologGoal) -> PrologGoal` | `;/2` | Either succeeds |
| `negated() -> PrologGoal` | `\+/1` | Negation as failure |

---

## Class `PrologQuery`

Returned by `solve()`. Always truthy as an object — use `has_solution()`.

| Method | Description |
|--------|-------------|
| `has_solution() -> bool` | At least one solution |
| `first() -> Variant` | First `PrologSolution` or `null` |
| `all() -> Array` | All solutions |
| `_iter_init` / `_iter_next` / `_iter_get` | Godot `for` loop protocol |

---

## Class `PrologSolution`

One answer. Variable bindings keyed by **object identity**.

| Method | Description |
|--------|-------------|
| `get(variable: PrologVariable) -> Variant` | Bound value or `null` |
| `has(variable: PrologVariable) -> bool` | |
| `get_bindings() -> Dictionary` | `PrologVariable` → value |
| `values() -> Array` | Bound values only |
| `put(variable, value)` | Internal / tests |

Anonymous variables omitted.

---

## Class `PrologVariable`

Logical variable. Subclass of `PrologTerm`.

| Method | Description |
|--------|-------------|
| `get_id() -> int` | Stable identity for binding keys |
| `get_name() -> String` | Debug label only |
| `is_anonymous() -> bool` | |

---

## Class `PrologTerm`

Atom, number, string, list, compound, or variable wrapper.

Inspectors: `get_kind()`, `is_atom()`, `is_integer()`, `is_float()`, `is_string()`, `is_nil()`, `is_list()`, `is_compound()`, `is_variable()`, `get_atom()`, `get_integer()`, `get_real()`, `get_string()`, `get_functor()`, `get_args()`, `as_text()`.

---

## Class `PrologObject`

Handle to a Godot `Object` (instance id blob).

| Method | Description |
|--------|-------------|
| `get_object() -> Object` | Live object or `null` if freed |
| `is_valid() -> bool` | ObjectDB still has id |
| `get_instance_id() -> int` | |
| `get_class_name() -> String` | |
| `equals(other: PrologObject) -> bool` | Same id |
| `as_text() -> String` | Debug label |

Does not keep the node alive.

---

## Scene integration

### `PrologotNode` (Node)

Exports: `knowledge` (`PrologKnowledge`), `consult_files`, `swipl_home`, `auto_start`, `use_autoload`.

Forwards `predicate`, `solve`, `consult_*`, `expose_*`, etc. Reuses `/root/PrologotEngine` when present.

### `PrologKnowledge` (Resource)

`@export` file list + inline Prolog. `load_into(engine) -> bool`.

---

## PrologotEngine singleton (Autoload)

Same API as `Prologot` via `addons/prologot/prologot_singleton.gd`. Available at runtime when the plugin is enabled.

**Additional methods:**

| Method | Description |
|--------|-------------|
| `create_knowledge_base(name: String, code: String) -> bool` | Store and load named KB |
| `switch_knowledge_base(name: String) -> bool` | Reload stored KB |
| `list_knowledge_bases() -> Array` | Names |

Example: [Use cases — Multiple knowledge bases](use-cases.md#scene-setup).

---

## Type conversion

Automatic conversion between Prolog terms and Godot `Variant`s when using `call()` / `solve()`.

### Prolog → Godot (typical bindings)

| Prolog | Godot |
|--------|-------|
| Atom | `String` |
| Integer | `int` |
| Float | `float` |
| Prolog string | `PrologTerm` or `String` (context) |
| `[]` | empty `Array` |
| List | `Array` |
| Compound | `Dictionary` `{functor, args}` or nested |
| Godot blob | `PrologObject` |
| Unbound var | omitted from `PrologSolution` |

### Godot → Prolog (in `call()`)

| Godot | Prolog |
|-------|--------|
| `String` | **Atom** (not Prolog string) |
| `int` / `float` | integer / float |
| `Array` | list |
| `bool` | atom `true` / `false` |
| `Vector2` / `Vector3` | `[x, y]` / `[x, y, z]` |
| `Node` / `Resource` | object blob |
| `PrologVariable` | variable |
| `PrologTerm` | corresponding term |

Use `prolog.string()` when you need a Prolog string term explicitly.

### Compound dictionary form

```gdscript
{"functor": "parent", "args": ["tom", "bob"]}
```

---

## Related documentation

- [Getting started](getting-started.md) — tutorial and concepts
- [Prolog developers](prolog-developers.md) — SWI name mapping
- [Use cases](use-cases.md) — game recipes
- [Editor console](editor-console.md) — REPL in the editor
- [Troubleshooting](troubleshooting.md) — common errors
