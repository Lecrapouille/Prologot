# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# Makefile for building and managing the Prologot GDExtension

# Terminal colors
ECHO := echo -e
CYAN := \033[0;36m
GREEN := \033[0;32m
YELLOW := \033[0;33m
RED := \033[0;31m
NC := \033[0m

# Read Prologot GDExtension version from VERSION file
VERSION := $(shell cat VERSION 2>/dev/null || echo "0.1.1")

# Godot godot-cpp tag or branch (e.g., "4.6", "godot-4.3-stable")
# SConstruct will parse this to extract version and determine if it's a tag or branch
# Note: Use 4.5 for Godot 4.6 until godot-cpp 4.6 is released
GODOT_CPP ?= 4.5

# Build output directory for the GDExtension library
BIN ?= bin

# Detect architecture
UNAME_M := $(shell uname -m)

# Default target
.PHONY: help
help:
	@$(ECHO) "$(CYAN)╔════════════════════════════════════════════════════════╗$(NC)"
	@$(ECHO) "$(CYAN)║          Prologot GDExtension v$(VERSION)                   ║$(NC)"
	@$(ECHO) "$(CYAN)║               Using godot-cpp: $(GODOT_CPP)                     ║$(NC)"
	@$(ECHO) "$(CYAN)╚════════════════════════════════════════════════════════╝$(NC)"
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)Prerequisites commands:$(NC)"
	@$(ECHO) "  $(YELLOW)make install-swi$(NC)   - Install SWI-Prolog on the operating system"
	@$(ECHO) "  $(YELLOW)make check-deps$(NC)    - Check dependencies for building"
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)Main commands:$(NC)"
	@$(ECHO) "  $(YELLOW)make all$(NC)           - Build debug + release"
	@$(ECHO) "  $(YELLOW)make debug$(NC)         - Compile the Prologot in debug mode"
	@$(ECHO) "  $(YELLOW)make release$(NC)       - Compile the Prologot in release mode"
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)Advanced commands:$(NC)"
	@$(ECHO) "  $(YELLOW)make tests$(NC)         - Run unit tests"
	@$(ECHO) "  $(YELLOW)make clean$(NC)         - Clean compiled files"
	@$(ECHO) "  $(YELLOW)make format$(NC)        - Format C++ sources"
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)Demo commands:$(NC)"
	@$(ECHO) "  $(YELLOW)make setup-demos$(NC)   - Set up demo and test projects"
	@$(ECHO) "  $(YELLOW)make run-demo$(NC)      - Run demo in Godot"
	@$(ECHO) "  $(YELLOW)make run-galactic$(NC)  - Run galactic_customs game in Godot"
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)Configuration:$(NC)"
	@$(ECHO) "  $(YELLOW)GODOT_CPP$(NC)          - Godot godot-cpp ref (default: $(GODOT_CPP))"
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)Examples:"
	@$(ECHO) "  $(YELLOW)make GODOT_CPP=4.5 all"
	@$(ECHO) "  $(YELLOW)make GODOT_CPP=4.3.1 debug"
	@$(ECHO) "  $(YELLOW)make run-galactic"
	@$(ECHO) "$(NC)"

