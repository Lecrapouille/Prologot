#!/usr/bin/env python
import sys
import os
import subprocess
from glob import glob
from pathlib import Path

# ============================================================================
# Find versioned godot-cpp directory
# ============================================================================
godot_cpp_dirs = list(Path('.').glob('godot-cpp-*'))
if not godot_cpp_dirs:
    print("Error: No godot-cpp-* directory found!")
    print("Run: make godot-cpp  or  python3 build.py --godot-cpp 4.3")
    Exit(1)

godot_cpp_dir = str(godot_cpp_dirs[0])
print(f"Using {godot_cpp_dir}")

# ============================================================================
# Get pkg-config output for a package
# ============================================================================
def get_pkg_config(package, *args):
    """Get pkg-config output for a package."""
    try:
        cmd = ['pkg-config'] + list(args) + [package]
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        return result.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None

# ============================================================================
# Get SWI-Prolog paths from swipl command
# ============================================================================
def get_swipl_paths():
    """Get SWI-Prolog paths using swipl --dump-runtime-variables."""
    # On Windows, try common installation paths if swipl is not in PATH
    if sys.platform == 'win32':
        swipl_paths = [
            r'C:\Program Files\swipl\bin\swipl.exe',
            r'C:\Program Files (x86)\swipl\bin\swipl.exe',
            'swipl'  # Try PATH as fallback
        ]
    else:
        swipl_paths = ['swipl']

    for swipl_cmd in swipl_paths:
        try:
            result = subprocess.run([swipl_cmd, '--dump-runtime-variables'],
                                    capture_output=True, text=True, check=True)
            # Parse output like: PLBASE="/usr/lib/swipl"; PLLIBDIR="/usr/lib/..."; etc.
            paths = {}
            for line in result.stdout.strip().split('\n'):
                if '=' in line:
                    key, val = line.split('=', 1)
                    paths[key] = val.strip('";')
            return paths
        except (subprocess.CalledProcessError, FileNotFoundError):
            continue

    return None

# ============================================================================
# Detect SWI-Prolog installation
# ============================================================================
def detect_swi_prolog():
    """Detect SWI-Prolog using pkg-config or swipl directly.

    Returns:
        tuple: (use_pkg_config: bool, swi_paths: dict or None)
    """
    is_windows = sys.platform == 'win32'

    # On Windows, skip pkg-config and use swipl directly
    if is_windows:
        swi_paths = get_swipl_paths()
        if swi_paths:
            print(f"SWI-Prolog (Windows): {swi_paths.get('PLBASE', 'found')}")
            return False, swi_paths
        else:
            print("Error: SWI-Prolog not found on Windows")
            print("Install: choco install swi-prolog")
            Exit(1)

    # Try pkg-config first on Linux/macOS
    swi_cflags = get_pkg_config('swipl', '--cflags')
    swi_libs = get_pkg_config('swipl', '--libs')

    if swi_cflags and swi_libs:
        print(f"SWI-Prolog (pkg-config): cflags={swi_cflags} libs={swi_libs}")
        return True, None

    # Fallback: use swipl --dump-runtime-variables
    swi_paths = get_swipl_paths()
    if swi_paths:
        print(f"SWI-Prolog (swipl): {swi_paths.get('PLBASE', 'found')}")
        return False, swi_paths

    print("Error: SWI-Prolog not found")
    print("Install: sudo apt-get install swi-prolog swi-prolog-nox")
    print("Or macOS: brew install swi-prolog")
    Exit(1)

# ============================================================================
# Configure SCons environment with SWI-Prolog
# ============================================================================
def configure_swi_prolog(env, use_pkg_config, swi_paths):
    """Configure environment with SWI-Prolog paths.

    Args:
        env: SCons environment
        use_pkg_config: If True, use pkg-config; else use swi_paths
        swi_paths: Dictionary from swipl --dump-runtime-variables
    """
    if use_pkg_config:
        env.ParseConfig('pkg-config --cflags --libs swipl')
        return

    # Manual configuration from swipl paths
    plbase = swi_paths.get('PLBASE', '')
    pllibdir = swi_paths.get('PLLIBDIR', '')

    # Include path
    include_dir = os.path.join(plbase, 'include')
    if os.path.exists(include_dir):
        env.Append(CPPPATH=[include_dir])

    # Library path and name
    if pllibdir and os.path.exists(pllibdir):
        env.Append(LIBPATH=[pllibdir])
    env.Append(LIBS=['swipl'])

# ============================================================================
# Add macOS rpath for runtime library lookup
# ============================================================================
def add_macos_rpath(env, use_pkg_config, swi_paths):
    """Add rpath for macOS runtime library lookup."""
    if sys.platform != "darwin":
        return

    libdir = None
    if use_pkg_config:
        libdir = get_pkg_config('swipl', '--variable=libdir')
    elif swi_paths:
        libdir = swi_paths.get('PLLIBDIR')

    if libdir:
        env.Append(LINKFLAGS=[f'-Wl,-rpath,{libdir}', '-Wl,-rpath,@loader_path'])

# ============================================================================
# Get library name based on platform
# ============================================================================
def get_library_name(env):
    """Get library name based on platform and target."""
    if env["platform"] == "macos":
        return "bin/libprologot.{}.{}.{}".format(env["platform"], env["target"], env["arch"])
    else:
        return "bin/libprologot{}{}".format(env["suffix"], env["SHLIBSUFFIX"])

# ============================================================================
# Main build configuration
# ============================================================================

# Load godot-cpp environment
scons_cache_path = os.environ.get("SCONS_CACHE", ".scons_cache")
if scons_cache_path:
    CacheDir(scons_cache_path)
    Decider("MD5")

# Load godot-cpp environment without rebuilding it
# The SConstruct of godot-cpp sets its library as default target,
# so we clear defaults right after to avoid rebuilding godot-cpp
sys.path.insert(0, godot_cpp_dir)
env = SConscript(f"{godot_cpp_dir}/SConstruct")

# Clear godot-cpp's default targets to prevent rebuild
Default(None)

env.Append(CPPPATH=["src/"])

# Detect and configure SWI-Prolog
use_pkg_config, swi_paths = detect_swi_prolog()
configure_swi_prolog(env, use_pkg_config, swi_paths)
add_macos_rpath(env, use_pkg_config, swi_paths)

# ============================================================================
# Build shared library
# ============================================================================
sources = glob("src/*.cpp")
lib_name = get_library_name(env)
library = env.SharedLibrary(lib_name, source=sources)
Default(library)
