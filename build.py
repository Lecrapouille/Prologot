#!/usr/bin/env python3
"""
Prologot - SWI-Prolog integration for Godot 4
Installation and build script for Prologot GDExtension.
"""

import os
import sys
import subprocess
import platform
import shutil
import argparse
import re
from pathlib import Path

VERSION = (Path(__file__).parent / "VERSION").read_text().strip() if (Path(__file__).parent / "VERSION").exists() else "0.1.0"
MIN_GODOT_VERSION = "4.2"


# ============================================================================
# Terminal colors
# ============================================================================
class Color:
    """ANSI color codes."""
    HEADER = '\033[95m\033[1m'
    SUCCESS = '\033[92m'
    ERROR = '\033[91m'
    WARNING = '\033[93m'
    INFO = '\033[96m'
    END = '\033[0m'

    @staticmethod
    def header(msg): print(f"\n{Color.HEADER}{'='*60}\n{msg}\n{'='*60}{Color.END}\n")

    @staticmethod
    def success(msg): print(f"{Color.SUCCESS}✔ {msg}{Color.END}")

    @staticmethod
    def error(msg): print(f"{Color.ERROR}✘ {msg}{Color.END}")

    @staticmethod
    def warning(msg): print(f"{Color.WARNING}⚠ {msg}{Color.END}")

    @staticmethod
    def info(msg): print(f"{Color.INFO}ℹ {msg}{Color.END}")


# ============================================================================
# System utilities
# ============================================================================

def run_cmd(cmd, cwd=None, realtime=False):
    """Execute command and return success status."""
    Color.info(f"Running: {' '.join(cmd)}")
    try:
        kwargs = {'cwd': cwd, 'check': True, 'text': True}
        if not realtime:
            kwargs.update({'stdout': subprocess.PIPE, 'stderr': subprocess.PIPE})

        result = subprocess.run(cmd, **kwargs)
        if not realtime and result.stdout:
            print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        Color.error(f"Command failed: {e}")
        if not realtime and hasattr(e, 'stderr') and e.stderr:
            print(e.stderr)
        return False


def get_system():
    """Detect OS and return standardized name."""
    system_map = {'linux': 'linux', 'darwin': 'macos', 'windows': 'windows'}
    system = platform.system().lower()
    return system_map.get(system, 'linux')


def get_cpu_count():
    """Get number of CPU cores, with fallback."""
    return os.cpu_count() or int(os.environ.get('NUMBER_OF_PROCESSORS', '4'))


def parse_version(version_str):
    """Parse version string '4.4.1' to tuple (4, 4, 1)."""
    match = re.match(r'^(\d+)\.(\d+)(?:\.(\d+))?', version_str)
    if match:
        major, minor, patch = match.groups()
        return (int(major), int(minor), int(patch) if patch else 0)
    return (0, 0, 0)


def validate_version(version):
    """Check if Godot version meets minimum requirement."""
    if parse_version(version) < parse_version(MIN_GODOT_VERSION):
        Color.error(f"Godot {version} not supported. Minimum: {MIN_GODOT_VERSION}")
        return False
    Color.success(f"Godot {version} is supported")
    return True


# ============================================================================
# SWI-Prolog installation
# ============================================================================

def install_swi_prolog(force=False):
    """Install SWI-Prolog based on OS."""
    Color.header("Installing SWI-Prolog")

    if not force and shutil.which('swipl'):
        Color.success("SWI-Prolog already installed")
        result = subprocess.run(['swipl', '--version'], capture_output=True, text=True)
        print(result.stdout)
        return True

    system = get_system()

    if system == 'linux':
        # Detect distro
        distro_cmds = {
            '/etc/debian_version': (['sudo', 'apt-get', 'update'],
                                   ['sudo', 'apt-get', 'install', '-y', 'swi-prolog', 'swi-prolog-nox']),
            '/etc/fedora-release': (['sudo', 'dnf', 'install', '-y', 'pl'],),
            '/etc/arch-release': (['sudo', 'pacman', '-S', '--noconfirm', 'swi-prolog'],),
        }

        for marker, cmds in distro_cmds.items():
            if os.path.exists(marker):
                return all(run_cmd(cmd) for cmd in cmds)

        Color.warning("Unknown distro, trying apt")
        return run_cmd(['sudo', 'apt-get', 'update'], check=False) and \
               run_cmd(['sudo', 'apt-get', 'install', '-y', 'swi-prolog'], check=False)

    elif system == 'macos':
        if not shutil.which('brew'):
            Color.error("Homebrew required. Install from https://brew.sh")
            return False
        return run_cmd(['brew', 'install', 'swi-prolog'])

    elif system == 'windows':
        Color.warning("Download and install SWI-Prolog from:")
        Color.info("https://www.swi-prolog.org/download/stable")
        Color.info("Add SWI-Prolog to PATH")
        input("Press Enter after installation...")
        return shutil.which('swipl') is not None

    else:
        Color.error(f"Unsupported system: {system}")
        return False


