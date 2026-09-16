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
def overwrite_copied_file(src: Path, dest: Path) -> None:
    """Replace dest even if a previous copy2 left it mode 444 (Homebrew)."""
    if dest.is_symlink() or dest.is_file():
        try:
            dest.chmod(0o666)
        except OSError:
            pass
        dest.unlink()
    elif dest.is_dir():
        def _rm_readonly(_func, path, _exc):
            try:
                Path(path).chmod(0o666)
                _func(path)
            except FileNotFoundError:
                pass
        shutil.rmtree(dest, onerror=_rm_readonly)
    shutil.copy2(src, dest)
    try:
        dest.chmod(0o644)
    except OSError:
        pass


def copy_shared_lib(lib_path: Path, bin_dir: Path, copied_files: list):
    """Copy a shared library, resolving symlinks but keeping the symlink name."""
    if not lib_path.exists():
        return False

    dest = bin_dir / lib_path.name
    overwrite_copied_file(lib_path.resolve(), dest)
    copied_files.append(str(dest))
    return True

# ============================================================================
# Copy SWI-Prolog Libraries (Windows)
# ============================================================================
def copy_swipl_libraries_windows(plbase, bin_dir, copied_files):
    """Copy SWI-Prolog DLL, its sibling runtime DLLs, and the import library.

    Godot loads GDExtensions with LoadLibraryEx(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)
    and does not search PATH, so gmp/zlib/pthread/gcc must sit next to libprologot.
    """
    search_dirs = [plbase / sub for sub in ("bin", "lib", "lib/x64") if (plbase / sub).exists()]

    swipl_dll = None
    for dll_name in ("libswipl.dll", "swipl.dll"):
        for d in search_dirs:
            if (dll := d / dll_name).exists():
                swipl_dll = dll
                break
        if swipl_dll:
            break

    if swipl_dll:
        for dll in swipl_dll.parent.glob("*.dll"):
            dest = bin_dir / dll.name
            overwrite_copied_file(dll, dest)
            copied_files.append(str(dest))

    for lib_name in ("swipl.lib", "libswipl.lib", "libswipl.dll.a"):
        for d in search_dirs:
            if (lib := d / lib_name).exists():
                overwrite_copied_file(lib, bin_dir / "swipl.lib")
                copied_files.append(str(bin_dir / "swipl.lib"))
                return
        else:
            continue
        break

MACOS_SYSTEM_DYLIB_PREFIXES = ("/usr/lib/", "/System/", "/Library/Apple/")


def _otool_load_dylibs(lib_path: Path) -> list:
    """Absolute / @rpath load commands from `otool -L` (skips the header line)."""
    out = subprocess.check_output(["otool", "-L", str(lib_path)], text=True)
    deps = []
    for line in out.splitlines()[1:]:
        line = line.strip()
        if line:
            deps.append(line.split(" ", 1)[0])
    return deps


def _macos_is_system_dylib(path: str) -> bool:
    return path.startswith(MACOS_SYSTEM_DYLIB_PREFIXES)


def _macos_rewrite_install_names(dylib: Path, bin_dir: Path) -> None:
    """Point this dylib at @rpath/<basename> so Godot finds copies next to itself."""
    subprocess.run(["install_name_tool", "-id", f"@rpath/{dylib.name}", str(dylib)], check=False)
    subprocess.run(["install_name_tool", "-add_rpath", "@loader_path", str(dylib)], check=False)
    for dep in _otool_load_dylibs(dylib):
        if _macos_is_system_dylib(dep) or dep.startswith("@rpath/") or dep.startswith("@loader_path/"):
            continue
        basename = Path(dep).name
        if (bin_dir / basename).exists():
            subprocess.run(
                ["install_name_tool", "-change", dep, f"@rpath/{basename}", str(dylib)],
                check=False,
            )
    subprocess.run(["codesign", "--force", "--sign", "-", str(dylib)], check=False)


def _macos_rewrite_built_extension(target, source, env) -> None:
    bin_dir = Path("bin") / "macos"
    for item in target:
        path = Path(str(item))
        if path.suffix == ".dylib" or ".dylib" in path.name:
            _macos_rewrite_install_names(path, bin_dir)


