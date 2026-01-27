#!/usr/bin/env python
import sys
import os
import subprocess
import shutil
from glob import glob
from pathlib import Path

MIN_GODOT_VERSION = "4.2"

# ============================================================================
# SCons Options
# ============================================================================
AddOption('--godot-cpp', dest='godot_cpp', type='string', default=None,
          metavar='VERSION', help='Godot-cpp version (e.g., 4.5)')
AddOption('--force', dest='force', action='store_true', default=False,
          help='Force re-clone of godot-cpp')
AddOption('--skip-build', dest='skip_build', action='store_true', default=False,
          help='Skip building')
AddOption('--gdextension', dest='gdextension', type='string', default=None,
          metavar='PATH', help='Generate prologot.gdextension by scanning bin/ in PATH')

# ============================================================================
# Setup godot-cpp
# ============================================================================
def setup_godot_cpp(version, force=False):
    """Clone or update godot-cpp repository."""
    godot_cpp_dir = Path(f'godot-cpp-{version.replace(".", "_")}')
    git_tag = f"godot-{version}-stable" if '.' in version else version

    if godot_cpp_dir.exists():
        if force:
            print(f"Removing {godot_cpp_dir}")
            shutil.rmtree(godot_cpp_dir)
        else:
            print(f"godot-cpp directory already exists: {godot_cpp_dir}")
            return str(godot_cpp_dir)

    print(f"Cloning godot-cpp ({git_tag})...")
    subprocess.run(['git', 'clone', '--recursive', '-b', git_tag,
                    'https://github.com/godotengine/godot-cpp', str(godot_cpp_dir)],
                   check=True)
    return str(godot_cpp_dir)

def find_or_setup_godot_cpp():
    """Find or configure godot-cpp."""
    godot_cpp_ref = GetOption('godot_cpp')

    if godot_cpp_ref:
        return setup_godot_cpp(godot_cpp_ref, GetOption('force'))

    # Find the first godot-cpp-* directory
    godot_cpp_dirs = list(Path('.').glob('godot-cpp-*'))
    if not godot_cpp_dirs:
        print("Error: No godot-cpp-* directory found!")
        print("Run: scons --godot-cpp=4.5")
        Exit(1)

    return str(godot_cpp_dirs[0])

# ============================================================================
# Find SWI-Prolog
# ============================================================================
def get_swipl_paths():
    """Get SWI-Prolog installation paths by running 'swipl --dump-runtime-variables'."""
    swipl_candidates = ['swipl']

    if sys.platform == 'win32':
        swipl_candidates = [
            r'C:\Program Files\swipl\bin\swipl.exe',
            r'C:\Program Files (x86)\swipl\bin\swipl.exe',
            'swipl'
        ]

    for swipl_cmd in swipl_candidates:
        try:
            result = subprocess.run([swipl_cmd, '--dump-runtime-variables'],
                                    capture_output=True, text=True, check=True)
            paths = {}
            for line in result.stdout.strip().split('\n'):
                if '=' in line:
                    key, val = line.split('=', 1)
                    paths[key] = val.strip('";')
            return paths
        except (subprocess.CalledProcessError, FileNotFoundError):
            continue

    print("Error: SWI-Prolog not found!")
    if sys.platform == 'win32':
        print("Install with: choco install swi-prolog")
    elif sys.platform == 'darwin':
        print("Install with: brew install swi-prolog")
    else:
        print("Install with: sudo apt-get install swi-prolog")
    Exit(1)

# ============================================================================
# Copy Shared Library
# ============================================================================
def copy_shared_lib(lib_path: Path, bin_dir: Path, copied_files: list):
    """Copy a shared library, resolving symlinks but keeping the symlink name."""
    if not lib_path.exists():
        return False

    dest = bin_dir / lib_path.name

    # Remove destination if it exists to avoid permission errors
    if dest.exists():
        dest.chmod(0o666)
        dest.unlink()

    shutil.copy2(lib_path.resolve(), dest)
    copied_files.append(str(dest))
    return True

