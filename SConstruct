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
# Detect SWI-Prolog paths
# ============================================================================
try:
    result = subprocess.run(['swipl', '--dump-runtime-variables'],
                          capture_output=True, text=True, check=True)
    plbase = None
    for line in result.stdout.split('\n'):
        if 'PLBASE=' in line:
            plbase = line.split('=')[1].strip('";')
            break

    if not plbase:
        print("Error: Could not detect PLBASE from swipl")
        Exit(1)

    import platform as py_platform
    machine = py_platform.machine()
    arch_subdir = f'{machine}-linux' if sys.platform == 'linux' else ''
    swi_lib = os.path.join(plbase, 'lib', arch_subdir)
    swi_include = os.path.join(plbase, 'include')

    print(f"SWI-Prolog include: {swi_include}")
    print(f"SWI-Prolog lib: {swi_lib}")

except (subprocess.CalledProcessError, FileNotFoundError):
    print("Error: swipl not found or failed to run")
    print("Install SWI-Prolog: sudo apt-get install swi-prolog")
    Exit(1)

# ============================================================================
# Load godot-cpp environment
# Use SCons cache if available to avoid recompiling godot-cpp
# ============================================================================
scons_cache_path = os.environ.get("SCONS_CACHE", ".scons_cache")
if scons_cache_path:
    CacheDir(scons_cache_path)
    Decider("MD5")

env = SConscript(f"{godot_cpp_dir}/SConstruct")

# SWI-Prolog configuration
env.Append(CPPPATH=[swi_include, "src/"])
env.Append(LIBPATH=[swi_lib])
env.Append(LIBS=["libswipl" if sys.platform == "win32" else "swipl"])

# Build shared library
sources = glob("src/*.cpp")

if env["platform"] == "macos":
    lib_name = "bin/libprologot.{}.{}.{}".format(env["platform"], env["target"], env["arch"])
else:
    lib_name = "bin/libprologot{}{}".format(env["suffix"], env["SHLIBSUFFIX"])

library = env.SharedLibrary(lib_name, source=sources)
Default(library)
