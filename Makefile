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

# Godot godot-cpp tag or branch (e.g., "4.6", "godot-4.3-stable")
# build.py will parse this to extract version and determine if it's a tag or branch
GODOT_CPP ?= 4.4

# Default target
.PHONY: help
help:
	@$(ECHO) "$(CYAN)╔════════════════════════════════════════════════════════╗$(NC)"
	@$(ECHO) "$(CYAN)║          Prologot GDExtension v$(VERSION)                   ║$(NC)"
	@$(ECHO) "$(CYAN)╚════════════════════════════════════════════════════════╝$(NC)"
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)Main commands:$(NC)"
	@$(ECHO) "  $(YELLOW)make setup$(NC)         - Setup project (SWI-Prolog + godot-cpp + build)"
	@$(ECHO) "  $(YELLOW)make debug$(NC)         - Compile the extension in debug mode"
	@$(ECHO) "  $(YELLOW)make release$(NC)       - Compile the extension in release mode"
	@$(ECHO) "  $(YELLOW)make all$(NC)           - Build debug + release + setup projects"
	@$(ECHO) "  $(YELLOW)make clean$(NC)         - Clean compiled files"
	@$(ECHO) "  $(YELLOW)make tests$(NC)          - Run tests"
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)Advanced commands:$(NC)"
	@$(ECHO) "  $(YELLOW)make install-swi$(NC)   - Install SWI-Prolog only"
	@$(ECHO) "  $(YELLOW)make godot-cpp$(NC)     - Clone and compile godot-cpp"
	@$(ECHO) "  $(YELLOW)make update$(NC)        - Git update godot-cpp (no compile)"
	@$(ECHO) "  $(YELLOW)make check-deps$(NC)    - Check dependencies"
	@$(ECHO) "  $(YELLOW)make format$(NC)        - Format C++ sources"
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)Demo commands:$(NC)"
	@$(ECHO) "  $(YELLOW)make setup-internal-projects$(NC) - Set up demo and test projects"
	@$(ECHO) "  $(YELLOW)make run-demo$(NC)      - Run demo in Godot"
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)Configuration:$(NC)"
	@$(ECHO) "  $(YELLOW)GODOT_CPP$(NC) - Godot godot-cpp ref (default: $(GODOT_CPP))"
	@$(ECHO) ""
	@echo "  Examples:"
	@echo "    make GODOT_CPP=4.3.1 setup"
	@echo "    make GODOT_CPP=godot-4.3-stable godot-cpp"
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)Using godot-cpp:$(NC) $(GODOT_CPP)"

# Setup project (SWI-Prolog + godot-cpp + build)
.PHONY: setup
setup:
	@$(ECHO) "$(CYAN)▶ Setting up project...$(NC)"
	python3 build.py --godot-cpp $(GODOT_CPP)

# Install SWI-Prolog only
.PHONY: install-swi
install-swi:
	@$(ECHO) "$(CYAN)▶ Installing SWI-Prolog only...$(NC)"
	@python3 build.py --skip-godot-cpp --skip-compile

# Clone and compile godot-cpp
.PHONY: godot-cpp
godot-cpp: clean
	@$(ECHO) "$(CYAN)▶ Setting up and compiling godot-cpp for: $(GODOT_CPP)...$(NC)"
	python3 build.py --godot-cpp $(GODOT_CPP) --skip-swi --compile-godot-cpp-only
	@$(ECHO) "$(GREEN)✓ godot-cpp compiled$(NC)"

# Update godot-cpp to a specific tag/branch (without compiling)
.PHONY: update
update: clean
	@$(ECHO) "$(CYAN)▶ Updating godot-cpp to: $(GODOT_CPP)...$(NC)"
	python3 build.py --godot-cpp $(GODOT_CPP) --skip-swi --skip-compile --force
	@$(ECHO) "$(GREEN)✓ godot-cpp updated$(NC)"
	@$(ECHO) "$(YELLOW)⚠ Run 'make debug' or 'make release' to rebuild$(NC)"

