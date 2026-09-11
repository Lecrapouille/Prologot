# Prologot API Reference

Complete API documentation for the Prologot GDExtension.

---

## Key Prologot API Calls

The demo showcases these essential API methods:

```gdscript
# Initialization
prolog = Prologot.new()
prolog.initialize({"home": "linux/swipl"})

# Loading Prolog code
prolog.consult_file("path/to/file.pl")    # Load from file
prolog.consult_string("fact(data).")      # Load from string

# Object API: predicates, variables, goals, solutions
var parent = prolog.predicate("parent", 2)
var child = prolog.variable()
prolog.succeeds(parent.bind("tom", "bob"))         # bool — do not use solve() as a test
for solution in prolog.solve(parent.bind("tom", child)):
    print(solution.get(child))                     # "bob", then "liz"
var first = prolog.solve_one(parent.bind("tom", child))

# Optional term factories
prolog.atom("tom")
prolog.integer(42)
prolog.list([1, 2, 3])

# Dynamic facts
prolog.add_fact("new_fact(value)")        # Add fact
prolog.retract_fact("old_fact(value)")    # Remove fact
prolog.retract_all("pattern(_)")          # Remove all matching

# Function calls
prolog.call_predicate("name", ["arg1"])   # Call with args, returns bool
prolog.call_function("name", ["arg1"])    # Call and get result

# Cleanup
prolog.cleanup()
```

---

## Prologot Class

The main class providing SWI-Prolog integration for Godot 4. This class wraps the SWI-Prolog C API and exposes it to GDScript.

### Initialization and Cleanup

#### `initialize(options: Dictionary = {}) -> bool`

Initializes the SWI-Prolog engine with optional configuration.

This method performs the following steps:

1. Checks if already initialized (idempotent).
2. Parses the options Dictionary for configuration settings.
3. Sets up the SWI-Prolog home directory if provided.
4. Initializes the Prolog engine with the specified options.
5. Bootstraps helper predicates needed for `consult_string()`.

The bootstrap predicates enable loading Prolog code from strings by:

- Parsing multi-line Prolog code into individual clauses.
- Handling directives (`:-`) and queries (`?-`) appropriately.
- Asserting regular clauses into the knowledge base.

**Parameters:**

- `options` (Dictionary, optional): Configuration options (see below).

**Returns:** `true` if initialization succeeded, `false` otherwise.

**Initialization Options:**

Dictionary keys use spaces for better readability in GDScript.

**Main options:**

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `"home"` | String | "" | Path to SWI-Prolog installation |
| `"quiet"` | bool | true | Suppress informational messages |
| `"goal"` | String/Array | - | Goal(s) to execute at startup |
| `"toplevel"` | String | "" | Custom toplevel goal |
| `"init file"` | String | "" | User initialization file |
| `"script file"` | String | "" | Script source file to load |

**Performance options:**

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `"stack limit"` | String | "" | Prolog stack limit (e.g. "1g", "512m", "256k") |
| `"table space"` | String | "" | Space for SLG tables (e.g. "128m") |
| `"shared table space"` | String | "" | Space for shared SLG tables |
| `"optimized"` | bool | false | Enable optimized compilation |

**Behavior options:**

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `"traditional"` | bool | false | Traditional mode, disable SWI-Prolog v7 extensions |
| `"threads"` | bool | true | Allow threads |
| `"packs"` | bool | true | Attach add-ons/packages |

**Error handling options:**

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `"on error"` | String | "print" | Error handling style: "print" (display error), "halt" (display and log), "status" (store only, no display) |
| `"on warning"` | String | "print" | Warning handling style: "print" (display warning), "halt" (display and log), "status" (store only, no display) |

**Advanced options:**

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `"prolog flags"` | Dictionary | {} | Define Prolog flags |
| `"file search paths"` | Dictionary | {} | Define file search paths |
| `"custom args"` | Array | [] | Additional custom arguments |

**Usage examples:**

