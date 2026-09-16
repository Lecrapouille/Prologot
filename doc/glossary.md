# Prolog vocabulary (and how Prologot names it)

Short glossary for people who know GDScript better than Prolog. For the full tutorial, read [Getting started](getting-started.md). For the SWI ↔ Prologot table, read [Prolog developers](prolog-developers.md).

---

## One sentence each

| Prolog name | Everyday meaning | Tiny example |
|-------------|------------------|--------------|
| **Term** | Any value Prolog can hold | `tom`, `42`, `[1,2]`, `parent(tom, bob)` |
| **Atom** | A name / label (not a number) | `tom`, `flee`, `'Tom'` |
| **Prolog string** | Quoted text, different from an atom | `"hello"` |
| **Variable** | A hole to fill in | `X`, `Child`, `_` |
| **Compound** | A named box with arguments | `parent(tom, bob)` |
| **Functor / arity** | Box name + how many arguments | `parent` / `2` |
| **Predicate** | All clauses that share a functor/arity | `parent/2` |
| **Clause** | One unit of knowledge, ends with `.` | `parent(tom, bob).` |
| **Fact** | A clause that is always true (no `:-`) | `parent(tom, bob).` |
| **Rule** | A clause with a condition | `grandparent(X,Z) :- parent(X,Y), parent(Y,Z).` |
| **Head / body** | Left / right of `:-` | `grandparent(X,Z)` / `parent(X,Y), …` |
| **Knowledge base (KB)** | Every clause currently loaded | `.pl` + `assert_fact` |
| **Goal** | A term you ask Prolog to prove | `parent(tom, X)` |
| **Query** | Asking that goal | `?- …` / `solve()` |
| **Unification** | Make two terms the same by filling holes | `X` becomes `bob` |
| **Succeed / fail** | The goal is true / no answer | `has_solution()` |
| **Backtracking** | Try the next clause / next choice | second child of tom |
| **Solution** | One set of bindings that worked | `{Child → bob}` |
| **Ground** | A term with no variables left | `parent(tom, bob)` |
| **`,` (conjunction)** | AND — both goals must succeed | `parent(tom, Y), parent(Y, Z)` |
| **`;` (disjunction)** | OR — either goal may succeed | `dog(X) ; cat(X)` |
| **`!` (cut)** | Commit: do not try other choices | `H > 30, !` |
| **`\+` (negation)** | Succeed if the goal cannot be proved | `\+ parent(bob, tom)` |

A **fact** is knowledge. A **goal** is the same shape used as a question. `parent(tom, bob).` in a `.pl` file is a fact. `parent(tom, bob)` passed to `solve()` is a goal: “is this true?”

A **predicate** is not a value. It is the *name* of a family of clauses. Calling that name produces a goal.

---

## Terms: the family tree

Everything you pass to Prolog, or get back, is a **term**.

```text
term
 ├── atom          tom
 ├── number        42, 3.14
 ├── Prolog string "hello"          ⚠️ not the same as atom hello
 ├── variable      X, Child, _
 ├── list          [tom, bob]
 └── compound      parent(tom, bob)
```

### Atom

A symbol. Think “enum value” or “id”, not a sentence.

```prolog
tom
flee
zone_1
'Tom'          ⚠️ quoted because it starts with an uppercase letter (else it would be a variable)
```

In Prologot, a GDScript `String` is this atom **in both directions**: `call("tom")` sends the atom `tom`; `solution.get(...)` for a bound atom is a Godot `String` (`"tom"`). Variant has no atom type; keeping `String` lets game code write `v == "bob"` and `match v: "attack"`.

```gdscript
parent.call("tom", "bob")   # parent(tom, bob)  — two atoms
print(sol.get(child) == "bob")  # true — still a String
```

### Prolog string

Quoted characters. Rare in game rules. **Not** the same as the atom `hello`.

```prolog
named(hello).      % atom
msg("hello").      % Prolog string
```

```gdscript
parent.call("hello")              # atom hello
prolog.string("hello")            # Prolog string "hello"
```

