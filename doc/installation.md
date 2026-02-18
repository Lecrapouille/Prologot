# Installation Guide

Complete installation instructions for Prologot.

## Prerequisites

- [Godot Engine 4.2+](https://godotengine.org/) - The game engine.
- [SWI-Prolog 8.0+](https://www.swi-prolog.org/) - The Prolog implementation.
- [godot-cpp](https://github.com/godotengine/godot-cpp) - C++ bindings for GDExtension (automatically cloned during build).
- [SCons](https://scons.org/) - The building system used by Godot and therefore for this project.
- [Python 3](https://www.python.org/) - Since Scons is based on Python.
- [C++ compiler](https://gcc.gnu.org/) - For building C++ sources, and optionally a Makefile.
- [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) - For detecting SWI-Prolog compiler and linker flags.

**Note:** The build script may ask your sudo password to install operating system packages.

## Installing SWI-Prolog and pkg-config

### Linux (Debian/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install swi-prolog swi-prolog-nox pkg-config
```

### Linux (Arch)

```bash
sudo pacman -S swi-prolog pkgconf
```

### macOS

```bash
brew install swi-prolog pkg-config
```

### Windows

- Download [SWI-Prolog 8.0+](https://www.swi-prolog.org/download/stable).
- Add to PATH during installation.
- Install pkg-config via [MSYS2](https://www.msys2.org/) or use vcpkg.

## Installing SCons

```bash
pip install scons
```

## Building Prologot

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/Prologot.git
cd Prologot
```

### 2. Build the Extension

The simplest way to build the project is using the **Makefile**, which directly calls **SCons** (no intermediate script needed).

| Command | Description |
|---------|-------------|
| `make help` | Show all available commands. |
| `make install-swi` | Install SWI-Prolog automatically. |
| `make check-deps` | Verify dependencies for building. |
| `make debug` | Compile the extension in debug mode and set up demo and test projects. |
| `make release` | Compile the extension in release mode and set up demo and test projects. |
| `make all` | Build both debug and release versions and set up demo and test projects. |
| `make run-demo` | Run the demo project in Godot. |
| `make run-galactic` | Run the demo game in Godot. |

**Notes:**

- By default, it builds for Godot 4.5. To use another version pass `GODOT_CPP=4.4` to make command.
- By default, the build folder name is `bin`. To use another name pass `BIN=prologot_artifacts` to make command.
- The `godot-cpp` library is automatically downloaded and compiled when needed during the build process. You don't need to compile it separately.
- The build system uses SCons with `--godot-cpp=` option internally. You can also use SCons directly: `scons --godot-cpp=4.4 --target=template_release`.

### 3. Other Useful Commands

```bash
make clean           # Clean build compiled files and Prologot artifacts
make format          # Format C++ sources (for developers)
make setup-demos     # Link Godot demos to Prologot artifacts (done automatically)
make tests           # Run tests (for developers)
```

## Installing Prologot in Your Project

To use Prologot in your own project:

1. Copy `addons/prologot/` to your project's `addons/` folder.
2. Copy the `bin/` folder to your project (it contains the compiled libraries and `prologot.gdextension`).
3. Enable the plugin in **Project → Project Settings → Plugins**
5. Optionally, you can move the folder `bin/swipl`  in another folder. You have to adapt, in your gdscript file, the `initialize` function and set `"home"` to your new path.

```gdscript
prolog = Prologot.new()
prolog.initialize({"home": "res://bin/swipl"})
```

## Verifying Installation

After installation, you can verify that Prologot is working by:

1. Opening your project in Godot
2. Going to **Project → Project Settings → Plugins** and enabling "Prologot"
3. Opening the Prologot Console dock (should appear in the bottom-right panel)
4. Loading some Prolog code and executing a query

If you see no errors, the installation was successful!
