# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# This singleton provides global access to the Prologot engine.
# It is automatically registered as an autoload when the plugin is enabled.

extends Node

## The Prologot engine instance for executing Prolog queries.
## This is set to null initially and will be initialized in _ready()
var engine = null

## Dictionary storing named knowledge bases for easy switching.
## Keys are knowledge base names (strings), values are Prolog code strings.
## Allows users to quickly switch between different Prolog knowledge bases.
var knowledge_bases: Dictionary = {}

###############################################################################
## Initialize the Prologot singleton.
##
## This function is called automatically when the singleton enters the scene tree.
## It checks for the Prologot GDExtension, instantiates the engine, and initializes
## the Prolog runtime. If initialization fails, error messages are logged and
## the engine remains null.
###############################################################################
func _ready() -> void:
	# Verify that the Prologot GDExtension is loaded and available
	# The GDExtension must be properly configured in the project
	if not ClassDB.class_exists("Prologot"):
		engine = null
		push_error("Prologot: GDExtension not loaded. Make sure bin/prologot.gdextension exists in your project.")
		return

	# Create a new instance of the Prologot engine
	engine = ClassDB.instantiate("Prologot")

	# Attempt to initialize the underlying Prolog engine
	# If initialization fails, log an error and set engine to null
	if not engine.initialize():
		engine = null
		push_error("Prologot: Failed to initialize Prolog engine")

###############################################################################
## Cleanup the Prologot singleton.
##
## Called when the singleton is removed from the scene tree. Properly cleans up
## the Prolog engine resources and releases memory. This is important to prevent
## memory leaks and ensure clean shutdown.
###############################################################################
func _exit_tree() -> void:
	if engine:
		# Clean up Prolog engine resources (terminate SWI-Prolog, free memory, etc.)
		engine.cleanup()
	# Ensure engine reference is cleared
	engine = null

###############################################################################
## Execute a Prolog query and return true if it succeeds.
##
## This is a simple boolean query - useful for checking if a fact is true
## or if a goal can be satisfied. For queries with variables, use query_all()
## or query_one() instead.
##
## @param goal: The Prolog query as a string (e.g., "parent(tom, bob)")
## @return: true if the query succeeds, false otherwise
###############################################################################
func query(goal: String) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.query(goal)

###############################################################################
## Execute a Prolog query and return all solutions as Array of Variants.
##
## This method collects all possible solutions to a query with variables.
## Each solution is returned as a Variant (which may be a Dictionary for compound
## terms, an Array for lists, or a basic type for atoms/numbers).
##
## @param goal: The Prolog query as a string (e.g., "parent(tom, X)")
## @return: An array of all solutions found, or an empty array if none
###############################################################################
func query_all(goal: String) -> Array:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return []
	return engine.query_all(goal)

###############################################################################
## Get the last error message from Prolog.
##
## Retrieves the most recent error message from the Prolog engine, useful for
## debugging failed queries or operations.
##
## @return: The error message string, or "Engine not initialized" if the engine is unavailable
###############################################################################
func get_last_error() -> String:
	if not engine:
		return "Engine not initialized"
	return engine.get_last_error()

###############################################################################
## Execute a Prolog query and return the first solution (null if none).
##
## This is a convenience method that returns only the first solution to a query.
## More efficient than query_all() if you only need one result. Useful when you
## know there should be exactly one solution or you only care about the first one.
##
## @param goal: The Prolog query as a string (e.g., "game_state(level, X)")
## @return: The first solution as a Variant, or null if no solution exists
###############################################################################
func query_one(goal: String) -> Variant:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return null
	return engine.query_one(goal)

###############################################################################
## Load a Prolog file from the given path (supports res:// paths).
##
## Loads and executes Prolog code from a file. The file path can be relative to
## the project root, absolute, or use the res:// protocol. The loaded code
## becomes part of the current knowledge base.
##
## @param path: Path to the Prolog file (e.g., "res://rules/game_logic.pl")
## @return: true if the file was loaded successfully, false otherwise
###############################################################################
func consult_file(path: String) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.consult_file(path)


