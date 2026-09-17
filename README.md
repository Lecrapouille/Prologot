[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT) [![Godot 4.2+](https://img.shields.io/badge/Godot-4.2%2B-blue.svg)](https://godotengine.org/) [![SWI-Prolog](https://img.shields.io/badge/SWI--Prolog-8.0%2B-orange.svg)](https://www.swi-prolog.org/) [![Version](https://img.shields.io/badge/version-0.2.0-green.svg)](https://github.com/yourusername/Prologot/releases)

# Prologot

<div align="center">
  <img src="doc/logos/Prologot.png" alt="Prologot Logo" width="200">
</div>

**[Prologot](https://github.com/Lecrapouille/Prologot)** is a [Godot 4](https://godotengine.org/) extension that embeds [SWI-Prolog](https://www.swi-prolog.org/), bringing logic programming to your games on Linux, macOS, and Windows. Use Prolog for AI decision-making, dialogue systems, rule engines, pathfinding, and more. This extension offers:

- SWI-Prolog integration via GDExtension
- Interactive Prolog console in the Godot editor
- Query execution with variable bindings
- Dynamic fact assertion and retraction
- Load Prolog from files or code strings
- Knowledge-base management
- Type conversion between Prolog terms and Godot Variants

**Prolog** is a logic programming language. You describe **facts** and **rules**, then ask **questions** (queries). The engine searches for values that make a question true — it does not run a fixed sequence of instructions like GDScript.

Think of it as a database plus inference:

- **Facts** are records you assert: `parent(tom, bob).`
- **Rules** derive new truths: “X is a grandparent of Z if X is parent of Y and Y is parent of Z.”
- **Queries** ask whether something holds, or **which values** make it hold: `parent(tom, Who)?` → `Who = bob`, then `Who = liz`.

---

## Documentation

- **Getting started**: Follow the **[Installation Guide](doc/installation.md)** to set everything up. Quick build on Ubuntu:

```bash
sudo apt-get install g++ make pkg-config swi-prolog swi-prolog-nox
pip install scons

git clone https://github.com/Lecrapouille/Prologot.git
cd Prologot
make GODOT_CPP=4.5 all
```

- **Try the demo**: Once built, run the **[Interactive Demo](demos/showcases/README.md)** to see Prologot execute Prolog examples and display the results:

```bash
make run-demo
```

- **Mini Dungeon**: A short dungeon crawler whose monster AI is powered by Prolog:

```bash
make run-mini-dungeon
```

- **Galactic Customs**: A logic puzzle where you play customs officer and approve or deny galactic travelers:

```bash
make run-galactic_customs
```

- **Learn Prolog and Prologot**: The **[Glossary](doc/glossary.md)** introduces Prolog basics; **[Getting Started](doc/getting-started.md)** walks you through Prologot. **Prolog veterans**: see **[Notes for Prolog Developers](doc/prolog-developers.md)** for switching from SWI-Prolog to Prologot—API mapping and common pitfalls.

- **Build your game**: The **[API Reference](doc/API.md)** covers the full syntax. **[Use Cases](doc/use-cases.md)** suggests patterns for game logic. For a minimal script-only sample, see [hello_world_prologot.gd](doc/hello_world_prologot.gd).

---

## Project Structure

```sh
Prologot/
├── src/                              # C++ GDExtension source
│   ├── Prologot.hpp / Prologot.cpp   # Core engine: init, consult, assert, solve
│   ├── PrologQuery.hpp / .cpp        # Lazy query handle (PrologQuery)
│   ├── PrologGoal.hpp / .cpp         # Goal builder (PrologGoal)
│   ├── PrologTerm.hpp / .cpp         # Term wrapper (PrologTerm)
│   ├── PrologSolution.hpp / .cpp     # Solution bindings (PrologSolution)
│   ├── PrologPredicate.hpp / .cpp    # Predicate builder (PrologPredicate)
│   ├── PrologVariable.hpp / .cpp     # Logic variable (PrologVariable)
│   ├── PrologObject.hpp / .cpp       # Opaque Prolog term handle (PrologObject)
│   ├── PrologConversion.hpp / .cpp   # Prolog ↔ Godot Variant conversion
│   ├── PrologExposure.cpp            # expose_property / expose_method bridge
│   ├── register_types.h              # GDExtension registration header
│   └── register_types.cpp            # Registers native classes with Godot
├── tests/                            # Headless regression suite
│   ├── run_tests.gd                  # Test runner (SceneTree entry point)
│   ├── test_prologot.gd              # Unit tests
├── addons/prologot/                  # Godot editor plugin
│   ├── plugin.cfg                    # Plugin manifest
│   ├── plugin.gd                     # EditorPlugin entry point
│   ├── prologot_boot.gd              # Shared initialize() and bundled SWI home
│   ├── prologot_facade.gd            # GDScript wrapper around the native API
│   ├── prologot_singleton.gd         # Autoload (PrologotEngine) and named KBs
│   ├── prologot_node.gd              # Scene-tree node wrapper (PrologotNode)
│   ├── prolog_knowledge.gd           # Inspectable knowledge-base resource
│   └── prologot_dock.gd              # In-editor Prolog console UI
├── demos/                            # Sample Godot projects
│   ├── showcases/                    # Interactive API walkthrough
│   │   ├── examples/*.pl             # 01–06: queries, rules, pathfinding, AI, …
│   │   ├── prologot-demos.gd         # Demo UI and logic
│   │   └── prologot-demos.tscn       # Demo scene
│   ├── mini_dungeon/                 # Dungeon crawler with Prolog-driven AI
│   │   ├── prolog/*.pl               # dungeon, combat, ai, rules
│   │   ├── scripts/*.gd              # player, goblin, dungeon, door, …
│   │   └── ui/procedural_actor.gd    # Procedural pixel-art rendering
│   └── galactic_customs/             # Customs-booth logic puzzle
│       ├── rules/galactic_customs.pl # Base knowledge base
│       ├── scripts/main.gd           # Game loop and Prolog integration
│       ├── scripts/alien_sprite.gd   # Alien portrait display
│       └── ui/galactic_atlas.gd      # Sprite atlas + button skinning
├── doc/                              # Guides, API reference, and examples
│   └── hello_world_prologot.gd       # Minimal script-only sample
├── bin/                              # Build output (created by make / scons)
│   ├── linux/                        # .so + bundled swipl/ runtime
│   ├── windows/                      # .dll + bundled swipl/ runtime
│   ├── macos/                        # .dylib + bundled swipl/ runtime
│   └── prologot.gdextension          # Generated GDExtension manifest
├── godot-cpp-*/                      # godot-cpp bindings (cloned by SConstruct)
├── SConstruct                        # Build: compile, copy SWI runtime, generate .gdextension
└── Makefile                          # Shortcuts (build, test, run demos)
```

## License

This project is licensed under the MIT License—see the [LICENSE](LICENSE) file for details.

Parts of this project were developed with AI assistance.
