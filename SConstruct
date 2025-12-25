#!/usr/bin/env python
import sys
import os
import subprocess
from glob import glob
from pathlib import Path

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
# SWI-Prolog configuration class
# ============================================================================
class SwiPrologConfig:
    """Base class for SWI-Prolog configuration."""

    def __init__(self, use_pkg_config=False, paths=None):
        """Initialize configuration.

        Args:
            use_pkg_config: If True, use pkg-config for configuration
            paths: Dictionary of SWI-Prolog paths (PLBASE, PLLIBDIR, etc.)
        """
        self.use_pkg_config = use_pkg_config
        self.paths = paths or {}

    @property
    def plbase(self):
        """Returns SWI-Prolog base directory."""
        return self.paths.get('PLBASE', '')

    @property
    def pllibdir(self):
        """Returns SWI-Prolog library directory."""
        return self.paths.get('PLLIBDIR', '')

    def get_libdir(self):
        """Returns library directory via pkg-config or paths."""
        if self.use_pkg_config:
            return get_pkg_config('swipl', '--variable=libdir')
        return self.pllibdir

    @classmethod
    def _get_swipl_candidates(cls):
        """Get list of swipl command candidates to try for this platform.

        Returns:
            list: List of swipl command paths to try
        """
        return ['swipl']

    @classmethod
    def _get_swipl_paths(cls):
        """Get SWI-Prolog paths using swipl --dump-runtime-variables."""
        for swipl_cmd in cls._get_swipl_candidates():
            try:
                result = subprocess.run([swipl_cmd, '--dump-runtime-variables'],
                                        capture_output=True, text=True, check=True)
                # Parse output like: PLBASE="/usr/lib/swipl"; PLLIBDIR="/usr/lib/..."; etc.
                # On Windows: PLBASE="C:\\Program Files\\swipl"; PLLIBDIR="C:\\Program Files\\swipl\\lib"; etc.
                paths = {}
                for line in result.stdout.strip().split('\n'):
                    if '=' in line:
                        key, val = line.split('=', 1)
                        # Remove quotes, semicolons, and normalize path
                        val = val.strip('";')
                        # Normalize path for Windows (handles backslashes)
                        if val:
                            val = os.path.normpath(val)
                        paths[key] = val
                return paths
            except (subprocess.CalledProcessError, FileNotFoundError):
                continue

        return None

    def configure_libraries(self, env):
        """Configure library paths and linking for this platform.

        Args:
            env: SCons environment to configure
        """
        raise NotImplementedError("Subclasses must implement configure_libraries()")

    def configure_rpath(self, env):
        """Configure rpath for runtime library lookup (macOS only by default).

        Args:
            env: SCons environment to configure
        """
        # Default implementation does nothing (override in DarwinSwiPrologConfig)
        pass

    def configure(self, env):
        """Configure SCons environment with SWI-Prolog.

        Configures paths, libraries, and platform-specific settings.

        Args:
            env: SCons environment to configure
        """
        # Configure via pkg-config (simpler)
        if self.use_pkg_config:
            env.ParseConfig('pkg-config --cflags --libs swipl')
            self.configure_rpath(env)
            return

        # Manual configuration from swipl paths
        include_dir = os.path.join(self.plbase, 'include')
        if os.path.exists(include_dir):
            env.Append(CPPPATH=[include_dir])

        # Configure libraries and rpath (platform-specific)
        self.configure_libraries(env)
        self.configure_rpath(env)


