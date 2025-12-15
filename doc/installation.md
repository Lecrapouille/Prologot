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
| `make debug` | Compile the extension in debug mode. |
| `make release` | Compile the extension in release mode for performance. |
| `make all` | Build both debug and release versions and set up demo and test projects. |

**Notes:**

- By default, it builds for Godot 4.5. To use another version: `make GODOT_CPP=4.4 debug` or `make GODOT_CPP=4.4 all`.
- The `godot-cpp` library is automatically compiled when needed during the build process. You don't need to compile it separately.
- The build system uses SCons with `--godot-cpp=` option internally. You can also use SCons directly: `scons --godot-cpp=4.4 --target=template_release`.

### 3. Other Useful Commands

```bash
make help          # Show all available commands
make install-swi   # Install SWI-Prolog automatically (Linux/macOS)
make check-deps    # Verify dependencies
make clean         # Clean build artifacts
make format        # Format C++ sources
make update        # Update godot-cpp to a specific version (without compiling)
make init-projects # Set up demo and test projects
make run-demo      # Run the demo project in Godot
make tests         # Run tests
```

## Installing Prologot in Your Project

To use Prologot in your own project:

1. Copy `prologot.gdextension` to your project root
2. Copy the `bin/` folder to your project
3. Copy `addons/prologot/` to your project's `addons/` folder
4. Enable the plugin in **Project → Project Settings → Plugins**

## Verifying Installation

After installation, you can verify that Prologot is working by:

1. Opening your project in Godot
2. Going to **Project → Project Settings → Plugins** and enabling "Prologot"
3. Opening the Prologot Console dock (should appear in the bottom-right panel)
4. Loading some Prolog code and executing a query

If you see no errors, the installation was successful!