```gdscript
# Simple initialization (default, quiet mode)
prolog.initialize()

# Simple initialization with path to SWI-Prolog installation
prolog.initialize({"home": "res://bin/linux/swipl"})

# With welcome message
prolog.initialize({"quiet": false})

# Optimal configuration for a strategy game
prolog.initialize({
    "quiet": true,
    "optimized": true,
    "stack limit": "2g",      # 2 GB for complex AI
    "table space": "512m",    # 512 MB for memoization
    "threads": true,
    "on error": "print"       # Don't crash the game
})

# Development mode
prolog.initialize({
    "quiet": false,
    "on error": "print",
    "on warning": "print"
})

# AI system with automatic initialization
prolog.initialize({
    "script file": "res://ai/rules.pl",
    "goal": ["load_knowledge_base", "init_ai_agents"],
    "stack limit": "1g",
    "table space": "256m"
})

# Memory optimization for mobile
prolog.initialize({
    "quiet": true,
    "stack limit": "128m",
    "table space": "32m",
    "threads": false,         # No threads on mobile
    "packs": false            # No packages to reduce memory
})

# Advanced configuration with custom flags
prolog.initialize({
    "prolog flags": {
        "verbose": "silent",
        "max_depth": "1000"
    },
    "file search paths": {
        "game": "res://prolog",
        "data": "user://prolog_data"
    },
    "init file": "res://prolog/bootstrap.pl",
    "goal": "init_game_engine"
})

# Strict error handling for development
prolog.initialize({
    "on error": "halt",       # Display and log errors
    "on warning": "halt",     # Display and log warnings
    "traditional": false
})
```

#### `cleanup() -> void`

Cleans up and shuts down the Prolog engine.

This method is safe to call multiple times. It only performs cleanup if the engine was actually initialized. After cleanup, the engine must be re-initialized before use.

#### `is_initialized() -> bool`

Checks if the Prolog engine is currently initialized.

**Returns:** `true` if initialized and ready to use, `false` otherwise.

#### `get_last_error() -> String`

Gets the last error message from Prolog.

This method returns the last error message stored in `m_last_error`. Note: This method does not call `push_error()`. Errors are automatically handled according to the "on error" and "on warning" options set during initialization. Use this method to retrieve error messages for custom error handling or logging.

**Returns:** The last error message, or empty string if no error.

---

### File and Code Loading

#### `consult_file(filename: String) -> bool`

Loads a Prolog file into the knowledge base.

This method uses Prolog's built-in consult/1 predicate to load a .pl file. The file is parsed and all clauses are added to the knowledge base.

**Note:** Multiple calls to `consult_file()` and `consult_string()` accumulate clauses in the knowledge base. Each new file or code string adds its clauses to the existing knowledge base without removing previous ones. If you need to replace the knowledge base, use `retract_all()` to remove specific predicates first, or reinitialize the engine with `cleanup()` and `initialize()`.

**Parameters:**

- `filename` (String): Path to the Prolog file (.pl) to load. Supports `res://` and `user://` paths which are automatically converted to absolute filesystem paths.

**Returns:** `true` if the file was loaded successfully, `false` otherwise.

**Example:**

```gdscript
# Load a Prolog file containing facts and rules:
# File: family.pl
#   parent(tom, bob).
#   parent(bob, ann).
#   grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
prolog.consult_file("res://rules/family.pl")

# Subsequent calls add to the knowledge base:
prolog.consult_file("res://rules/game_rules.pl")  # Adds more clauses
prolog.consult_string("enemy(goblin, 10).")  # Adds even more clauses
# All clauses from both files and the code string are now available
```

#### `consult_string(code: String) -> bool`

Loads Prolog code from a string into the knowledge base.

This method uses the bootstrap predicate load_program_from_string/1 (created during initialization) to parse and load multi-line Prolog code. The code can contain multiple clauses, directives, and queries.

**Note:** Multiple calls to `consult_string()` and `consult_file()` accumulate clauses in the knowledge base. Each new code string adds its clauses to the existing knowledge base without removing previous ones. If you need to replace the knowledge base, use `retract_all()` to remove specific predicates first, or reinitialize the engine with `cleanup()` and `initialize()`.

**Parameters:**

- `code` (String): The Prolog code to load (can be multi-line).

**Returns:** `true` if the code was loaded successfully, `false` otherwise.

