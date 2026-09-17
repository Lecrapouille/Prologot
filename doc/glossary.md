# Prolog vocabulary (and how Prologot names it)

Short glossary for developers who know GDScript better than Prolog. For a full walkthrough, read [Getting started](getting-started.md). For the SWI ↔ Prologot mapping table, read [Prolog developers](prolog-developers.md).

---

## What is Prolog?

**Prolog** is a logic programming language. You describe **facts** and **rules**, then ask **questions** (queries). The engine searches for values that make a question true — it does not run a fixed sequence of instructions like GDScript.

Think of it as a database plus inference:

- **Facts** are records you assert: `parent(tom, bob).`
- **Rules** derive new truths: “X is a grandparent of Z if X is parent of Y and Y is parent of Z.”
- **Queries** ask whether something holds, or **which values** make it hold: `parent(tom, Who)?` → `Who = bob`, then `Who = liz`.

Prologot embeds [SWI-Prolog](https://www.swi-prolog.org/) in Godot so your game can use the same style for AI, dialogue, and rule systems but using Godot classes and functions (not strings of Prolog code).

### Shared example (used throughout this page)

Unless noted otherwise, examples assume this knowledge base:

```prolog
parent(tom, bob).
parent(tom, liz).
parent(bob, ann).

grandparent(X, Z) :-
    parent(X, Y),
    parent(Y, Z).
```

In GDScript, the usual setup is:

```gdscript
const PrologotBoot = preload("res://addons/prologot/prologot_boot.gd")

var prolog = PrologotBoot.create_engine()
prolog.consult_string("""
parent(tom, bob).
parent(tom, liz).
parent(bob, ann).
grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
""")

var parent = prolog.predicate("parent")
var grandparent = prolog.predicate("grandparent")
var child = prolog.variable("Child")
var via = prolog.variable("Via")
```

---

## One sentence each

| Prolog name | Everyday meaning | Tiny example |
|-------------|------------------|--------------|
| **Term** | Any value Prolog can hold | `tom`, `42`, `[1, 2]`, `parent(tom, bob)` |
| **Atom** | A name or label (not a number) | `tom`, `flee`, `'Tom'` |
| **Prolog string** | Quoted text, distinct from an atom | `"hello"` |
| **Variable** | A placeholder to fill in | `Child`, `Via`, `_` |
| **Compound** | A structured value with a name and arguments | `parent(tom, bob)` |
| **Functor / arity** | The name of the compound + argument count | `parent` / `2` → `parent/2` |
| **Predicate** | All clauses that share a functor and arity | `parent/2` |
| **Clause** | One unit of knowledge, ending with `.` | `parent(tom, bob).` |
| **Fact** | A clause with no condition (no `:-`) | `parent(tom, bob).` |
| **Rule** | A clause with a condition | `grandparent(X, Z) :- parent(X, Y), parent(Y, Z).` |
| **Head / body** | Left / right side of `:-` | `grandparent(X, Z)` / `parent(X, Y), …` |
| **Knowledge base (KB)** | All clauses currently loaded | `.pl` file + `assert_fact` |
| **Goal** | A term you ask Prolog to prove | `parent(tom, Child)` |
| **Query** | The act of asking that goal | `?- …` in SWI / `solve()` in Prologot |
| **Unification** | Making two terms match by binding variables | `Child` becomes `bob` |
| **Succeed / fail** | The goal can / cannot be proved | `has_solution()` |
| **Backtracking** | Try the next matching clause or choice | second child of `tom` → `liz` |
| **Solution** | One set of bindings that worked | `{Child → bob}` |
| **Ground** | A term with no variables left | `parent(tom, bob)` |
| **`,` (conjunction)** | AND — both goals must succeed | `parent(tom, Via), parent(Via, Child)` |
| **`;` (disjunction)** | OR — either goal may succeed | `dog(X) ; cat(X)` |
| **`!` (cut)** | Commit: do not try other choices | `H > 30, !` |
| **`\+` (negation)** | Succeed if the goal cannot be proved | `\+ parent(bob, tom)` |

A **fact** is knowledge you store. A **goal** has the same shape but is used as a question. `parent(tom, bob).` in a file is a fact; `parent(tom, bob)` passed to `solve()` asks “is this true?”

A **predicate** is not a value. It is the *name* of a family of clauses. Applying that name with arguments produces a goal.

---

## Terms: the family tree

Everything you pass to Prolog, or get back, is a **term**.

```text
term
 ├── atom           tom
 ├── number         42, 3.14
 ├── Prolog string  "hello"          ⚠ not the same as atom hello
 ├── variable       Child, Via, _
 ├── list           [tom, bob]
 └── compound       parent(tom, bob)
```

### Atom

A symbol — like an enum value or an identifier, not a full sentence.

```prolog
tom
flee
zone_1
'Tom'          % quoted: starts with uppercase (otherwise it would be a variable)
```

In Prologot, a GDScript `String` is treated as an atom **in both directions**: `call("tom")` sends the atom `tom`; `solution.get(...)` for a bound atom returns a Godot `String` (`"bob"`). Godot has no atom type, so keeping `String` lets game code write `v == "bob"` and `match v: "attack"`.

```gdscript
parent.call("tom", "bob")              # parent(tom, bob) — two atoms
print(sol.get(child) == "bob")         # true — still a String
```

### Prolog string

Quoted characters. Rare in game rules. **Not** the same as the atom `hello`.

```prolog
named(hello).      % atom hello
msg("hello").       % Prolog string "hello"
```

```gdscript
named.call("hello")        # atom hello
prolog.string("hello")     # Prolog string "hello"
```

`named(hello)` does **not** unify with `msg("hello")`.

### Variable

A placeholder. In Prolog source, names start with an uppercase letter (or `_`).

```prolog
?- parent(tom, Child).
```

`_` is the **anonymous** variable: a hole you do not read back. Two `_` in the same clause are independent holes.

In GDScript, `"Child"` is **not** a variable — it is an atom. Only `prolog.variable()` / `prolog.anonymous()` create real variables.

```gdscript
parent.call("tom", "Child")                 # atom Child — usually a bug
parent.call("tom", prolog.variable("Child")) # a real hole
parent.call("tom", prolog.anonymous())      # _ — omitted from the solution
```

### Compound, functor, arity

`parent(tom, bob)` is a compound. **Functor** = `parent`. **Arity** = 2. Written `parent/2`.

```gdscript
var p = prolog.predicate("parent")   # functor name only
p.call("tom")                        # parent/1
p.call("tom", child)                 # parent/2
```

### Predicate

A **predicate** is every clause that shares the same functor and arity. Both facts below belong to `parent/2`; `parent(tom).` would be a different predicate (`parent/1`).

```prolog
parent(tom, bob).          % clause of parent/2
parent(tom, liz).          % same predicate
parent(tom).               % different predicate: parent/1
```

Calling a predicate builds a **goal**. Loading knowledge defines the predicate; `solve()` asks a question about it.

In Prologot, `prolog.predicate("parent")` is a handle to the **name** only. Arity is fixed when you `call(...)`. There is no `parent/2` object in GDScript.

```gdscript
var goal = parent.call("tom", child)   # goal for parent/2
```

### Goal

A term you pass to `solve()`: “find values that make this true.”

```gdscript
for solution in prolog.solve(parent.call("tom", child)):
    print(solution.get(child))   # bob, then liz
```

Same question in the SWI REPL:

```prolog
?- parent(tom, Child).
Child = bob ;
Child = liz.
```

---

## Knowledge: clause, fact, rule

These are not values you pass to `call()`. They are **what you load** into Prolog.

### Clause

One sentence of knowledge, always ending with `.`. A clause is either a fact or a rule. `consult_file` / `consult_string` add clauses to the **knowledge base**. `assert_fact` / `retract_fact` add or remove one clause at runtime.

### Fact

A clause with **no condition**. Prolog treats it as always true.

```prolog
parent(tom, bob).
parent(tom, liz).
```

The same compound as a **goal** asks “is this known (or derivable)?”

```gdscript
prolog.solve(parent.call("tom", "bob")).has_solution()   # true
prolog.assert_fact(parent.call("tom", "liz"))            # add another fact at runtime
```

### Rule

A clause with a **head** and a **body**, written `Head :- Body.`
Read it as: “the head is true **if** the body succeeds.”

```prolog
grandparent(X, Z) :-          % head
    parent(X, Y),             % body — two goals, AND
    parent(Y, Z).
```

`:-` is not a GDScript operator. Rules live in `.pl` files or `consult_string`. From GDScript you **query** the head:

```gdscript
prolog.solve(grandparent.call("tom", "ann")).has_solution()   # true via bob
```

A **predicate** (`grandparent/2`) is the set of all its facts and rules.

### Knowledge base

The set of clauses SWI currently holds. Each `consult_*` call **adds** clauses. Nothing is removed unless you `retract_fact` / `retract_all`, or the last Prologot handle `cleanup()` clears user predicates.

---

## How a question is answered

### Query

A **query** is “please prove this goal.” In the REPL you type `?- parent(tom, Child).` In Prologot you call `solve(goal)` and receive a `PrologQuery`.

### Unification

Prolog makes two terms match by binding variables. `parent(tom, Child)` against the fact `parent(tom, bob)` **unifies**: `Child` is bound to `bob`. If they cannot match (`parent(tom, bob)` vs `parent(liz, bob)`), that attempt **fails**.

Shared variables stay shared: in `parent(tom, Via), parent(Via, Child)`, the same `Via` must satisfy both goals (`Via = bob`, `Child = ann`).

### Succeed and fail

The goal **succeeds** if Prolog finds at least one proof. It **fails** if none exists. Test with `has_solution()`, not `if solve(goal):` — a `PrologQuery` object is always truthy in GDScript.

### Backtracking and solution

When several clauses or bindings apply, Prolog can return **multiple solutions**. After `bob`, it **backtracks** and tries `liz`. Each `PrologSolution` holds one set of **bindings** (`solution.get(child)`).

A term is **ground** when it has no variables left (`parent(tom, bob)`). Yes/no checks use ground goals; enumerating children needs a variable.

### `\+` — not provable

`\+ Goal` succeeds when `Goal` **fails**. It is not logical negation — it means “I cannot prove it with the current knowledge.”

```gdscript
parent.call("bob", "tom").negated()   # \+ parent(bob, tom) — true (no such fact)
```

---

## Conjunction, disjunction, and cut

These are not types. They control **how goals combine** and how far Prolog may search.

### `,` — AND (conjunction)

Both sides must succeed, left then right. Shared variables stay shared.

```prolog
grandparent(X, Z) :-
    parent(X, Y),      % first find Y such that parent(X, Y)
    parent(Y, Z).      % then find Z such that parent(Y, Z)
```

In Prologot (equivalent to `parent(tom, Via), parent(Via, Child)`):

```gdscript
var goal = parent.call("tom", via).conjunction(parent.call(via, child))
for solution in prolog.solve(goal):
    print(solution.get(via), "->", solution.get(child))   # bob -> ann
```

### `;` — OR (disjunction)

Either side may succeed. Prolog tries the left goal first; if it fails (or you ask for more answers), it tries the right.

This example uses a **separate** mini-KB (not the family tree):

```prolog
animal(X) :- dog(X) ; cat(X).
dog(rex).
cat(mia).
```

In Prologot:

```gdscript
prolog.consult_string("""
animal(X) :- dog(X) ; cat(X).
dog(rex).
cat(mia).
""")
var pet = prolog.predicate("animal")
var x = prolog.variable("X")
var goal = prolog.predicate("dog").call(x).disjunction(prolog.predicate("cat").call(x))
# same shape as: dog(X) ; cat(X)
```

### `!` — cut (commit)

“Keep the choices made so far. Do **not** backtrack past this point.”

Typical game use: the first matching rule wins.

```prolog
decide(attack, H) :- H > 30, !.
decide(flee, H)   :- H < 20, !.
decide(patrol, _).
```

Without cuts, `decide(Action, 40)` can still yield `patrol` later. With cuts, once `H > 30` succeeds, Prolog commits to `attack`.

Write priority rules with `!` in `.pl` / `consult_string`. From GDScript, `goal.cut()` builds `goal, !`:

```gdscript
prolog.consult_string("""
decide(attack, H) :- H > 30, !.
decide(flee, H)   :- H < 20, !.
decide(patrol, _).
""")
var decide = prolog.predicate("decide")
var action = prolog.variable("Action")
var first_only = parent.call("tom", child).cut()
# parent(tom, Child), ! — one answer even though tom has two children

var first = prolog.solve(decide.call(action, 40)).first()
print(first.get(action))   # attack
```

---

## How Prologot maps the names

| You write in GDScript | Prolog name | Class / type |
|----------------------|-------------|--------------|
| `"tom"` in `call()` | atom | Godot `String` (input) |
| `prolog.atom("tom")` | atom (explicit) | `PrologTerm` |
| `prolog.string("hi")` | Prolog string | `PrologTerm` |
| `prolog.integer(42)` | integer | `PrologTerm` |
| `prolog.variable("Child")` | variable | `PrologVariable` (a kind of `PrologTerm`) |
| `prolog.list([1, 2])` | list | `PrologTerm` |
| `prolog.compound("point", [1, 2])` | compound | `PrologTerm` |
| `prolog.predicate("parent")` | predicate name (no arity) | `PrologPredicate` |
| `predicate("parent").call(...)` | goal (compound) | `PrologGoal` |
| `consult_file` / `consult_string` | load clauses into the KB | — |
| `assert_fact(goal)` | add a fact (`assertz`) | — |
| `retract_fact` / `retract_all` | remove clause(s) | — |
| `solve(goal)` | query | `PrologQuery` |
| one iteration / `.first()` | one solution | `PrologSolution` |
| `prolog.object($Player)` | blob (Godot handle) | `PrologObject` |

`PrologTerm` — a Prolog value as a Godot object.
`PrologGoal` — a `PrologTerm` used as a question (plus `conjunction` / `disjunction` / `negated` / `cut`).
`PrologVariable` — a `PrologTerm` that is a hole.
`PrologPredicate` — a handle to a predicate **name**; arity comes from `call()`.
`PrologQuery` — the result of asking a goal (`has_solution`, `first`, iterate).
`PrologSolution` — one successful set of bindings.

---

## What comes back from `solution.get(var)`

### Going in (`call()`)

| GDScript | Becomes in Prolog |
|----------|-------------------|
| `String` | **always an atom** |
| `int` / `float` | number |
| `Array` | list |
| `PrologVariable` | variable |
| `PrologTerm` from `prolog.string()` | Prolog string |
| Node / Resource | `PrologObject` blob |

### Coming out (`solution.get(...)`)

| Prolog | Becomes in GDScript |
|--------|---------------------|
| atom `hello` | Godot `String` `"hello"` |
| Prolog string `"hello"` | **`PrologTerm`**, not a `String` |
| integer / float | `int` / `float` |
| list | `Array` |
| compound | `Dictionary` `{functor, args}` |
| Godot blob | `PrologObject` |

This is the **retained hybrid policy** (implemented in `term_to_variant` / `variant_to_term` in `src/PrologConversion.cpp`).

An atom **round-trips** as `String` → atom → `String`. Game code can compare and `match` names as strings.

A Prolog string does **not** become a `String`: you send it with `prolog.string("hello")` and get a `PrologTerm` back, so `hello` and `"hello"` stay distinguishable after `get()`.

`prolog.atom("tom")` builds an explicit `PrologTerm` for input (inspection, `as_text()`). Bindings never return that wrapper — wrapping atoms on output would break `== "bob"`.

`PrologObject` is separate: it identifies a Godot node, not a text value.

---

## Three conversion philosophies

The question applies only to **text-like** values: atom vs Prolog string vs Godot `String`. Numbers, lists, and `PrologObject` behave the same in all three approaches.

Same knowledge for every comparison:

```prolog
named(hello).      % atom
msg("hello").       % Prolog string
```

Same GDScript question:

```gdscript
var who = prolog.variable("Who")
var from_atom = prolog.solve(prolog.predicate("named").call(who)).first().get(who)
var from_str  = prolog.solve(prolog.predicate("msg").call(who)).first().get(who)
```

### 1. Godot-first (output)

Both results become a Godot `String`. The atom / Prolog-string distinction exists **only on input**, via `prolog.string()`.

```text
from_atom → "hello"     (String)
from_str  → "hello"     (String)
```

If you pass `from_str` back into `call()`, it becomes an **atom**, not a Prolog string. That round-trip is intentionally lossy.

| For | Against |
|-----|---------|
| Game code always reads names as `String` — no `typeof` | A Prolog string and an atom look identical after `get()` |
| Matches how games use labels (`bob`, `flee`) | SWI code that needs `"hello"` ≠ `hello` must rebuild with `prolog.string()` |
| One simple output rule | Documented loss if you echo a Prolog string back in |

### 2. Prolog-first

Anything that is not a number, a list, or a Godot object returns a `PrologTerm` — **including atoms**.

```text
from_atom → PrologTerm (atom hello)
from_str  → PrologTerm (string "hello")
```

You would call `from_atom.get_atom()` (or similar) instead of using the value as a `String`.

| For | Against |
|-----|---------|
| One Godot type for “a Prolog value” | `get(child)` is never `"bob"` |
| Atom and Prolog string stay distinct | Every NPC name needs an extra unwrap |
| Faithful round-trips | Heavy for typical game code |

### 3. Hybrid — **retained**

Atoms become `String` (the common case). Prolog strings become `PrologTerm` (distinct from atoms).

```text
from_atom → "hello"        (String)
from_str  → PrologTerm     (not a String)
```

| For | Against |
|-----|---------|
| Honest about SWI: `hello` ≠ `"hello"` on output | “Text” can be two Godot types |
| Atoms (usual bindings) are already `String` | Rare Prolog strings are not `String` |
| `== "bob"` and `match` keep working | Use `is PrologTerm` only for quoted Prolog strings |

**This is the policy in Prologot.** Input: `String` → atom (`"Child"` is never a variable). Output: atom → `String`, Prolog string → `PrologTerm`. Philosophies 1 and 2 were considered but are not runtime switches.

`PrologObject` is outside this choice: it is a Godot identity (instance id, lifetime), not another spelling of text.

---

## Related

- [Getting started](getting-started.md) — first program
- [Prolog developers](prolog-developers.md) — SWI name mapping
- [API — Type conversion](API.md#type-conversion)