def copy_macos_swipl_runtime(swipl_lib, bin_dir, copied_files):
    """Copy libswipl plus Homebrew deps (gmp, …) and rewrite install names."""
    resolved = swipl_lib.resolve()
    copy_shared_lib(swipl_lib, bin_dir, copied_files)
    if swipl_lib.name != "libswipl.dylib":
        dest = bin_dir / "libswipl.dylib"
        overwrite_copied_file(resolved, dest)
        copied_files.append(str(dest))

    queue = [resolved]
    seen = set()
    extras = {}
    while queue:
        current = queue.pop()
        if not current.exists():
            continue
        key = str(current.resolve())
        if key in seen:
            continue
        seen.add(key)
        for dep in _otool_load_dylibs(current):
            if _macos_is_system_dylib(dep) or dep.startswith("@loader_path/") or dep.startswith("@executable_path/"):
                continue
            dep_path = Path(dep)
            if dep.startswith("@rpath/"):
                candidate = current.parent / Path(dep).name
                if not candidate.exists():
                    continue
                dep_path = candidate
            if not dep_path.exists():
                continue
            dep_path = dep_path.resolve()
            if dep_path == resolved:
                continue
            extras[dep_path.name] = dep_path
            queue.append(dep_path)

    for name, src in extras.items():
        dest = bin_dir / name
        overwrite_copied_file(src, dest)
        copied_files.append(str(dest))
        print(f"Copied {name}")

    for dylib in bin_dir.glob("*.dylib"):
        if dylib.name.startswith("libprologot"):
            continue
        _macos_rewrite_install_names(dylib, bin_dir)


# ============================================================================
# Copy SWI-Prolog Libraries (Unix and MacOS)
# ============================================================================
def copy_swipl_libraries_unix(swipl_lib, bin_dir, copied_files):
    """Copy SWI-Prolog shared library on Unix/macOS."""
    if sys.platform == "darwin":
        copy_macos_swipl_runtime(swipl_lib, bin_dir, copied_files)
        return
    copy_shared_lib(swipl_lib, bin_dir, copied_files)

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
        overwrite_copied_file(boot_files[0], boot_dest)
        copied.append(str(boot_dest))
        print(f"Copied {boot_files[0].name} -> {boot_dest}")
    else:
        print(f"Warning: No boot*.prc found in {plbase}")

    # SWI-Prolog 9+ refuses a home without ABI (and uses swipl.home as marker).
    # make all runs scons twice; Homebrew copies are often 444, so overwrite.
    for name in ("ABI", "swipl.home", "swipl.rc"):
        src = plbase / name
        if src.is_file():
            dest = output_dir / name
            overwrite_copied_file(src, dest)
            copied.append(str(dest))
            print(f"Copied {name} -> {dest}")

    # Copy directories: library/ and lib/
    def _rm_readonly(_func, path, _exc):
        try:
            Path(path).chmod(0o666)
            _func(path)
        except FileNotFoundError:
            pass

    def _rmtree(path):
        try:
            if Path(path).exists():
                shutil.rmtree(path, onerror=_rm_readonly)
        except FileNotFoundError:
            pass

    for dir_name in ("library", "lib"):
        src = plbase / dir_name
        if src.exists() and src.is_dir():
            dest = output_dir / dir_name
            tmp = output_dir / f".{dir_name}.{os.getpid()}"
            _rmtree(tmp)
            shutil.copytree(src, tmp)
            _rmtree(dest)
            try:
                tmp.rename(dest)
            except OSError:
                _rmtree(tmp)
                if not dest.exists():
                    raise
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
    if sys.platform == "darwin":
        env.AddPostAction(library, _macos_rewrite_built_extension)

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

def compiler_has_static_libstdcxx():
    """True if g++ can resolve libstdc++.a (needed for -static-libstdc++)."""
    try:
        out = subprocess.check_output(
            ["g++", "-print-file-name=libstdc++.a"],
            text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return False
    return bool(out) and out != "libstdc++.a" and Path(out).is_file()


# Setup godot-cpp
godot_cpp_dir = find_or_setup_godot_cpp()
sys.path.insert(0, godot_cpp_dir)

# godot-cpp Linux defaults to -static-libstdc++. Fedora ships that archive
# in libstdc++-static, which is not installed by default.
if sys.platform != "win32" and not compiler_has_static_libstdcxx():
    if "use_static_cpp" not in ARGUMENTS:
        ARGUMENTS["use_static_cpp"] = "no"
        print(
            "libstdc++.a not found: linking libstdc++ dynamically "
            "(dnf install libstdc++-static for portable static binaries)."
        )

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