# ============================================================================
# godot-cpp setup
# ============================================================================

def parse_godot_cpp_ref(git_ref):
    """Parse godot-cpp ref to extract version and git tag/branch.

    Examples: "4.3" -> ("4.3", "4.3")
              "4.3.1" -> ("4.3.1", "godot-4.3-stable")
              "4.3-stable" -> ("4.3", "godot-4.3-stable")
              "godot-4.3-stable" -> ("4.3", "godot-4.3-stable")
    """
    if "-stable" in git_ref:
        version = git_ref.replace("godot-", "").replace("-stable", "")
        tag = git_ref if git_ref.startswith("godot-") else f"godot-{git_ref}"
        return (version, tag)

    parts = git_ref.split('.')
    if len(parts) >= 3:  # "4.3.1" -> use stable tag
        major_minor = '.'.join(parts[:2])
        return (git_ref, f"godot-{major_minor}-stable")

    return (git_ref, git_ref)  # Use as-is for branch


def setup_godot_cpp(godot_version, git_tag, force=False):
    """Clone or update godot-cpp repository."""
    Color.header(f"Setting up godot-cpp for Godot {godot_version}")

    version_suffix = godot_version.replace('.', '_')
    godot_cpp_dir = Path(f'godot-cpp-{version_suffix}')

    if godot_cpp_dir.exists():
        if force:
            Color.warning("Removing existing godot-cpp")
            shutil.rmtree(godot_cpp_dir)
        else:
            Color.info(f"Updating godot-cpp to {git_tag}...")
            run_cmd(['git', 'fetch', 'origin'], cwd=str(godot_cpp_dir))
            run_cmd(['git', 'checkout', git_tag], cwd=str(godot_cpp_dir))
            run_cmd(['git', 'pull', 'origin', git_tag], cwd=str(godot_cpp_dir))
            run_cmd(['git', 'submodule', 'update', '--init', '--recursive'], cwd=str(godot_cpp_dir))
            Color.success("godot-cpp updated")
            return True

    Color.info(f"Cloning godot-cpp ({git_tag})...")
    return run_cmd(['git', 'clone', '--recursive', '-b', git_tag,
                    'https://github.com/godotengine/godot-cpp', str(godot_cpp_dir)])


# ============================================================================
# Build configuration
# ============================================================================

def create_gdextension_file():
    """Generate prologot.gdextension by detecting compiled libraries."""
    Color.header("Generating prologot.gdextension")

    bin_dir = Path('bin')
    bin_dir.mkdir(parents=True, exist_ok=True)

    # Scan for compiled libraries
    libraries = []
    for lib_file in bin_dir.glob('libprologot.*'):
        lib_name = lib_file.name
        rel_path = f"res://bin/{lib_name}"

        # Parse platform/target/arch from filename
        patterns = [
            (r'\.linux\.template_debug\.x86_64\.so$', 'linux.debug.x86_64'),
            (r'\.linux\.template_release\.x86_64\.so$', 'linux.release.x86_64'),
            (r'\.linux\.template_debug\.arm64\.so$', 'linux.debug.arm64'),
            (r'\.linux\.template_release\.arm64\.so$', 'linux.release.arm64'),
            (r'\.windows\.template_debug\.x86_64\.dll$', 'windows.debug.x86_64'),
            (r'\.windows\.template_release\.x86_64\.dll$', 'windows.release.x86_64'),
            (r'\.macos\.template_debug\.framework$', 'macos.debug'),
            (r'\.macos\.template_release\.framework$', 'macos.release'),
        ]

        for pattern, key in patterns:
            if re.search(pattern, lib_name):
                libraries.append((key, rel_path))
                break

    # Build gdextension content
    lines = [
        "[configuration]\n",
        'entry_symbol = "prologot_library_init"\n',
        f'compatibility_minimum = "{MIN_GODOT_VERSION}"\n\n',
        "[libraries]\n\n"
    ]

    if libraries:
        lines.extend(f"{key} = \"{path}\"\n" for key, path in sorted(libraries))
        Color.success(f"Found {len(libraries)} compiled library(ies)")
    else:
        Color.warning("No compiled libraries found, creating template")
        lines.extend([
            'linux.debug.x86_64 = "res://bin/libprologot.linux.template_debug.x86_64.so"\n',
            'linux.release.x86_64 = "res://bin/libprologot.linux.template_release.x86_64.so"\n',
            'windows.debug.x86_64 = "res://bin/libprologot.windows.template_debug.x86_64.dll"\n',
            'windows.release.x86_64 = "res://bin/libprologot.windows.template_release.x86_64.dll"\n',
            'macos.debug = "res://bin/libprologot.macos.template_debug.framework"\n',
            'macos.release = "res://bin/libprologot.macos.template_release.framework"\n',
        ])

    Path('prologot.gdextension').write_text(''.join(lines))
    Color.success("prologot.gdextension created")
    return True