**Example:**

```gdscript
prolog.consult_string("""
    parent(tom, bob).
    parent(bob, ann).
    grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
""")

# Subsequent calls add to the knowledge base:
prolog.consult_string("enemy(goblin, 10).")
prolog.consult_file("res://rules/game_rules.pl")
# All clauses from both code strings and the file are now available
```

---

### Structured terms

Rules stay in Prolog source (`consult_file`, `consult_string`). Queries from GDScript are objects: a `PrologPredicate`, one or more `PrologVariable`, a `PrologGoal` from `bind()`, then `succeeds` / `solve` / `solve_one`.

A `String` passed to `bind()` is always an atom, never a variable. Only `prolog.variable()` (or `anonymous()`) creates a variable.

GDScript cannot write `parent("tom", child)` on a stored object, and `if []:` is true, so:

- use `parent.bind("tom", child)` (or `bindv([...])`) to build a goal
- use `succeeds(goal)` for a yes/no test — never treat the Array from `solve()` as a bool
- use `solution.get(child)` / `solution.has(child)` — object keys do not work via `_get`

`and` / `or` are GDScript keywords; composition is `conjunction()`, `disjunction()`, `negated()`.

#### Factories on `Prologot`

| Method | Result |
|--------|--------|
| `atom(name)` | Atom term |
| `integer(value)` / `real(value)` | Numbers |
| `string(value)` | Prolog string `"text"` — not the same as the atom from a GDScript `String` |
| `object(node)` | `PrologObject` handle (Node / Resource instance id, SWI blob) |
| `nil()` / `list(items)` | Empty list / list |
| `compound(functor, args)` | Compound term |
| `variable(name = "")` | Named or anonymous variable |
| `anonymous()` | Fresh anonymous variable |
| `predicate(name, arity)` | Reusable `PrologPredicate` |

#### `PrologPredicate.bind(...) -> PrologGoal`

Checks arity. `as_text()` prints `parent/2`.

#### `PrologGoal`

- `conjunction(other)` → `','/2`
- `disjunction(other)` → `';'/2`
- `negated()` → `'\+'/1`

#### `PrologSolution`

- `get(variable)` / `has(variable)`
- `get_bindings()` — Dictionary keyed by the `PrologVariable` objects
- `values()` — bound values only

---

### Query Execution

```gdscript
prolog.add_fact("parent(tom, bob)")
prolog.add_fact("parent(tom, liz)")

var parent = prolog.predicate("parent", 2)
var child = prolog.variable()

prolog.succeeds(parent.bind("tom", "bob"))   # true
prolog.succeeds(parent.bind("tom", "alice")) # false

for solution in prolog.solve(parent.bind("tom", child)):
    print(solution.get(child))               # bob, then liz
```

Prolog source strings are not a public query API. The editor dock parses text through an internal `_query_text_all()` helper.

#### `succeeds(goal: PrologGoal) -> bool`

Returns `true` if the goal has at least one solution.

#### `solve(goal: PrologGoal) -> Array`

Returns every `PrologSolution`. Empty if the goal fails. `solve_all()` is an alias.

#### `solve_one(goal: PrologGoal) -> Variant`

Returns the first `PrologSolution`, or `null` if none.

---

#### `object(value: Object) -> PrologObject`

Wraps a Godot `Node`, `Resource`, or other `Object`. Passing the same object to `bind()` wraps it automatically. `get_object()` / `is_valid()` follow the Godot instance id (the object may already be freed).

---

### Dynamic Facts

#### `assert_fact(goal: PrologGoal) -> bool`

Asserts a goal with `assertz/1`. This is the object API for facts.

```gdscript
var parent = prolog.predicate("parent", 2)
prolog.assert_fact(parent.bind("tom", "bob"))
```

#### `add_fact(fact: String) -> bool`

Adds a fact into the Prolog knowledge base.

This method adds a new clause to the knowledge base. The fact will be available for subsequent queries. Uses Prolog's assert/1 predicate.

**Note:** Do not include a trailing period ('.') in the fact string. If a period is present, it will be automatically removed. For example, `"parent(tom, bob)."` will be treated as `"parent(tom, bob)"`.