# ============================================================================
# Copy SWI-Prolog Libraries (Windows)
# ============================================================================
def copy_swipl_libraries_windows(paths, bin_dir, copied_files):
    """Copy SWI-Prolog DLL and import library on Windows."""
    plbase = Path(paths.get("PLBASE", ""))
    search_dirs = [plbase / sub for sub in ("bin", "lib", "lib/x64") if (plbase / sub).exists()]

    # Find and copy DLL
    for dll_name in ("libswipl.dll", "swipl.dll"):
        for d in search_dirs:
            if (dll := d / dll_name).exists():
                shutil.copy2(dll, bin_dir / dll_name)
                copied_files.append(str(bin_dir / dll_name))
                break
        else:
            continue
        break

    # Find and copy import library
    for lib_name in ("swipl.lib", "libswipl.lib", "libswipl.dll.a"):
        for d in search_dirs:
            if (lib := d / lib_name).exists():
                shutil.copy2(lib, bin_dir / "swipl.lib")
                copied_files.append(str(bin_dir / "swipl.lib"))
                return
        else:
            continue
        break

# ============================================================================
# Copy SWI-Prolog Libraries (Unix and MacOS)
# ============================================================================
def copy_swipl_libraries_unix(paths, bin_dir, copied_files):
    """Copy SWI-Prolog shared library on Unix/macOS."""
    # Try PLLIBSWIPL first
    if pllibswipl := paths.get("PLLIBSWIPL"):
        lib_path = Path(pllibswipl)
        copy_shared_lib(lib_path, bin_dir, copied_files)

        # On macOS, also copy as libswipl.dylib for linking
        if sys.platform == 'darwin' and lib_path.name != 'libswipl.dylib':
            base_dest = bin_dir / 'libswipl.dylib'
            if base_dest.exists():
                base_dest.chmod(0o666)
                base_dest.unlink()
            shutil.copy2(lib_path.resolve(), base_dest)
            copied_files.append(str(base_dest))
        return

    # Fallback to PLLIBDIR
    print("Warning: PLLIBSWIPL not found, falling back to PLLIBDIR")
    plbase = paths.get("PLBASE", "")
    lib_dir = Path(paths.get("PLLIBDIR") or plbase) / "lib"

    for pattern in ("libswipl.so*", "libswipl.dylib*"):
        for lib in lib_dir.glob(pattern):
            if lib.is_file():
                copy_shared_lib(lib, bin_dir, copied_files)
                return

    print("Error: No SWI-Prolog shared library found")

# ============================================================================
# Copy SWI-Prolog Libraries (Windows, MacOS and Unix)
# ============================================================================
def copy_swipl_libraries(paths):
    """Copy the necessary SWI-Prolog libraries into the bin/ directory."""
    bin_dir = Path("bin")
    bin_dir.mkdir(parents=True, exist_ok=True)

    copied_files = []
    if sys.platform == "win32":
        copy_swipl_libraries_windows(paths, bin_dir, copied_files)
    else:
        copy_swipl_libraries_unix(paths, bin_dir, copied_files)
    return copied_files

# ============================================================================
# Copy SWI-Prolog Resources (boot.prc, library/, etc.)
# ============================================================================
def copy_swipl_resources(paths, output_dir=None):
    """Copy SWI-Prolog runtime resources for standalone distribution.

    Args:
        paths: SWI-Prolog paths from get_swipl_paths()
        output_dir: Target directory for swipl resources (default: bin/swipl)

    Returns:
        List of copied file paths
    """
    output_dir = Path(output_dir or "bin/swipl")
    plbase = Path(paths.get("PLBASE", ""))

    if not plbase.exists():
        print(f"Warning: PLBASE directory not found: {plbase}")
        return []

    output_dir.mkdir(parents=True, exist_ok=True)
    copied = []

    # Copy boot*.prc file
    boot_files = list(plbase.glob("boot*.prc"))
    if boot_files:
        boot_dest = output_dir / "boot.prc"
        shutil.copy2(boot_files[0], boot_dest)
        copied.append(str(boot_dest))
        print(f"Copied {boot_files[0].name} -> {boot_dest}")
    else:
        print(f"Warning: No boot*.prc found in {plbase}")

    # Copy directories: library/ and lib/
    for dir_name in ("library", "lib"):
        src = plbase / dir_name
        if src.exists() and src.is_dir():
            dest = output_dir / dir_name
            if dest.exists():
                shutil.rmtree(dest)
            shutil.copytree(src, dest)
            copied.append(str(dest))
            print(f"Copied {dir_name}/ -> {dest}")
        else:
            print(f"Warning: {dir_name}/ not found in {plbase}")

    return copied