`named(hello)` does **not** unify with `msg("hello")`.

### Variable

A placeholder. In Prolog source, an identifier that starts with an uppercase letter (or `_`).

```prolog
?- parent(tom, X).     % X is a variable
```

`_` is the **anonymous** variable: a hole you do not read back. Two `_` in the same clause are two different holes.

In GDScript, `"X"` is **not** a variable. Only `prolog.variable()` / `prolog.anonymous()` are.

```gdscript
parent.call("tom", "X")                 # atom X  — usually a bug
parent.call("tom", prolog.variable())   # a real hole
parent.call("tom", prolog.anonymous())  # _  — not in the solution
```

### Compound, functor, arity

`parent(tom, bob)` is a compound. Functor = `parent`. Arity = 2. Prologists write that `parent/2`.

```gdscript
var parent = prolog.predicate("parent")  # functor only
parent.call("tom")                       # parent/1
parent.call("tom", child)                # parent/2
```

### Predicate

A **predicate** is every clause that shares the same functor and arity. `parent/2` is one predicate: both facts below belong to it.

```prolog
parent(tom, bob).          % clause of parent/2
parent(tom, liz).          % same predicate
parent(tom).               % different predicate: parent/1
```

Calling a predicate produces a **goal**. Loading knowledge defines the predicate; `solve()` asks it a question.

In Prologot, `prolog.predicate("parent")` is only a handle to the **name**. The arity is chosen when you `call(...)`. There is no `parent/2` object in GDScript.

```gdscript
var parent = prolog.predicate("parent")   # the name "parent"
var goal = parent.call("tom", child)      # a goal for parent/2
```

### Goal

A term you give to `solve()`: “find values that make this true”.

```gdscript
var child = prolog.variable("Child")
var goal = parent.call("tom", child)   # PrologGoal
for solution in prolog.solve(goal):
    print(solution.get(child))         # bob, then liz
```

Same question in the SWI REPL:

```prolog
?- parent(tom, Child).
```

---

## Knowledge: clause, fact, rule

These are not values you pass to `call()`. They are **what you load** into Prolog.

### Clause

One sentence of knowledge, always ending with `.`. A clause is either a fact or a rule. `consult_file` / `consult_string` add clauses to the **knowledge base**. `assert_fact` / `retract_fact` add or remove one at runtime.

### Fact

A clause with **no condition**. Prolog treats it as always true.

```prolog
parent(tom, bob).
parent(tom, liz).
```

The same compound used as a **goal** asks “is this a fact (or provable by a rule)?”

```gdscript
prolog.consult_string("parent(tom, bob).")
prolog.assert_fact(parent.call("tom", "liz"))   # another fact
prolog.solve(parent.call("tom", "bob")).has_solution()  # true
```

### Rule

A clause with a **head** and a **body**, written `Head :- Body.`
Read: “the head is true **if** the body succeeds.”

```prolog
grandparent(X, Z) :-          % head
    parent(X, Y),             % body (two goals, AND)
    parent(Y, Z).
```

`:-` is not a GDScript operator. Rules live in `.pl` / `consult_string`. From GDScript you only **query** the head:

```gdscript
prolog.solve(grandparent.call("tom", "ann")).has_solution()
```

A **predicate** (`grandparent/2`) is the set of all its facts and rules.

### Knowledge base

The set of clauses SWI currently holds. Each `consult_*` **adds**. Nothing disappears unless you `retract_fact` / `retract_all`, or the last Prologot handle `cleanup()` wipes user predicates.

---

## How a question is answered

### Query

A **query** is “please prove this goal.” In the REPL you type `?- parent(tom, X).` In Prologot you call `solve(goal)` and get a `PrologQuery`.

### Unification

Prolog makes two terms the same by filling variables. `parent(tom, Child)` against the fact `parent(tom, bob)` **unifies**: `Child` is bound to `bob`. If they cannot be made the same (`parent(tom, bob)` vs `parent(liz, bob)`), that attempt **fails**.

Shared variables stay shared: in `parent(tom, Via), parent(Via, Child)`, the same `Via` must fit both goals.