**Parameters:**

- `fact` (String): The Prolog fact to add (e.g., "likes(john, pizza)").

**Returns:** `true` if the fact was added successfully, `false` otherwise.

**Example:**

```gdscript
prolog.add_fact("parent(tom, bob)")
prolog.add_fact("game_state(level, 5)")
# Note: "parent(tom, bob)." also works (period is removed automatically)
```

#### `retract_fact(fact) -> bool`

Removes the first matching clause. `fact` is a `PrologGoal` or a legacy source string.

```gdscript
prolog.retract_fact(parent.bind("tom", "bob"))
prolog.retract_fact("parent(tom, bob)")  # legacy
```

#### `retract_all(pattern) -> bool`

Removes every matching clause. `pattern` is a `PrologGoal` or a legacy source string.

```gdscript
prolog.retract_all(parent.bind("tom", prolog.anonymous()))
prolog.retract_all("likes(_, _)")  # legacy
```

---

### Predicate Manipulation

#### `call_predicate(name: String, args: Array) -> bool`

Calls a Prolog predicate with arguments.

This method constructs a Prolog goal from a predicate name and arguments, then executes it. The arguments are converted from Godot Variants to Prolog terms automatically.

**Parameters:**

- `name` (String): Name of the predicate to call (e.g., "member").
- `args` (Array): Array of arguments to pass to the predicate.

**Returns:** `true` if the predicate call succeeded, `false` otherwise.

**Example:**

```gdscript
# Call member/2: member(3, [1, 2, 3, 4])
prolog.call_predicate("member", [3, [1, 2, 3, 4]])  # Returns true

# Call traditional Prolog predicates
prolog.call_predicate("assertz", ["parent(tom, bob)"])
prolog.call_predicate("retract", ["parent(tom, bob)"])
```

#### `call_function(name: String, args: Array) -> Variant`

Calls a Prolog predicate and returns a result value.

This method is similar to `call_predicate()`, but treats the predicate as a function that returns a value. The result is expected to be the last argument of the predicate.

**Parameters:**

- `name` (String): Name of the predicate to call.
- `args` (Array): Array of input arguments (result is the last argument).

**Returns:** The result value as a Variant, or `Variant()` if the call failed.

**Example:**

```gdscript
# Call length/2: length([1, 2, 3], N) returns N
var len = prolog.call_function("length", [[1, 2, 3]])
# Returns: 3

# Call arithmetic: plus(5, 3, Result) returns Result
var sum = prolog.call_function("plus", [5, 3])
# Returns: 8
```

---

### Introspection

#### `predicate_exists(name: String, arity: int) -> bool`

Checks if a predicate exists with the given arity.

This method uses PL_predicate() to look up a predicate. If the predicate doesn't exist, PL_predicate() returns 0 (NULL).

**Parameters:**

- `name` (String): Name of the predicate to check.
- `arity` (int): Number of arguments the predicate should have.

**Returns:** `true` if the predicate exists, `false` otherwise.

**Example:**

```gdscript
# Check if parent/2 exists
if prolog.predicate_exists("parent", 2):
    print("parent/2 is defined")

# Check if a built-in predicate exists
prolog.predicate_exists("member", 2)  # Returns true (built-in)
```

#### `list_predicates() -> Array`

Lists all currently defined predicates.

This method uses Prolog's current_predicate/1 to query for all predicates currently in the knowledge base.

**Returns:** Array of Dictionary objects describing each predicate.

**Example:**

```gdscript
# List all predicates in the knowledge base
var predicates = prolog.list_predicates()
# Returns: [{"functor": "parent", "args": ["/", 2]}, ...]
# Format: Name/Arity
```

---

## PrologotEngine Singleton (Autoload)

The singleton provides the same API as the Prologot class for use in game scripts. It wraps the Prologot class and exposes all the same methods.

**Note:** The singleton is available at runtime (when running the game). For editor plugins, use the Prologot class directly.

**Additional methods:** The singleton also provides `create_knowledge_base(name, code)` and `switch_knowledge_base(name)` for managing multiple knowledge bases. See [this file](use-cases.md) for an example of usage.

