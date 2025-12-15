# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# Makefile for building and managing the Prologot GDExtension

# Terminal colors
CYAN := \033[0;36m
GREEN := \033[0;32m
YELLOW := \033[0;33m
RED := \033[0;31m
NC := \033[0m

# Godot godot-cpp tag or branch (e.g., "4.6", "godot-4.3-stable")
# build.py will parse this to extract version and determine if it's a tag or branch
GODOT_CPP ?= 4.4

# Default target
.PHONY: help
help:
	@echo "$(CYAN)╔════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(CYAN)║          Prologot GDExtension v$(VERSION)                   ║$(NC)"
	@echo "$(CYAN)╚════════════════════════════════════════════════════════╝$(NC)"
	@echo ""
	@echo "$(GREEN)Main commands:$(NC)"
	@echo "  $(YELLOW)make setup$(NC)         - Setup project (SWI-Prolog + godot-cpp + build)"
	@echo "  $(YELLOW)make debug$(NC)         - Compile the extension in debug mode"
	@echo "  $(YELLOW)make release$(NC)       - Compile the extension in release mode"
	@echo "  $(YELLOW)make all$(NC)           - Build debug + release + setup demo"
	@echo "  $(YELLOW)make clean$(NC)         - Clean compiled files"
	@echo "  $(YELLOW)make test$(NC)          - Run tests"
	@echo ""
	@echo "$(GREEN)Advanced commands:$(NC)"
	@echo "  $(YELLOW)make install-swi$(NC)   - Install SWI-Prolog only"
	@echo "  $(YELLOW)make godot-cpp$(NC)     - Clone and compile godot-cpp"
	@echo "  $(YELLOW)make update$(NC)        - Git update godot-cpp (no compile)"
	@echo "  $(YELLOW)make check-deps$(NC)    - Check dependencies"
	@echo "  $(YELLOW)make format$(NC)        - Format C++ sources"
	@echo ""
	@echo "$(GREEN)Demo commands:$(NC)"
	@echo "  $(YELLOW)make setup-demo$(NC)    - Set up demo project"
	@echo "  $(YELLOW)make run-demo$(NC)      - Run demo in Godot"
	@echo ""
	@echo "$(GREEN)Configuration:$(NC)"
	@echo "  $(YELLOW)GODOT_CPP$(NC) - Godot godot-cpp ref (default: $(GODOT_CPP))"
	@echo ""
	@echo "  Examples:"
	@echo "    make GODOT_CPP=4.3.1 setup"
	@echo "    make GODOT_CPP=godot-4.3-stable godot-cpp"
	@echo ""
	@echo "$(GREEN)Using godot-cpp:$(NC) $(GODOT_CPP)"

# Setup project (SWI-Prolog + godot-cpp + build)
.PHONY: setup
setup:
	@echo "$(CYAN)▶ Setting up project...$(NC)"
	python3 build.py --godot-cpp $(GODOT_CPP)

# Install SWI-Prolog only
.PHONY: install-swi
install-swi:
	@echo "$(CYAN)▶ Installing SWI-Prolog only...$(NC)"
	@python3 build.py --skip-godot-cpp --skip-compile

# Clone and compile godot-cpp
.PHONY: godot-cpp
godot-cpp: clean
	@echo "$(CYAN)▶ Setting up and compiling godot-cpp for: $(GODOT_CPP)...$(NC)"
	python3 build.py --godot-cpp $(GODOT_CPP) --skip-swi --compile-godot-cpp-only
	@echo "$(GREEN)✓ godot-cpp compiled$(NC)"

# Update godot-cpp to a specific tag/branch (without compiling)
.PHONY: update
update: clean
	@echo "$(CYAN)▶ Updating godot-cpp to: $(GODOT_CPP)...$(NC)"
	python3 build.py --godot-cpp $(GODOT_CPP) --skip-swi --skip-compile --force
	@echo "$(GREEN)✓ godot-cpp updated$(NC)"
	@echo "$(YELLOW)⚠ Run 'make debug' or 'make release' to rebuild$(NC)"

# Debug build
.PHONY: debug
debug:
	@echo "$(CYAN)▶ Building debug version...$(NC)"
	python3 build.py --godot-cpp $(GODOT_CPP) --target template_debug --skip-swi $(if $(ARCH),--arch $(ARCH))

# Release build
.PHONY: release
release:
	@echo "$(CYAN)▶ Building release version...$(NC)"
	python3 build.py --godot-cpp $(GODOT_CPP) --target template_release --skip-swi $(if $(ARCH),--arch $(ARCH))

# Build both debug and release + setup demo
.PHONY: all
all: debug release setup-demo
	@echo "$(GREEN)✓ Full build completed$(NC)"

# Clean build artifacts
.PHONY: clean
clean:
	@echo "$(YELLOW)▶ Cleaning...$(NC)"
	@rm -fr bin/*.so bin/*.dll bin/*.dylib bin/*.framework .sconsign.dblite src/*.os
	@rm -fr godot-cpp-* bin __pycache__
	@echo "$(GREEN)✓ Clean completed$(NC)"

# Run tests
.PHONY: test
test:
	@echo "$(CYAN)▶ Running tests...$(NC)"
	@echo "$(YELLOW)Note: Tests require Godot to be installed$(NC)"
	@echo "Run: godot --headless --path . -s tests/run_tests.gd"

# Set up demo project with symlinks
.PHONY: setup-demo
setup-demo:
	@echo "$(CYAN)▶ Setting up demo project...$(NC)"
	@cp -r bin addons/prologot/demos/bin 2>/dev/null || true
	@cp prologot.gdextension addons/prologot/demos/prologot.gdextension 2>/dev/null || true
	@echo ""
	@echo "$(GREEN)╔════════════════════════════════════════════════════════╗$(NC)"
	@echo "$(GREEN)║  ✓ Demo project ready!                                 ║$(NC)"
	@echo "$(GREEN)╚════════════════════════════════════════════════════════╝$(NC)"
	@echo ""
	@echo "$(CYAN)▶ To run the demo:$(NC)"
	@echo "   $(YELLOW)make run-demo$(NC)"
	@echo ""
	@echo "$(CYAN)▶ Or manually:$(NC)"
	@echo "   $(YELLOW)godot --path addons/prologot/demos$(NC)"
	@echo ""

# Run demo project
.PHONY: run-demo
run-demo: setup-demo
	@echo "$(CYAN)▶ Running demo project...$(NC)"
	@godot --path addons/prologot/demos

# Check dependencies
.PHONY: check-deps
check-deps:
	@echo "$(CYAN)▶ Checking dependencies...$(NC)"
	@command -v python3 >/dev/null 2>&1 || { echo "$(RED)✗ python3 missing$(NC)"; exit 1; }
	@command -v git >/dev/null 2>&1 || { echo "$(RED)✗ git missing$(NC)"; exit 1; }
	@command -v scons >/dev/null 2>&1 || { echo "$(RED)✗ scons missing (pip install scons)$(NC)"; exit 1; }
	@command -v swipl >/dev/null 2>&1 || { echo "$(RED)✗ swipl missing$(NC)"; exit 1; }
	@echo "$(GREEN)✓ All dependencies present$(NC)"

# Format C++ sources with clang-format
.PHONY: format
format:
	@echo "$(CYAN)▶ Formatting C++ sources...$(NC)"
	@find src/ -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | xargs clang-format -i
	@echo "$(GREEN)✓ Formatting completed$(NC)"