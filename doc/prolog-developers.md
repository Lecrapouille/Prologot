# Note for Prolog Developers

## Compatibility with SWI-Prolog

Prologot uses SWI-Prolog 8.0+ and supports all standard Prolog features. The underlying Prolog engine is fully compatible with standard Prolog code, so you can use any Prolog code that works with SWI-Prolog.

## Syntax Variation

Prologot uses an object API from GDScript. Rules stay in `.pl` files (or `consult_string`). Queries are not Prolog source strings.

| Prologot Name | Traditional Prolog Name | Description |
|---------------|-------------------------|-------------|
| `consult_file()` | `consult/1` | Load a Prolog file |
| `consult_string()` | `consult_string/1` or `assertz/1` | Load Prolog code from a string |
| `assert_fact(goal)` | `assertz/1` | Add a fact from a `PrologGoal` |
| `add_fact()` | `assert/1` | Legacy: add a fact from a source string |
| `retract_fact()` | `retract/1` | Goal or legacy source string |
| `retract_all()` | `retractall/1` | Goal or legacy source string |
| `object()` | SWI blob `godot_object` | Handle to a Godot `Object` (instance id) |
| `predicate(name, arity)` | functor / arity | Reusable predicate object |
| `variable()` / `anonymous()` | logical variable / `_` | Variable object, never a `String` |
| `predicate.bind(...)` | compound term | Builds a `PrologGoal` (`parent(tom, Child)`) |
| `goal.conjunction` / `disjunction` / `negated` | `','/2`, `';'/2`, `\+/1` | Goal composition (`and` / `or` are GDScript keywords) |
| `succeeds(goal)` | `once(Goal)` / success test | Boolean — GDScript `if []:` is true |
| `solve(goal)` / `solve_all(goal)` | `findall/3` | Array of `PrologSolution` |
| `solve_one(goal)` | first answer | First `PrologSolution` or `null` |
| `solution.get(variable)` | binding lookup | Value of that variable object |

```gdscript
var parent = prolog.predicate("parent", 2)
var child = prolog.variable()
for solution in prolog.solve(parent.bind("tom", child)):
    print(solution.get(child))
```

There is no string query API (`query_text` and friends are gone). The editor console parses typed goals internally. Conjunctions in game code use `PrologGoal.conjunction()`.

## Prolog Syntax Notes

- **Variables are objects.** A `String` passed to `bind()` is always an atom. `"X"` is the atom `X`, not a variable. Call `prolog.variable()` (optionally with a debug name).
- **Atoms vs strings.** A GDScript `String` sent as an argument is a Prolog atom. Use `prolog.string()` for a Prolog string. Bound atoms come back as `String`; bound Prolog strings come back as `PrologTerm`.
- **Godot objects.** `prolog.object(node)` (or passing a `Node` / `Resource` to `bind()`) stores the instance id as a SWI blob. The handle does not keep the node alive.
- **No trailing periods in facts.** `add_fact()` / `retract_*()` ignore a trailing period if present.
- **Knowledge base accumulation.** Multiple calls to `consult_file()` and `consult_string()` accumulate clauses. Use `retract_all()` to remove specific predicates if needed.