---

## Type Conversion Reference

Prologot automatically converts between Prolog terms and Godot Variants. The following tables show the complete mapping:

### Prolog Term → Godot Variant

| Prolog Term Type | Godot Variant Type | Notes |
|------------------|---------------------|-------|
| `PL_VARIABLE` (unbound) | `Variant()` (null) | Unbound variables cannot be converted to concrete values |
| `PL_ATOM` | `String` | Prolog atoms (e.g., `foo`, `bar`) become Godot strings |
| `PL_INTEGER` | `int` (int64_t) | Prolog integers are converted to 64-bit integers |
| `PL_FLOAT` | `float` (double) | Prolog floats are converted to double-precision floats |
| `PL_STRING` | `String` | Prolog strings (e.g., `"text"`) become Godot strings |
| `PL_NIL` | `Array()` (empty) | Empty list `[]` becomes an empty Array |
| `PL_LIST_PAIR` | `Array` | Prolog lists `[a, b, c]` become Godot Arrays |
| `PL_TERM` (list) | `Array` | Lists represented as compound terms become Arrays |
| `PL_TERM` (compound) | `Dictionary` | Compound terms become `{"functor": "name", "args": [...]}` |
| `PL_TERM` (atom `[]`) | `Array()` (empty) | Special case: atom `[]` becomes empty Array |

**Compound Terms Format:**

Compound terms (e.g., `parent(tom, bob)`) are converted to a Dictionary with:

- `"functor"`: The predicate name (e.g., `"parent"`)
- `"args"`: An Array of arguments (e.g., `["tom", "bob"]`)

**Example:**

```gdscript
# Prolog: parent(tom, bob)
# Godot: {"functor": "parent", "args": ["tom", "bob"]}
```

### Godot Variant → Prolog Term

| Godot Variant Type | Prolog Term Type | Notes |
|--------------------|------------------|-------|
| `Variant::NIL` | `PL_ATOM` (`[]`) | Null becomes the empty list atom `[]` |
| `Variant::BOOL` | `PL_ATOM` | `true` → `true`, `false` → `false` |
| `Variant::INT` | `PL_INTEGER` | Integers are converted to Prolog integers |
| `Variant::FLOAT` | `PL_FLOAT` | Floats are converted to Prolog floats |
| `Variant::STRING` | `PL_ATOM` | **Important:** Strings become Prolog atoms, not strings |
| `Variant::ARRAY` (empty) | `PL_NIL` | Empty Array becomes empty list `[]` |
| `Variant::ARRAY` (non-empty) | `PL_LIST_PAIR` | Arrays become Prolog lists `[elem1, elem2, ...]` |
| `Variant::DICTIONARY` | `PL_TERM` (compound) | Dictionary with `"functor"` and `"args"` becomes compound term |

**Dictionary Format for Compound Terms:**

To create a compound term from a Dictionary, use this format:

```gdscript
{
    "functor": "predicate_name",
    "args": [arg1, arg2, ...]
}
```

**Example:**

```gdscript
# Godot: {"functor": "parent", "args": ["tom", "bob"]}
# Prolog: parent(tom, bob)
```

**Important Notes:**

1. **Strings become atoms**: GDScript strings are converted to Prolog atoms (not Prolog strings). This is because atoms are more commonly used in Prolog. If you need Prolog strings, you'll need to handle this in your Prolog code.

2. **Recursive conversion**: Arrays and compound terms are converted recursively, so nested structures are fully supported.

3. **Anonymous variables**: Anonymous variables (`_`) in Prolog appear as `null` in Godot results.

4. **Unsupported types**: If a conversion is not supported, the method will return `Variant()` (null) or fail silently. If you encounter a missing conversion, please [open an issue](https://github.com/yourusername/Prologot/issues) so we can add support for it.

### Missing Conversions

If you need a conversion that is not currently supported, please don't hesitate to [open a bug report](https://github.com/lecrapouille/Prologot/issues) with:

- The Prolog term type or Godot Variant type you're trying to convert
- A minimal example showing the issue
- Your use case

We're actively working to improve type conversion support and welcome feedback!