# Install SWI-Prolog only
.PHONY: install-swi
install-swi:
	@$(ECHO) "$(CYAN)▶ Installing SWI-Prolog...$(NC)"
	@if command -v swipl >/dev/null 2>&1 && [ -z "$(FORCE)" ]; then \
		$(ECHO) "$(GREEN)✓ SWI-Prolog already installed$(NC)"; \
		swipl --version; \
		exit 0; \
	fi
	@UNAME_S=$$(uname -s); \
	if [ "$$UNAME_S" = "Linux" ]; then \
		if [ -f /etc/debian_version ]; then \
			$(ECHO) "$(YELLOW)Installing via apt-get...$(NC)"; \
			sudo apt-get install -y swi-prolog swi-prolog-nox; \
		elif [ -f /etc/fedora-release ]; then \
			$(ECHO) "$(YELLOW)Installing via dnf...$(NC)"; \
			sudo dnf install -y pl pl-devel; \
		elif [ -f /etc/arch-release ]; then \
			$(ECHO) "$(YELLOW)Installing via pacman...$(NC)"; \
			sudo pacman -S --noconfirm swi-prolog; \
		else \
			$(ECHO) "$(YELLOW)Unknown Linux distro, trying apt-get...$(NC)"; \
			sudo apt-get install -y swi-prolog swi-prolog-nox || true; \
		fi; \
	elif [ "$$UNAME_S" = "Darwin" ]; then \
		if ! command -v brew >/dev/null 2>&1; then \
			$(ECHO) "$(RED)✗ Homebrew required. Install from https://brew.sh$(NC)"; \
			exit 1; \
		fi; \
		$(ECHO) "$(YELLOW)Installing via Homebrew...$(NC)"; \
		brew install swi-prolog; \
	elif echo "$$UNAME_S" | grep -qE "^(MINGW|MSYS|CYGWIN)"; then \
		$(ECHO) "$(YELLOW)Windows: Please install SWI-Prolog manually$(NC)"; \
		$(ECHO) "$(YELLOW)Download from: https://www.swi-prolog.org/download/stable$(NC)"; \
		$(ECHO) "$(YELLOW)Or use: choco install swi-prolog$(NC)"; \
		exit 1; \
	else \
		$(ECHO) "$(RED)✗ Unsupported system: $$UNAME_S$(NC)"; \
		exit 1; \
	fi
	@if command -v swipl >/dev/null 2>&1; then \
		$(ECHO) "$(GREEN)✓ SWI-Prolog installed successfully$(NC)"; \
		swipl --version; \
	else \
		$(ECHO) "$(RED)✗ Installation failed$(NC)"; \
		exit 1; \
	fi

# Check dependencies
.PHONY: check-deps
check-deps:
	@$(ECHO) "$(CYAN)▶ Checking dependencies...$(NC)"
	@command -v python3 >/dev/null 2>&1 || { $(ECHO) "$(RED)✗ python3 missing$(NC)"; exit 1; }
	@command -v git >/dev/null 2>&1 || { $(ECHO) "$(RED)✗ git missing$(NC)"; exit 1; }
	@command -v scons >/dev/null 2>&1 || { $(ECHO) "$(RED)✗ scons missing (pip install scons)$(NC)"; exit 1; }
	@command -v swipl >/dev/null 2>&1 || { $(ECHO) "$(RED)✗ swipl missing. Call make install-swi first!$(NC)"; exit 1; }
	@command -v pkg-config >/dev/null 2>&1 || { $(ECHO) "$(RED)✗ pkg-config missing$(NC)"; exit 1; }
	@pkg-config --exists swipl 2>/dev/null || { $(ECHO) "$(RED)✗ pkg-config cannot find swipl$(NC)"; exit 1; }
	@$(ECHO) "$(GREEN)✓ All dependencies present$(NC)"


# Build both debug and release + setup projects
.PHONY: all
all: check-deps debug release setup-demos
	@$(ECHO) "$(CYAN)▶ Building release version...$(NC)"
	@scons --godot-cpp=$(GODOT_CPP) target=template_release arch=$(UNAME_M)
	@$(ECHO) "$(CYAN)▶ Building debug version...$(NC)"
	@scons --godot-cpp=$(GODOT_CPP) target=template_debug arch=$(UNAME_M)
	@$(MAKE) setup-demos

# Debug build
.PHONY: debug
debug: check-deps
	@$(ECHO) "$(CYAN)▶ Building debug version...$(NC)"
	@scons --godot-cpp=$(GODOT_CPP) target=template_debug arch=$(UNAME_M)
	@$(MAKE) setup-demos

# Release build
.PHONY: release
release: check-deps
	@$(ECHO) "$(CYAN)▶ Building release version...$(NC)"
	@scons --godot-cpp=$(GODOT_CPP) target=template_release arch=$(UNAME_M)
	@$(MAKE) setup-demos

