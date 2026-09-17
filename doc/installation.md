# Installation Guide

How to build Prologot from source and add it to a Godot project.

## Prerequisites

| Requirement | Purpose |
|-------------|---------|
| [Godot Engine 4.2+](https://godotengine.org/) | Run demos, tests, and your game |
| [SWI-Prolog 8.0+](https://www.swi-prolog.org/) | **Build time only** — copied into `bin/<os>/` by SCons |
| [Python 3](https://www.python.org/) | Required by SCons |
| [SCons](https://scons.org/) | Build system |
| C++ toolchain | `g++` / Clang — compile the GDExtension |
| `pkg-config` | Locate SWI-Prolog during the build (Linux/macOS) |

[godot-cpp](https://github.com/godotengine/godot-cpp) is cloned automatically by SConstruct; you do not need to install it yourself.

**Note:** `make install-swi` may use `sudo` to install OS packages when SWI-Prolog is missing.

---

## Install build dependencies

### Debian / Ubuntu

```bash
sudo apt-get install g++ make pkg-config swi-prolog swi-prolog-nox
python3 -m pip install scons
```

### Fedora

```bash
sudo dnf install gcc-c++ make python3-scons pkgconf pl pl-devel libstdc++-static
python3 -m pip install scons
```

### macOS

```bash
brew install scons swi-prolog pkg-config
```

### Windows

1. Install [SWI-Prolog 8.0+](https://www.swi-prolog.org/Download.html) and add it to `PATH`.
2. Install Python 3.
3. Install SCons:

```powershell
python -m pip install scons
```

---

## Build from source

### 1. Clone the repository

```bash
git clone https://github.com/Lecrapouille/Prologot.git
cd Prologot
```

### 2. Build (Linux and macOS)

The **Makefile** wraps SCons and sets up demo/test projects.

| Command | Description |
|---------|-------------|
| `make help` | List all targets |
| `make install-swi` | Install SWI-Prolog via the OS package manager |
| `make check-deps` | Verify SWI-Prolog and build tools |
| `make debug` | Build debug libraries + run `setup-demos` |
| `make release` | Build release libraries + run `setup-demos` |
| `make all` | Build debug **and** release + run `setup-demos` |
| `make tests` | Run the headless regression suite |
| `make run-demo` | Open the showcases demo in Godot |
| `make run-mini-dungeon` | Open the Mini Dungeon demo |
| `make run-galactic` | Open the Galactic Customs demo |

Quick start:

```bash
make GODOT_CPP=4.5 all
```

**Configuration**

| Variable | Default | Description |
|----------|---------|-------------|
| `GODOT_CPP` | `4.5` | godot-cpp version passed to SCons (`4.4`, `4.3.1`, …) |
| `BIN` | `bin` | Output directory for libraries and `prologot.gdextension` |
| `JOBS` | CPU count | Parallel SCons jobs (`make JOBS=16 all`) |

**Notes**

- `setup-demos` creates symlinks so demos and tests share the built `bin/` folder and `addons/`.
- godot-cpp is downloaded on first build when missing.
- You can call SCons directly:

```bash
scons --godot-cpp=4.5 target=template_debug
scons --godot-cpp=4.5 target=template_release
```

**Other useful commands**

```bash
make clean        # Remove bin/, godot-cpp build cache, demo symlinks
make format       # clang-format on C++ sources
make setup-demos  # Re-link bin/ into demos/ and tests/ (after a build)
```

### 3. Build (Windows)

From a shell where `scons` and SWI-Prolog are on `PATH`:

```powershell
scons --godot-cpp=4.5 target=template_debug arch=x86_64
scons --godot-cpp=4.5 target=template_release arch=x86_64
```

Then copy `bin/` and `addons/prologot/` into your Godot project manually (there is no Windows Makefile in this repo).

---

## Add Prologot to your Godot project

Distribution layout after a successful build:

```text
your_project/
├── addons/prologot/          # GDScript plugin (editor dock, autoload, helpers)
└── bin/
    ├── prologot.gdextension  # Generated manifest
    ├── linux/                # .so + libswipl + swipl/ runtime
    ├── windows/              # .dll + libswipl + swipl/ runtime
    └── macos/                # .dylib + libswipl + swipl/ runtime
```

Steps:

1. Copy `addons/prologot/` into your project’s `addons/` folder.
2. Copy the whole `bin/` folder to your project root (libraries **and** bundled `swipl/` resources).
3. Enable **Project → Project Settings → Plugins → Prologot**.
4. Open the **Prologot** dock in the editor (bottom panel).

**Runtime initialization**

End players do **not** need SWI-Prolog installed if you ship the bundled `bin/<os>/swipl/` folder.

The plugin and autoload use `prologot_boot.gd`, which picks `res://bin/<os>/swipl` when it exists. In your own scripts you can do the same:

```gdscript
const PrologotBoot = preload("res://addons/prologot/prologot_boot.gd")

var prolog = PrologotBoot.create_engine()
if prolog == null:
    push_error("Prologot failed to start")
    return
```

Custom SWI home:

```gdscript
var prolog = PrologotBoot.create_engine("res://path/to/swipl")
if prolog == null:
    push_error("Prologot failed to start")
    return
```

Minimal example:

```gdscript
const PrologotBoot = preload("res://addons/prologot/prologot_boot.gd")

var prolog = PrologotBoot.create_engine()
prolog.consult_string("""
parent(tom, bob).
parent(tom, liz).
""")

var parent = prolog.predicate("parent")
var child = prolog.variable("Child")
prolog.solve(parent.call("tom", "bob")).has_solution()
for solution in prolog.solve(parent.call("tom", child)):
    print(solution.get(child))

prolog.cleanup()
```

---

## Verify the installation

1. Open the project in Godot.
2. Enable the Prologot plugin.
3. Open the **Prologot** editor dock.
4. Run a query, for example: `member(X, [a, b, c]).`

If the dock loads and queries return results, the installation is working.

For common failures (missing `boot.prc`, headless mode, plugin not listed), see **[Troubleshooting](troubleshooting.md)**.