def compile_godot_cpp(godot_version, platform_name, target, jobs):
    """Compile godot-cpp library."""
    version_suffix = godot_version.replace('.', '_')
    godot_cpp_dir = Path(f'godot-cpp-{version_suffix}')

    if not godot_cpp_dir.exists():
        Color.error(f"godot-cpp directory not found: {godot_cpp_dir}")
        return False

    Color.header(f"Compiling godot-cpp for Godot {godot_version}")
    return run_cmd(['scons', f'platform={platform_name}', f'target={target}', f'-j{jobs}'],
                   cwd=str(godot_cpp_dir), realtime=True)


def compile_extension(platform_name, target, jobs):
    """Compile the GDExtension."""
    Color.header(f"Compiling extension ({platform_name}, {target})")
    return run_cmd(['scons', f'platform={platform_name}', f'target={target}', f'-j{jobs}'],
                   realtime=True)


# ============================================================================
# Main
# ============================================================================
def main():
    parser = argparse.ArgumentParser(description='Prologot GDExtension installer')
    parser.add_argument('--skip-swi', action='store_true', help='Skip SWI-Prolog installation')
    parser.add_argument('--skip-godot-cpp', action='store_true', help='Skip godot-cpp setup')
    parser.add_argument('--skip-compile', action='store_true', help='Skip compilation')
    parser.add_argument('--compile-godot-cpp-only', action='store_true', help='Only compile godot-cpp (skip extension compilation)')
    parser.add_argument('--force', action='store_true', help='Force reinstallation')
    parser.add_argument('--target', default='template_debug',
                       choices=['template_debug', 'template_release'], help='Build target')
    parser.add_argument('--platform', help='Target platform (auto-detected if not set)')
    parser.add_argument('--godot-cpp', required=True, help='Godot godot-cpp git tag or branch (e.g., "4.3", "4.3-stable", "godot-4.3-stable") [REQUIRED]')
    parser.add_argument('--jobs', type=int, help='Parallel compilation jobs (auto-detected if not set)')

    args = parser.parse_args()

    print(f"\n{Color.HEADER}╔{'='*56}╗")
    title = f"Prologot GDExtension Installer v{VERSION}"
    print(f"║ {title:^54} ║")
    print(f"╚{'='*56}╝{Color.END}\n")

    # Platform detection
    platform_name = args.platform or get_system()
    jobs = args.jobs or get_cpu_count()
    godot_version, git_tag = parse_godot_cpp_ref(args.godot_cpp)

    Color.info(f"Platform: {platform_name}, Jobs: {jobs}, Godot: {godot_version}")

    if not validate_version(godot_version):
        return 1

    # Install SWI-Prolog
    if not args.skip_swi:
        if not install_swi_prolog(args.force):
            return 1
        if not shutil.which('swipl'):
            Color.error("swipl not found in PATH")
            return 1

    # Setup godot-cpp
    if not args.skip_godot_cpp:
        if not setup_godot_cpp(godot_version, git_tag, args.force):
            return 1

    # Compile godot-cpp if missing or if specifically requested
    # We check for the library existence to avoid unnecessary scons calls
    if args.compile_godot_cpp_only or not args.skip_compile:
        version_suffix = godot_version.replace('.', '_')
        godot_cpp_dir = Path(f'godot-cpp-{version_suffix}')
        # Quick check if godot-cpp seems already compiled (at least one .a or .lib)
        lib_exists = any((godot_cpp_dir / "bin").glob("*.a")) or any((godot_cpp_dir / "bin").glob("*.lib"))

        if args.compile_godot_cpp_only or not lib_exists:
            if not compile_godot_cpp(godot_version, platform_name, args.target, jobs):
                return 1
            if args.compile_godot_cpp_only:
                return 0

    # Generate .gdextension file
    if not create_gdextension_file():
        return 1

    # Compile extension
    if not args.skip_compile:
        if not compile_extension(platform_name, args.target, jobs):
            return 1

    Color.success("Build completed successfully!\n")
    return 0


if __name__ == '__main__':
    sys.exit(main())