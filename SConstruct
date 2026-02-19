#!/usr/bin/env python
import sys
import os
import subprocess
import shutil
from glob import glob
from pathlib import Path

MIN_GODOT_VERSION = "4.2"
PLATFORM_NAMES = {'win32': 'windows', 'darwin': 'macos', 'linux': 'linux'}

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
# Setup godot-cpp folder
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

    # Find godot-cpp-* directories
    godot_cpp_dirs = list(Path('.').glob('godot-cpp-*'))
    if not godot_cpp_dirs:
        print("Error: No godot-cpp-* directory found!")
        print("Run: scons --godot-cpp=4.5")
        Exit(1)

    # Several godot-cpp-* directories found, error
    if len(godot_cpp_dirs) > 1:
        print("Error: Multiple godot-cpp-* directories found, please specify one with --godot-cpp=VERSION")
        print("Found:", [str(d) for d in godot_cpp_dirs])
        print("Example: scons --godot-cpp=4.5")
        Exit(1)

    # Return the first godot-cpp-* directory
    return str(godot_cpp_dirs[0])

# ============================================================================
# Find SWI-Prolog
# ============================================================================
SWIPL_CANDIDATES = {
    'win32': [
        r'C:\Program Files\swipl\bin\swipl.exe',
        r'C:\Program Files (x86)\swipl\bin\swipl.exe',
        'swipl',
    ],
}
SWIPL_INSTALL_HINT = {
    'win32': 'choco install swi-prolog',
    'darwin': 'brew install swi-prolog',
}

def find_swipl():
    """Find SWI-Prolog via --dump-runtime-variables. Exit with install hint if not found."""
    for swipl_cmd in SWIPL_CANDIDATES.get(sys.platform, ['swipl']):
        try:
            result = subprocess.run([swipl_cmd, '--dump-runtime-variables'],
                                    capture_output=True, text=True, check=True)
            swipl = {}
            for line in result.stdout.strip().split('\n'):
                if '=' in line:
                    key, val = line.split('=', 1)
                    swipl[key] = val.strip('";')
            return swipl
        except (subprocess.CalledProcessError, FileNotFoundError):
            continue

    print("Error: SWI-Prolog not found!")
    hint = SWIPL_INSTALL_HINT.get(sys.platform, 'sudo apt-get install swi-prolog')
    print(f"Install with: {hint}")
    Exit(1)

def find_plbase(swipl):
    """Extract PLBASE as Path from swipl runtime variables."""
    plbase = Path(swipl.get("PLBASE", ""))
    if not plbase.name or not plbase.exists():
        print(f"Error: PLBASE not found or invalid: {plbase}")
        Exit(1)
    return plbase

def find_swipl_lib(swipl, plbase):
    """Find the SWI-Prolog shared library from PLLIBSWIPL, fallback to PLLIBDIR/PLBASE."""
    lib_path = Path(swipl.get("PLLIBSWIPL", ""))
    if lib_path.exists():
        return lib_path

    lib_dir = Path(swipl.get("PLLIBDIR") or str(plbase)) / "lib"
    for pattern in ("libswipl.so*", "libswipl.dylib*"):
        for lib in lib_dir.glob(pattern):
            if lib.is_file():
                return lib
    print("Error: SWI-Prolog shared library not found")
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
def copy_swipl_libraries_windows(plbase, bin_dir, copied_files):
    """Copy SWI-Prolog DLL and import library on Windows."""
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
def copy_swipl_libraries_unix(swipl_lib, bin_dir, copied_files):
    """Copy SWI-Prolog shared library on Unix/macOS."""
    copy_shared_lib(swipl_lib, bin_dir, copied_files)

    # On macOS, also copy as libswipl.dylib for linking
    if sys.platform == 'darwin' and swipl_lib.name != 'libswipl.dylib':
        base_dest = bin_dir / 'libswipl.dylib'
        if base_dest.exists():
            base_dest.chmod(0o666)
            base_dest.unlink()
        shutil.copy2(swipl_lib.resolve(), base_dest)
        copied_files.append(str(base_dest))

# ============================================================================
# Copy SWI-Prolog Libraries (Windows, MacOS and Unix)
# ============================================================================
def copy_swipl_libraries(swipl, plbase):
    """Copy the necessary SWI-Prolog libraries into the bin/<platform>/ directory."""
    platform_name = PLATFORM_NAMES.get(sys.platform, sys.platform)
    bin_dir = Path("bin") / platform_name
    bin_dir.mkdir(parents=True, exist_ok=True)

    copied_files = []
    if sys.platform == "win32":
        copy_swipl_libraries_windows(plbase, bin_dir, copied_files)
    else:
        swipl_lib = find_swipl_lib(swipl, plbase)
        copy_swipl_libraries_unix(swipl_lib, bin_dir, copied_files)

    if not copied_files:
        print("Error: No SWI-Prolog libraries copied")
        Exit(1)
    for f in copied_files:
        print(f"Copied {f}")
    return copied_files