# ============================================================================
# Setup SCons Environment for SWI-Prolog
# ============================================================================
def configure_swipl(env, paths):
    """Configure the SCons environment for SWI-Prolog."""
    plbase = paths.get('PLBASE', '')

    # Add 'include' directory to CPPPATH if it exists
    include_dir = Path(plbase) / 'include'
    if include_dir.exists():
        env.Append(CPPPATH=[str(include_dir)])

    # Copy the necessary libraries into bin/
    copied_files = copy_swipl_libraries(paths)
    if not copied_files:
        raise RuntimeError("Error: No SWI-Prolog libraries copied")
    for file in copied_files:
        print(f"Copied {file}")

    # Copy SWI-Prolog resources (boot.prc, library/) for standalone distribution
    copied_resources = copy_swipl_resources(paths)
    if not copied_resources:
        print("Warning: No SWI-Prolog resources copied")

    # Add bin/ to library search path
    bin_dir = os.path.abspath('bin')
    env.Append(LIBPATH=[bin_dir])

    if sys.platform == 'win32':
        # On Windows, link with swipl.lib
        env.Append(LIBS=['swipl'])
    elif sys.platform == 'darwin':
        # On macOS, link with libswipl and set @loader_path rpath
        env.Append(LIBS=['swipl'])
        env.Append(LINKFLAGS=['-Wl,-rpath,@loader_path'])
    else:
        # On Linux, link with libswipl and set rpath to $ORIGIN
        env.Append(LIBS=['swipl'])
        env.Append(RPATH=['$$ORIGIN'])

# ============================================================================
# Create prologot.gdextension
# ============================================================================
def create_gdextension_file(base_path=".", verbose=False):
    """Generate prologot.gdextension by scanning bin/ directory."""
    bin_dir = Path(base_path) / "bin"
    if not bin_dir.exists():
        return

    # Scan for prologot libraries and swipl dependencies
    libraries = []
    dependencies = []

    for lib in bin_dir.iterdir():
        if not lib.is_file() or lib.suffix in ('.lib', '.a', '.exp'):
            continue
        if lib.name.startswith("libprologot"):
            key = lib.stem.replace("libprologot.", "").replace("template_", "")
            libraries.append((key, lib.name))
        elif "swipl" in lib.name:
            dependencies.append(lib.name)

    # Build content
    content = [
        "[configuration]",
        'entry_symbol = "prologot_library_init"',
        f'compatibility_minimum = "{MIN_GODOT_VERSION}"',
        "reloadable = true",
        "",
        "[libraries]",
        ""
    ]

    content.extend(f'{key} = "bin/{name}"' for key, name in sorted(libraries))

    # Add dependencies
    if dependencies:
        content.extend(["", "[dependencies]", ""])
        platform_map = {"windows": ".dll", "linux": ".so", "macos": ".dylib"}

        for key, _ in sorted(libraries):
            platform = key.split('.')[0]
            suffix = platform_map.get(platform, "")
            dep = next((d for d in dependencies if suffix and d.endswith(suffix)), None)
            if dep:
                content.append(f'{key} = {{"bin/{dep}": ""}}')

    output_path = Path(base_path) / "prologot.gdextension"
    output_path.write_text('\n'.join(content) + '\n')

    if verbose:
        print(f"prologot.gdextension: {len(libraries)} lib(s), {len(dependencies)} dep(s)")
        print('\n'.join(content))


# ============================================================================
# Build Functions
# ============================================================================
def build_library(env):
    """Build the Prologot library."""
    # Configure build paths
    env.Append(CPPPATH=["src/"])
    configure_swipl(env, get_swipl_paths())

    # Determine library name
    if env["platform"] == "macos":
        lib_name = f"bin/libprologot.{env['platform']}.{env['target']}.{env['arch']}"
    else:
        lib_name = f"bin/libprologot{env['suffix']}{env['SHLIBSUFFIX']}"

    # Build and post-process
    library = env.SharedLibrary(lib_name, source=glob("src/*.cpp"))
    env.AddPostAction(library, lambda target, source, env: create_gdextension_file())

    Default(library)
    return library

# ============================================================================
# Main Entry Point
# ============================================================================
if GetOption('gdextension'):
    create_gdextension_file(GetOption('gdextension'), verbose=True)
    Exit(0)

if GetOption('skip_build'):
    print("Build skipped (--skip-build)")
    Exit(0)

# Setup godot-cpp and build
godot_cpp_dir = find_or_setup_godot_cpp()
sys.path.insert(0, godot_cpp_dir)
env = SConscript(f"{godot_cpp_dir}/SConstruct")
Default(None)  # Clear godot-cpp default targets

build_library(env)