### Succeed and fail

The goal **succeeds** if Prolog finds at least one way to prove it. It **fails** if there is no way. That is `has_solution()`, not `if solve(goal):` (a `PrologQuery` object is always truthy).

### Backtracking and solution

If several clauses match, Prolog can give **several solutions**. After `bob`, it **backtracks** and tries `liz`. Each `PrologSolution` is one set of **bindings** (`solution.get(child)`).

A term is **ground** when it has no variables left (`parent(tom, bob)`). A yes/no check is usually a ground goal; enumerating children needs a variable.

### `\+` — not provable

`\+ Goal` succeeds if `Goal` **fails**. It is not logical “false”, it is “I cannot prove it now.”

```gdscript
parent.call("bob", "tom").negated()   # \+ parent(bob, tom)
```

---

## Conjunction, disjunction, and cut

These are not types. They control **how goals are combined** (and how far Prolog is allowed to search).

### `,` — AND (conjunction)

Both sides must succeed, left then right. Shared variables stay shared.

```prolog
grandparent(X, Z) :-
    parent(X, Y),      % first find a child Y of X
    parent(Y, Z).      % then a child Z of that Y
```

In Prologot:

```gdscript
var via = prolog.variable("Via")
var child = prolog.variable("Child")
var goal = parent.call("tom", via).conjunction(parent.call(via, child))
# same as: parent(tom, Via), parent(Via, Child)
```

### `;` — OR (disjunction)

Either side may succeed. Prolog tries the left goal first; if that fails (or you ask for more answers), it tries the right.

```prolog
animal(X) :- dog(X) ; cat(X).
```

In Prologot:

```gdscript
var x = prolog.variable("X")
var goal = dog.call(x).disjunction(cat.call(x))
# same as: dog(X) ; cat(X)
```

### `!` — cut (commit)

“Keep the choices made so far. Do **not** backtrack past this point.”

Typical game use: the first matching clause wins.

```prolog
decide(attack, H) :- H > 30, !.
decide(flee, H)   :- H < 20, !.
decide(patrol, _).
```

Without the cuts, `decide(Action, 40)` can still offer `patrol` as a later answer. With the cuts, once `H > 30` succeeds, Prolog commits to `attack`.

Write priority rules with `!` in `.pl` / `consult_string`. From GDScript,
`goal.cut()` builds `goal, !` (commit the choices so far):

```gdscript
var child = prolog.variable("Child")
var first_only = parent.call("tom", child).cut()
# parent(tom, Child), !  — one answer even if tom has several children

var action = prolog.variable("Action")
var first = prolog.solve(decide.call(action, 40)).first()
print(first.get(action))   # attack
```

---

## How Prologot maps the names

| You write in GDScript | Prolog name | Class / type |
|----------------------|-------------|--------------|
| `"tom"` in `call()` | atom | Godot `String` (on the way in) |
| `prolog.atom("tom")` | atom (explicit) | `PrologTerm` |
| `prolog.string("hi")` | Prolog string | `PrologTerm` |
| `prolog.integer(42)` | integer | `PrologTerm` |
| `prolog.variable("X")` | variable | `PrologVariable` (a kind of `PrologTerm`) |
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

`PrologTerm` = “a Prolog value as a Godot object”.
`PrologGoal` = “a PrologTerm used as a question” (plus `conjunction` / `disjunction` / `negated`).
`PrologVariable` = “a PrologTerm that is a hole”.
`PrologPredicate` = “a handle to a predicate **name**”; arity comes from `call()`.
`PrologQuery` = “the result of asking a goal” (`has_solution`, `first`, iterate).
`PrologSolution` = “one successful set of bindings”.

---

## What comes back from `solution.get(var)`

Going **in** (`call()`):

| GDScript | Becomes in Prolog |
|----------|-------------------|
| `String` | **always an atom** |
| `int` / `float` | number |
| `Array` | list |
| `PrologVariable` | variable |
| `PrologTerm` from `prolog.string()` | Prolog string |
| Node / Resource | `PrologObject` blob |