# Clean build artifacts
.PHONY: clean
clean:
	@$(ECHO) "$(YELLOW)▶ Cleaning...$(NC)"
	@rm -fr $(BIN) .sconsign.dblite src/*.os godot-cpp-* .scons_cache prologot.gdextension
	@for project in demos/showcases demos/galactic_customs tests; do \
		rm -rf $$project/$(BIN) $$project/prologot.gdextension; \
	done
	@$(ECHO) "$(GREEN)✓ Clean completed$(NC)"
	@$(ECHO) "$(YELLOW)Note: .godot/ directories are preserved for faster project loading$(NC)"

# Set up demo and test projects (creates symbolic links to $(BIN)/ and prologot.gdextension)
.PHONY: setup-demos
setup-demos:
	@$(ECHO) "$(CYAN)▶ Setting up demo and test projects...$(NC)"
	@if [ ! -d $(BIN) ]; then \
		$(ECHO) "$(RED)✗ Error: $(BIN)/ directory not found. Run 'make debug' or 'make release' first$(NC)"; \
		exit 1; \
	fi
	@for project in demos/showcases demos/galactic_customs; do \
		rm -rf $$project/$(BIN) $$project/prologot.gdextension; \
		[ -d $(BIN) ] && ln -sf ../../$(BIN) $$project/$(BIN) || true; \
		[ -f prologot.gdextension ] && ln -sf ../../prologot.gdextension $$project/prologot.gdextension || true; \
	done
	@rm -rf tests/$(BIN) tests/prologot.gdextension; \
	[ -d $(BIN) ] && ln -sf ../$(BIN) tests/$(BIN) || true; \
	[ -f prologot.gdextension ] && ln -sf ../prologot.gdextension tests/prologot.gdextension || true
	@$(ECHO) "$(YELLOW)▶ Initializing Godot projects (creating .godot cache)...$(NC)"
	@for project in demos/showcases demos/galactic_customs tests; do \
		if [ -f $$project/project.godot ] && [ ! -d $$project/.godot ]; then \
			$(ECHO) "$(CYAN)  Initializing $$project...$(NC)"; \
			godot --headless --quit --path $$project 2>&1 | grep -v "^Godot Engine" || true; \
		fi; \
	done
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)╔════════════════════════════════════════════════════════╗$(NC)"
	@$(ECHO) "$(GREEN)║  ✓ Demo and test projects ready!                       ║$(NC)"
	@$(ECHO) "$(GREEN)╚════════════════════════════════════════════════════════╝$(NC)"
	@$(ECHO) ""
	@$(ECHO) "$(CYAN)▶ To run the demo:$(NC)"
	@$(ECHO) "   $(YELLOW)make run-demo$(NC)"
	@$(ECHO) ""
	@$(ECHO) "$(CYAN)▶ To run galactic_customs:$(NC)"
	@$(ECHO) "   $(YELLOW)make run-galactic_customs$(NC)"
	@$(ECHO) ""
	@$(ECHO) "$(CYAN)▶ To run tests:$(NC)"
	@$(ECHO) "   $(YELLOW)make tests$(NC)"
	@$(ECHO) ""

# Run tests
.PHONY: tests
tests: setup-demos
	@$(ECHO) "$(CYAN)▶ Running tests...$(NC)"
	@$(ECHO) "$(YELLOW)Note: Tests require Godot to be installed$(NC)"
	@godot --headless --path tests -s run_tests.gd

# Run demo project
.PHONY: run-demo
run-demo: setup-demos
	@$(ECHO) "$(CYAN)▶ Running demo project...$(NC)"
	@godot --path demos/showcases

# Run galactic_customs game
.PHONY: run-galactic
run-galactic: setup-demos
	@$(ECHO) "$(CYAN)▶ Running galactic_customs game...$(NC)"
	@godot --path demos/galactic_customs

# Format C++ sources with clang-format
.PHONY: format
format:
	@$(ECHO) "$(CYAN)▶ Formatting C++ sources...$(NC)"
	@find src/ -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | xargs clang-format -i
	@$(ECHO) "$(GREEN)✓ Formatting completed$(NC)"