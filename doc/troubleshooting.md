# Troubleshooting

Common problems and how to fix them.

---

## "Failed to initialize Prologot"

**Problem:** The Prolog engine fails to start.

**Solution:**

- End players do **not** need SWI-Prolog installed if you ship the bundled runtime.
- Copy the entire `bin/` folder into your Godot project. Each OS subdirectory (`linux/`, `windows/`, `macos/`) should contain:
  - `libprologot.so` (Linux), `libprologot.dylib` (macOS), or `libprologot.dll` (Windows)
  - `libswipl.so`, `libswipl.dylib`, or `libswipl.dll` (SWI-Prolog runtime)
  - a `swipl/` folder with SWI resources (`boot.prc`, `library/`, …)
- Verify that `bin/prologot.gdextension` points to the correct library paths.
- In headless mode (`--headless`), ensure a `.godot` folder exists. Open the project once in the editor, or create `.godot/extension_list.cfg` manually:

```txt
res://bin/prologot.gdextension
```

Initialize from GDScript:

```gdscript
const PrologotBoot = preload("res://addons/prologot/prologot_boot.gd")

var prolog = PrologotBoot.create_engine()
# Or with an explicit home: PrologotBoot.create_engine("res://bin/linux/swipl")
if prolog == null:
    push_error("Prologot failed to start")
```

---

## Plugin not appearing

**Problem:** Prologot does not show up in the plugin list.

**Solution:**

1. Confirm `bin/prologot.gdextension` exists in the project root.
2. Confirm the `bin/` folder is at the project root.
3. Confirm `addons/prologot/` is present.
4. Restart Godot.
5. Check the Output panel for load errors.

---

## Prolog files not visible in the FileSystem dock

Godot hides unknown extensions by default. To show `.pl` files under `res://`, add `pl` (or `pro` / `prolog`) under **Editor → Editor Settings → Docks → FileSystem → Text File Extensions**:

![filesystem](pics/filesystem.png)

---

## Editor console not working

**Problem:** The Prologot Console dock is missing or queries fail.

**Solution:**

1. Enable the plugin under **Project → Project Settings → Plugins**.
2. Check **View → Docks** for the Prologot panel.
3. Read the Output panel for initialization messages (`Prologot: Editor engine initialized`) or home-path errors.
4. Reload the project if needed.
5. Remember the dock and `PrologotEngine` share one SWI process: facts loaded in the console persist when you press Play. Call `clear_knowledge()` or detach handles with `cleanup()` when you need a clean knowledge base.

---

## Variables not working in queries

**Problem:** Queries return no solutions, or a string behaves like a constant instead of a variable.

**Solution:**

A GDScript `String` passed to `call()` is always a Prolog **atom**. Only `prolog.variable()` creates a logical variable. `"Child"` is the atom `Child`, not a hole.

```gdscript
var parent = prolog.predicate("parent")

# Wrong: "Child" is the atom Child
prolog.solve(parent.call("tom", "Child"))

# Correct: a PrologVariable object
var child = prolog.variable("Child")
for solution in prolog.solve(parent.call("tom", child)):
    print(solution.get(child))
```

Use lowercase atoms in facts (`parent(tom, bob)`). Names that start with an uppercase letter in a `.pl` file must be quoted: `parent('Tom', 'Bob').`

A `PrologQuery` is always truthy in GDScript. Use `solve(goal).has_solution()` for yes/no tests — never `if prolog.solve(goal):`.

---

## Performance issues

**Problem:** Queries are slow or memory use is high.

**Solution:**

1. Optimize the Prolog rules (indexing, cuts, smaller search spaces).
2. Limit resources at initialization:

```gdscript
prolog.initialize({
    "stack limit": "512m",
    "table space": "128m",
    "optimized": true,
})
```

3. `solve()` is lazy: prefer `first()`, `for` with `break`, or `solve(goal, n)` over `all()` on huge or infinite domains (for example `between(1, inf, N)`).
4. Use `retract_all()` to remove stale dynamic facts.

Do not call `cleanup()` while a `PrologQuery` from `solve()` is still open (for example mid-`for`). Temporary yes/no checks with `has_solution()` or `first()` are safe — they cut remaining choice points.

---

## "Could not find system resources" (boot.prc missing)

**Problem:** Godot or SWI reports an invalid home directory or missing `boot.prc`:

```text
ERROR: Prologot: Invalid SWI-Prolog home directory: boot.prc not found in: ...
FATAL ERROR: Could not find system resources
```

**Solution:**

1. **Shipping a game:** copy the full `bin/<os>/` folder from a Prologot build, including `swipl/boot.prc`. Initialize with:

```gdscript
const PrologotBoot = preload("res://addons/prologot/prologot_boot.gd")
var prolog = PrologotBoot.create_engine()
```

2. **Building from source:** install SWI-Prolog (`make install-swi`), then run `make all`. SCons copies the runtime into `bin/<os>/swipl/`.

3. **Asset Library install:** use the packaged `bin/` layout as shipped — do not point `"home"` at an empty folder.

4. **Custom location:** pass the folder that directly contains `boot.prc`:

```gdscript
var prolog = PrologotBoot.create_engine("res://path/to/swipl")
if prolog == null:
    push_error("Prologot failed to start")
```

When `"home"` is omitted, Prologot tries `res://bin/<os>/swipl`, then falls back to a system SWI-Prolog install.