class WindowsSwiPrologConfig(SwiPrologConfig):
    """SWI-Prolog configuration for Windows platform."""

    @classmethod
    def _get_swipl_candidates(cls):
        """Get list of swipl command candidates to try on Windows."""
        return [
            r'C:\Program Files\swipl\bin\swipl.exe',
            r'C:\Program Files (x86)\swipl\bin\swipl.exe',
            'swipl'  # Try PATH as fallback
        ]

    def configure_libraries(self, env):
        """Configure Windows library paths and linking."""
        lib_name = 'swipl.lib'
        lib_dirs = []
        lib_candidates = []

        # Normalize paths for Windows
        def normpath(p):
            return os.path.normpath(p) if p else None

        plbase = normpath(self.plbase) if self.plbase else None
        pllibdir = normpath(self.pllibdir) if self.pllibdir else None

        # Collect potential library directories
        if pllibdir:
            lib_dirs.append(pllibdir)
            lib_candidates.append(os.path.join(pllibdir, lib_name))

        if plbase:
            # Try lib subdirectory
            lib_dir = os.path.join(plbase, 'lib')
            lib_dir = normpath(lib_dir)
            lib_candidates.append(os.path.join(lib_dir, lib_name))
            lib_dirs.append(lib_dir)

            # Try lib/x86_64 or lib/x64 for 64-bit
            for arch in ['x86_64', 'x64']:
                arch_lib_dir = os.path.join(plbase, 'lib', arch)
                arch_lib_dir = normpath(arch_lib_dir)
                lib_candidates.append(os.path.join(arch_lib_dir, lib_name))
                lib_dirs.append(arch_lib_dir)

            # Also try bin directory (sometimes .lib files are there on Windows)
            bin_dir = os.path.join(plbase, 'bin')
            bin_dir = normpath(bin_dir)
            lib_candidates.append(os.path.join(bin_dir, lib_name))
            lib_dirs.append(bin_dir)

            # Try lib/msvc or lib/msvc64 (common on Windows)
            for msvc_dir in ['msvc', 'msvc64']:
                msvc_lib_dir = os.path.join(plbase, 'lib', msvc_dir)
                msvc_lib_dir = normpath(msvc_lib_dir)
                lib_candidates.append(os.path.join(msvc_lib_dir, lib_name))
                lib_dirs.append(msvc_lib_dir)

        # Also check common installation paths (Chocolatey, etc.)
        common_paths = [
            r'C:\Program Files\swipl',
            r'C:\Program Files (x86)\swipl',
            os.path.expanduser(r'~\swipl'),
        ]
        for common_path in common_paths:
            if os.path.exists(common_path):
                common_path = normpath(common_path)
                for subdir in ['lib', 'lib\\x64', 'lib\\x86_64', 'bin']:
                    test_dir = os.path.join(common_path, subdir)
                    test_dir = normpath(test_dir)
                    if os.path.isdir(test_dir):
                        lib_candidates.append(os.path.join(test_dir, lib_name))
                        lib_dirs.append(test_dir)

        # Find the actual library file and filter existing directories
        lib_path = None
        for candidate in lib_candidates:
            candidate = normpath(candidate)
            if os.path.exists(candidate) and os.path.isfile(candidate):
                lib_path = candidate
                break

        existing_lib_dirs = [normpath(d) for d in lib_dirs if os.path.exists(d) and os.path.isdir(d)]

        if lib_path:
            # Use full path to library file
            lib_path = normpath(lib_path)
            lib_dir = os.path.dirname(lib_path)
            # Add directory to LIBPATH
            env.Append(LIBPATH=[lib_dir])
            # Use library name (linker will find it via LIBPATH)
            env.Append(LIBS=['swipl'])
            print(f"SWI-Prolog library found: {lib_path}")
            print(f"  Using LIBPATH: {lib_dir}")
        elif existing_lib_dirs:
            # Add all existing library directories to LIBPATH
            for lib_dir in existing_lib_dirs:
                env.Append(LIBPATH=[lib_dir])
            env.Append(LIBS=['swipl'])
            print(f"Warning: {lib_name} not found explicitly, using LIBPATH={existing_lib_dirs}")
            print("  The linker will search for swipl.lib in these directories.")
        else:
            # Error: library not found
            print(f"Error: {lib_name} not found!")
            print(f"  PLBASE: {plbase}")
            print(f"  PLLIBDIR: {pllibdir}")
            print(f"  Searched in: {lib_candidates}")
            print("  Please ensure SWI-Prolog is installed correctly.")
            print("  On Windows, try: choco install swi-prolog")
            Exit(1)