Coming **out** (`solution.get(...)`):

| Prolog | Becomes in GDScript |
|--------|---------------------|
| atom `hello` | Godot `String` `"hello"` |
| Prolog string `"hello"` | **`PrologTerm`**, not a `String` |
| integer / float | `int` / `float` |
| list | `Array` |
| compound | `Dictionary` `{functor, args}` |
| Godot blob | `PrologObject` |

This is the **retained hybrid policy** (implemented in `term_to_variant` / `variant_to_term`).

An atom **round-trips** as `String` → atom → `String`. Game code can compare and `match` names as strings.

A Prolog string does **not** become a `String`: you send it with `prolog.string("hello")` and get a `PrologTerm` back, so `hello` and `"hello"` stay distinguishable after `get()`.

`prolog.atom("tom")` builds an explicit `PrologTerm` for the **way in** (inspection, `as_text()`). Bindings never come back as that wrapper — wrapping atoms on the way out would break `== "bob"`.

`PrologObject` is unrelated: it is a Godot node identity, not another spelling of text.

---

## Three conversion philosophies

The question is only about **text-like** values: atom vs Prolog string vs Godot `String`. Numbers, lists, and `PrologObject` (a Godot node handle) are the same in all three.

Same knowledge for every example:

```prolog
named(hello).      % atom
msg("hello").      % Prolog string
```

Same GDScript question:

```gdscript
var who = prolog.variable("Who")
var from_atom = prolog.solve(named.call(who)).first().get(who)
var from_str  = prolog.solve(msg.call(who)).first().get(who)
```

### 1. Godot-first (output)

Both results become a Godot `String`. The atom / Prolog-string distinction exists **only on the way in**, via `prolog.string()`.

```text
from_atom → "hello"     (String)
from_str  → "hello"     (String)
```

If you send `from_str` back into `call()`, it becomes an **atom**, not a Prolog string. That round-trip is lossy, on purpose.

| For | Against |
|-----|---------|
| Game code always reads names as `String` — no `typeof` | A Prolog string and an atom look identical after `get()` |
| Matches how games use labels (`bob`, `flee`) | Faithful SWI code that *needs* `"hello"` ≠ `hello` must rebuild the string with `prolog.string()` |
| One rule to remember on output | Documented loss if you echo a Prolog string back in |

### 2. Prolog-first

Anything that is not a number, a list, or a Godot object comes back as a `PrologTerm` — **including atoms**.

```text
from_atom → PrologTerm (atom hello)
from_str  → PrologTerm (string "hello")
```

You would write `from_atom.get_atom()` (or similar) instead of using the value as a `String`.

| For | Against |
|-----|---------|
| One Godot type for “a Prolog value” | `get(child)` is never `"bob"` |
| Atom and Prolog string stay distinct | Every NPC name, action, zone id needs an extra unwrap |
| Round-trips stay faithful | Heavy for the 99 % game path |

### 3. Hybrid — **retained**

Atoms become `String` (the common case). Prolog strings become `PrologTerm` (to stay distinct from atoms).

```text
from_atom → "hello"        (String)
from_str  → PrologTerm     (not a String)
```

| For | Against |
|-----|---------|
| Honest about SWI: `hello` ≠ `"hello"` on the way out | “A piece of text” is two Godot types |
| Atoms (usual bindings) are already `String` | A rare Prolog string is not a `String` |
| `== "bob"` and `match` keep working | You must use `is PrologTerm` only for quoted Prolog strings |

**This is the policy.** Input: `String` → atom (`"X"` is never a variable). Output: atom → `String`, Prolog string → `PrologTerm`. Implemented in `src/PrologConversion.cpp` (`term_to_variant` / `variant_to_term`). Philosophies 1 and 2 are alternatives that were considered, not planned switches.

`PrologObject` is outside this choice: it is a Godot identity (instance id, lifetime), not another spelling of text.

---

## Related

- [Getting started](getting-started.md) — first program
- [Prolog developers](prolog-developers.md) — SWI name mapping
- [API — Type conversion](API.md#type-conversion)