###############################################################################
## Load Prolog code from a string.
##
## Dynamically loads and executes Prolog code from a string. This is useful for
## runtime code generation or loading code from external sources (network, etc.).
## The code is added to the current knowledge base.
##
## @param code: Prolog code as a string (facts, rules, etc.)
## @return: true if the code was loaded successfully, false otherwise
###############################################################################
func consult_string(code: String) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.consult_string(code)

###############################################################################
## Assert a new fact into the Prolog knowledge base.
##
## Adds a new fact to the knowledge base at runtime. The fact must be in Prolog
## syntax (e.g., "parent(tom, bob)" or "game_state(level, 5)"). This allows
## dynamic modification of the knowledge base during game execution.
##
## @param fact: The Prolog fact to add as a string
## @return: true if the fact was successfully added, false otherwise
###############################################################################
func add_fact(fact: String) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.add_fact(fact)

###############################################################################
## Retract a fact from the Prolog knowledge base.
##
## Removes a specific fact from the knowledge base. The fact string must match
## exactly (including arguments) for it to be removed. This allows runtime
## modification of the knowledge base.
##
## @param fact: The Prolog fact to remove as a string (must match exactly)
## @return: true if the fact was successfully removed, false otherwise
###############################################################################
func retract_fact(fact: String) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.retract_fact(fact)

###############################################################################
## Call a Prolog predicate with the given arguments.
##
## Calls a Prolog predicate with arguments passed as a GDScript array. Returns
## true if the predicate succeeds. This is useful for calling predicates that
## don't return values but perform checks or side effects.
##
## @param predicate: The name of the predicate (e.g., "one_shot_kill")
## @param args: Array of arguments to pass to the predicate
## @return: true if the predicate succeeds, false otherwise
###############################################################################
func call_predicate(predicate: String, args: Array) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.call_predicate(predicate, args)


###############################################################################
## Call a Prolog predicate and return the result.
##
## Calls a Prolog predicate and returns its result value. This is used for
## predicates that compute and return values (functions). The result can be
## a number, string, list, or compound term.
##
## @param predicate: The name of the predicate/function (e.g., "damage")
## @param args: Array of input arguments
## @return: The result value as a Variant, or null if the predicate fails
###############################################################################
func call_function(predicate: String, args: Array) -> Variant:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return null
	return engine.call_function(predicate, args)


###############################################################################
## Create and load a named knowledge base.
##
## Stores a Prolog code string under a name and immediately loads it into the
## engine. This allows switching between different knowledge bases at runtime.
## Useful for different game modes, scenarios, or AI configurations.
##
## @param kb_name: A unique name to identify this knowledge base
## @param code: The Prolog code for this knowledge base
## @return: true if the knowledge base was created and loaded successfully
###############################################################################
func create_knowledge_base(kb_name: String, code: String) -> bool:
	# Store the code for later retrieval
	knowledge_bases[kb_name] = code
	# Load it into the engine immediately
	return consult_string(code)

###############################################################################
## Switch to a previously created knowledge base.
##
## Switches the active knowledge base by loading a previously stored one.
## Note: This will replace the current knowledge base. If you need to preserve
## the current state, consider saving it first.
##
## @param kb_name: The name of the knowledge base to switch to
## @return: true if the switch was successful, false if the name doesn't exist
###############################################################################
func switch_knowledge_base(kb_name: String) -> bool:
	# Check if the knowledge base exists
	if kb_name in knowledge_bases:
		# Load the stored code into the engine
		return consult_string(knowledge_bases[kb_name])
	return false

###############################################################################
## Get the list of available knowledge bases.
##
## Returns an array of all knowledge base names that have been created.
## Useful for UI dropdowns or debugging.
##
## @return: An array of knowledge base name strings
###############################################################################
func list_knowledge_bases() -> Array:
	return knowledge_bases.keys()