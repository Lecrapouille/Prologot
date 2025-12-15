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

    real_lib = lib_path.resolve()
    dest = bin_dir / lib_path.name

    shutil.copy2(real_lib, dest)
    copied_files.append(str(dest))
    return True

# ============================================================================
# Copy SWI-Prolog Libraries (Windows)
# ============================================================================
def copy_swipl_libraries_windows(paths, bin_dir, copied_files):
    plbase = paths.get("PLBASE", "")

    search_dirs = [
        Path(plbase) / sub
        for sub in ("bin", "lib", "lib/x64")
        if (Path(plbase) / sub).exists()
    ]

    dll_copied = False
    lib_copied = False

    # DLL
    for dll_name in ("libswipl.dll", "swipl.dll"):
        for d in search_dirs:
            dll = d / dll_name
            if dll.exists():
                dest = bin_dir / dll_name
                shutil.copy2(dll, dest)
                copied_files.append(str(dest))
                dll_copied = True
                break
        if dll_copied:
            break

    # Import library
    for lib_name in ("swipl.lib", "libswipl.lib", "libswipl.dll.a"):
        for d in search_dirs:
            lib = d / lib_name
            if lib.exists():
                dest = bin_dir / "swipl.lib"
                shutil.copy2(lib, dest)
                copied_files.append(str(dest))
                lib_copied = True
                break
        if lib_copied:
            break

# ============================================================================
# Copy SWI-Prolog Libraries (Unix and MacOS)
# ============================================================================
def copy_swipl_libraries_unix(paths, bin_dir, copied_files):
    plbase = paths.get("PLBASE", "")
    pllibswipl = paths.get("PLLIBSWIPL")
    if pllibswipl:
        copy_shared_lib(Path(pllibswipl), bin_dir, copied_files)
        return

    print("Warning: PLLIBSWIPL not found, falling back to PLLIBDIR")
    lib_dir = Path(paths.get("PLLIBDIR", "")) or (Path(plbase) / "lib")
    patterns = ("libswipl.so*", "libswipl.dylib*")
    for pattern in patterns:
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
    for file in copied_files:
        print(f"Copied {file}")
    if not copied_files:
        raise RuntimeError("Error: No SWI-Prolog libraries copied")

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
def create_gdextension_file():
    """Generate prologot.gdextension by scanning the bin/ directory."""
    bin_dir = Path("bin")
    libraries = []

    for lib in bin_dir.glob("libprologot.*"):
        key = lib.stem.replace("libprologot.", "").replace("template_", "")
        libraries.append((key, f"bin/{lib.name}"))

    content = [
        "[configuration]\n",
        'entry_symbol = "prologot_library_init"\n',
        f'compatibility_minimum = "{MIN_GODOT_VERSION}"\n\n',
        "[libraries]\n\n",
    ]

    if libraries:
        content.extend(
            f'{key} = "{path}"\n'
            for key, path in sorted(libraries)
        )
        print(f"prologot.gdextension created with {len(libraries)} library(ies)")
    else:
        print("No libraries found in bin/")

    Path("prologot.gdextension").write_text("".join(content))


# ============================================================================
# Build Functions
# ============================================================================
def get_library_name(env):
    """Return the library name depending on the platform."""
    if env["platform"] == "macos":
        return f"bin/libprologot.{env['platform']}.{env['target']}.{env['arch']}"
    else:
        return f"bin/libprologot{env['suffix']}{env['SHLIBSUFFIX']}"

def build_library(env, godot_cpp_dir):
    """Build the library."""
    env.Append(CPPPATH=["src/"])

    # Configure SWI-Prolog integration
    swipl_paths = get_swipl_paths()
    configure_swipl(env, swipl_paths)

    # Compile source files
    sources = glob("src/*.cpp")
    lib_name = get_library_name(env)
    library = env.SharedLibrary(lib_name, source=sources)

    # Generate .gdextension after library build
    env.AddPostAction(library, lambda target, source, env: create_gdextension_file())

    Default(library)
    return library

# ============================================================================
# Main Entry Point
# ============================================================================
godot_cpp_dir = find_or_setup_godot_cpp()

if GetOption('skip_build'):
    print("Build skipped (--skip-build)")
    Exit(0)

# Load godot-cpp SCons environment
sys.path.insert(0, godot_cpp_dir)
env = SConscript(f"{godot_cpp_dir}/SConstruct")
Default(None)  # Clear godot-cpp default targets

# Build the library
library = build_library(env, godot_cpp_dir)