# ============================================================================
# Copy SWI-Prolog Resources (boot.prc, library/, etc.)
# ============================================================================
def copy_swipl_resources(plbase, output_dir=None):
    """Copy SWI-Prolog runtime resources (boot.prc, library/, lib/)."""
    if not output_dir:
        platform_name = PLATFORM_NAMES.get(sys.platform, sys.platform)
        output_dir = Path("bin") / platform_name / "swipl"
    output_dir = Path(output_dir)

    if not plbase.exists():
        print(f"Warning: PLBASE directory not found: {plbase}")
        return []

    output_dir.mkdir(parents=True, exist_ok=True)
    copied = []

    # Copy boot*.prc file
    boot_files = list(plbase.glob("boot*.prc"))
    if boot_files:
        boot_dest = output_dir / "boot.prc"
        if boot_dest.exists():
            boot_dest.chmod(0o666)
            boot_dest.unlink()
        shutil.copy2(boot_files[0], boot_dest)
        copied.append(str(boot_dest))
        print(f"Copied {boot_files[0].name} -> {boot_dest}")
    else:
        print(f"Warning: No boot*.prc found in {plbase}")

    # Copy directories: library/ and lib/
    def _rm_readonly(_func, path, _exc):
        Path(path).chmod(0o666)
        _func(path)

    for dir_name in ("library", "lib"):
        src = plbase / dir_name
        if src.exists() and src.is_dir():
            dest = output_dir / dir_name
            if dest.exists():
                shutil.rmtree(dest, onerror=_rm_readonly)
            shutil.copytree(src, dest)
            copied.append(str(dest))
            print(f"Copied {dir_name}/ -> {dest}")
        else:
            print(f"Warning: {dir_name}/ not found in {plbase}")

    return copied

# ============================================================================
# Configure SCons Environment for SWI-Prolog
# ============================================================================
def configure_swipl(env, plbase):
    """Configure the SCons environment for SWI-Prolog."""
    include_dir = plbase / 'include'
    if include_dir.exists():
        env.Append(CPPPATH=[str(include_dir)])

    platform_name = PLATFORM_NAMES.get(sys.platform, sys.platform)
    env.Append(LIBPATH=[os.path.abspath(f'bin/{platform_name}')])
    env.Append(LIBS=['swipl'])
    if sys.platform == 'darwin':
        env.Append(LINKFLAGS=['-Wl,-rpath,@loader_path'])
    elif sys.platform != 'win32':
        env.Append(RPATH=['$$ORIGIN'])

# ============================================================================
# Create prologot.gdextension
# ============================================================================
def create_gdextension_file(base_path=".", verbose=False):
    """Generate prologot.gdextension by scanning bin/<platform>/ subdirectories."""
    bin_dir = Path(base_path) / "bin"
    if not bin_dir.exists():
        return

    known_platforms = set(PLATFORM_NAMES.values())
    libraries = []
    dependencies = []

    for platform_dir in sorted(bin_dir.iterdir()):
        if not platform_dir.is_dir() or platform_dir.name not in known_platforms:
            continue
        platform = platform_dir.name
        for lib in platform_dir.iterdir():
            if not lib.is_file() or lib.suffix in ('.lib', '.a', '.exp'):
                continue
            if lib.name.startswith("libprologot"):
                key = lib.stem.replace("libprologot.", "").replace("template_", "")
                libraries.append((key, f"{platform}/{lib.name}"))
            elif "swipl" in lib.name:
                dependencies.append((platform, lib.name))

    content = [
        "[configuration]",
        'entry_symbol = "prologot_library_init"',
        f'compatibility_minimum = "{MIN_GODOT_VERSION}"',
        "reloadable = true",
        "",
        "[libraries]",
        ""
    ]

    content.extend(f'{key} = "{path}"' for key, path in sorted(libraries))

    if dependencies:
        content.extend(["", "[dependencies]", ""])
        for key, lib_path in sorted(libraries):
            platform = lib_path.split('/')[0]
            dep = next((f"{p}/{n}" for p, n in dependencies if p == platform), None)
            if dep:
                content.append(f'{key} = {{"{dep}": ""}}')

    output_path = bin_dir / "prologot.gdextension"
    output_path.write_text('\n'.join(content) + '\n')

    if verbose:
        print(f"prologot.gdextension: {len(libraries)} lib(s), {len(dependencies)} dep(s)")
        print('\n'.join(content))


# ============================================================================
# Build Functions
# ============================================================================
def build_library(env):
    """Build the Prologot library."""
    platform_name = PLATFORM_NAMES.get(sys.platform, sys.platform)
    platform_dir = f"bin/{platform_name}"

    if env["platform"] == "macos":
        lib_name = f"{platform_dir}/libprologot.{env['platform']}.{env['target']}.{env['arch']}"
    else:
        lib_name = f"{platform_dir}/libprologot{env['suffix']}{env['SHLIBSUFFIX']}"

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

# Setup godot-cpp
godot_cpp_dir = find_or_setup_godot_cpp()
sys.path.insert(0, godot_cpp_dir)
env = SConscript(f"{godot_cpp_dir}/SConstruct")
Default(None)  # Clear godot-cpp default targets

# Setup SWI-Prolog
swipl = find_swipl()
plbase = find_plbase(swipl)

# Configure and build
env.Append(CPPPATH=["src/"])
configure_swipl(env, plbase)
build_library(env)

# Copy SWI-Prolog libraries and resources
copy_swipl_libraries(swipl, plbase)
copy_swipl_resources(plbase)