# Debug build
.PHONY: debug
debug:
	@$(ECHO) "$(CYAN)▶ Building debug version...$(NC)"
	python3 build.py --godot-cpp $(GODOT_CPP) --target template_debug --skip-swi $(if $(ARCH),--arch $(ARCH))

# Release build
.PHONY: release
release:
	@$(ECHO) "$(CYAN)▶ Building release version...$(NC)"
	python3 build.py --godot-cpp $(GODOT_CPP) --target template_release --skip-swi $(if $(ARCH),--arch $(ARCH))

# Build both debug and release + setup projects
.PHONY: all
all: debug release setup-internal-projects
	@$(ECHO) "$(GREEN)✓ Full build completed$(NC)"

# Clean build artifacts
.PHONY: clean
clean:
	@$(ECHO) "$(YELLOW)▶ Cleaning...$(NC)"
	@rm -fr bin/*.so bin/*.dll bin/*.dylib bin/*.framework .sconsign.dblite src/*.os
	@rm -fr godot-cpp-* bin __pycache__
	@$(ECHO) "$(GREEN)✓ Clean completed$(NC)"

# Set up demo and test projects (copies bin/ and prologot.gdextension)
.PHONY: setup-internal-projects
setup-internal-projects:
	@$(ECHO) "$(CYAN)▶ Setting up demo and test projects...$(NC)"
	@for project in addons/prologot/demos tests; do \
		rm -rf $$project/bin; \
		cp -r bin $$project/bin 2>/dev/null || true; \
		cp prologot.gdextension $$project/prologot.gdextension 2>/dev/null || true; \
	done
	@$(ECHO) ""
	@$(ECHO) "$(GREEN)╔════════════════════════════════════════════════════════╗$(NC)"
	@$(ECHO) "$(GREEN)║  ✓ Demo and test projects ready!                       ║$(NC)"
	@$(ECHO) "$(GREEN)╚════════════════════════════════════════════════════════╝$(NC)"
	@$(ECHO) ""
	@$(ECHO) "$(CYAN)▶ To run the demo:$(NC)"
	@$(ECHO) "   $(YELLOW)make run-demo$(NC)"
	@$(ECHO) ""
	@$(ECHO) "$(CYAN)▶ To run tests:$(NC)"
	@$(ECHO) "   $(YELLOW)make tests$(NC)"
	@$(ECHO) ""

# Run tests
.PHONY: test
test: setup-internal-projects
	@$(ECHO) "$(CYAN)▶ Running tests...$(NC)"
	@$(ECHO) "$(YELLOW)Note: Tests require Godot to be installed$(NC)"
	@godot --headless --path tests -s run_tests.gd

# Run demo project
.PHONY: run-demo
run-demo: setup-internal-projects
	@$(ECHO) "$(CYAN)▶ Running demo project...$(NC)"
	@godot --path addons/prologot/demos

# Check dependencies
.PHONY: check-deps
check-deps:
	@$(ECHO) "$(CYAN)▶ Checking dependencies...$(NC)"
	@command -v python3 >/dev/null 2>&1 || { $(ECHO) "$(RED)✗ python3 missing$(NC)"; exit 1; }
	@command -v git >/dev/null 2>&1 || { $(ECHO) "$(RED)✗ git missing$(NC)"; exit 1; }
	@command -v scons >/dev/null 2>&1 || { $(ECHO) "$(RED)✗ scons missing (pip install scons)$(NC)"; exit 1; }
	@command -v swipl >/dev/null 2>&1 || { $(ECHO) "$(RED)✗ swipl missing$(NC)"; exit 1; }
	@command -v pkg-config >/dev/null 2>&1 || { $(ECHO) "$(RED)✗ pkg-config missing$(NC)"; exit 1; }
	@pkg-config --exists swipl 2>/dev/null || { $(ECHO) "$(RED)✗ pkg-config cannot find swipl$(NC)"; exit 1; }
	@$(ECHO) "$(GREEN)✓ All dependencies present$(NC)"

# Format C++ sources with clang-format
.PHONY: format
format:
	@$(ECHO) "$(CYAN)▶ Formatting C++ sources...$(NC)"
	@find src/ -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | xargs clang-format -i
	@$(ECHO) "$(GREEN)✓ Formatting completed$(NC)"