class DarwinSwiPrologConfig(SwiPrologConfig):
    """SWI-Prolog configuration for macOS/Darwin platform."""

    def configure_libraries(self, env):
        """Configure Unix (macOS) library paths and linking."""
        if self.pllibdir and os.path.exists(self.pllibdir):
            env.Append(LIBPATH=[self.pllibdir])
        env.Append(LIBS=['swipl'])

    def configure_rpath(self, env):
        """Configure macOS rpath for runtime library lookup."""
        libdir = self.get_libdir()
        if libdir:
            env.Append(LINKFLAGS=[f'-Wl,-rpath,{libdir}', '-Wl,-rpath,@loader_path'])


class LinuxSwiPrologConfig(SwiPrologConfig):
    """SWI-Prolog configuration for Linux platform."""

    def configure_libraries(self, env):
        """Configure Unix (Linux) library paths and linking."""
        if self.pllibdir and os.path.exists(self.pllibdir):
            env.Append(LIBPATH=[self.pllibdir])
        env.Append(LIBS=['swipl'])


# Add factory method to SwiPrologConfig after all subclasses are defined
def _find_swi_prolog_config():
    """Find and return available SWI-Prolog configuration.

    Factory method that returns the appropriate platform-specific class.
    Tries pkg-config first (Linux/macOS), then swipl --dump-runtime-variables.

    Returns:
        SwiPrologConfig: Found configuration (platform-specific subclass)

    Exits:
        If SWI-Prolog is not found, prints error message and exits.
    """
    # Determine platform-specific class
    if sys.platform == 'win32':
        config_class = WindowsSwiPrologConfig
    elif sys.platform == 'darwin':
        config_class = DarwinSwiPrologConfig
    else:
        config_class = LinuxSwiPrologConfig

    # Try pkg-config first on Linux/macOS (skip on Windows)
    if sys.platform != 'win32':
        swi_cflags = get_pkg_config('swipl', '--cflags')
        swi_libs = get_pkg_config('swipl', '--libs')

        if swi_cflags and swi_libs:
            print(f"SWI-Prolog (pkg-config): cflags={swi_cflags} libs={swi_libs}")
            return config_class(use_pkg_config=True)

    # Fallback: use swipl --dump-runtime-variables (all platforms)
    swi_paths = config_class._get_swipl_paths()
    if swi_paths:
        platform_name = "Windows" if sys.platform == 'win32' else "swipl"
        print(f"SWI-Prolog ({platform_name}): {swi_paths.get('PLBASE', 'found')}")
        return config_class(use_pkg_config=False, paths=swi_paths)

    # Error handling for all platforms
    print("Error: SWI-Prolog not found")
    if sys.platform == 'win32':
        print("Install: choco install swi-prolog")
    elif sys.platform == 'darwin':
        print("Install: brew install swi-prolog")
    else:
        print("Install: sudo apt-get install swi-prolog swi-prolog-nox")
    Exit(1)

# Attach the factory method as a static method to SwiPrologConfig
SwiPrologConfig.find = staticmethod(_find_swi_prolog_config)

# ============================================================================
# Find versioned godot-cpp directory
# ============================================================================

# Get godot_cpp_dir from SCons arguments if provided, otherwise use fallback
godot_cpp_dir = ARGUMENTS.get('godot_cpp_dir')

if not godot_cpp_dir:
    # Fallback: find first matching directory
    godot_cpp_dirs = list(Path('.').glob('godot-cpp-*'))
    if not godot_cpp_dirs:
        print("Error: No godot-cpp-* directory found!")
        print("Run: make godot-cpp  or  python3 build.py --godot-cpp 4.3")
        Exit(1)
    godot_cpp_dir = str(godot_cpp_dirs[0])

# Validate that the directory exists
if not os.path.exists(godot_cpp_dir):
    print(f"Error: godot-cpp directory not found: {godot_cpp_dir}")
    Exit(1)

print(f"Using {godot_cpp_dir}")

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

# ============================================================================
# Build shared library
# ============================================================================

env.Append(CPPPATH=["src/"])
swi_config = SwiPrologConfig.find()
swi_config.configure(env)
sources = glob("src/*.cpp")
lib_name = get_library_name(env)
library = env.SharedLibrary(lib_name, source=sources)
Default(library)
