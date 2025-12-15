# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# This singleton provides global access to the Prologot engine.
# It is automatically registered as an autoload when the plugin is enabled.

extends Node

## The Prologot engine instance for executing Prolog queries.
var engine = null  # Will be set to Prologot instance when ready

## Dictionary storing named knowledge bases for easy switching.
var knowledge_bases: Dictionary = {}


func _ready() -> void:
	# Check if Prologot class is available (GDExtension must be loaded)
	if not ClassDB.class_exists("Prologot"):
		engine = null
		push_error("Prologot: GDExtension not loaded. Make sure prologot.gdextension is in your project root.")
		return

	engine = ClassDB.instantiate("Prologot")
	if not engine.initialize():
		engine = null
		push_error("Prologot: Failed to initialize Prolog engine")


# =============================================================================
# Simplified API for global access
# =============================================================================

## Execute a Prolog query and return true if it succeeds.
func query(goal: String) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.query(goal)


## Execute a Prolog query and return all solutions as Array of Variants.
func query_all(goal: String) -> Array:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return []
	return engine.query_all(goal)


## Get the last error message from Prolog.
func get_last_error() -> String:
	if not engine:
		return "Engine not initialized"
	return engine.get_last_error()


## Execute a Prolog query and return the first solution (null if none).
func query_one(goal: String) -> Variant:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return null
	return engine.query_one(goal)


## Load a Prolog file from the given path (supports res:// paths).
func load_file(path: String) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.consult(path)


## Load Prolog code from a string.
func load_code(code: String) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.consult_string(code)


## Assert a new fact into the Prolog knowledge base.
func add_fact(fact: String) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.assert_fact(fact)


## Retract a fact from the Prolog knowledge base.
func remove_fact(fact: String) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.retract_fact(fact)


## Call a Prolog predicate with the given arguments.
func call_predicate(predicate: String, args: Array) -> bool:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return false
	return engine.call_predicate(predicate, args)


## Call a Prolog predicate and return the result.
func call_with_result(predicate: String, args: Array) -> Variant:
	if not engine:
		push_error("Prologot: Engine not initialized")
		return null
	return engine.call_function(predicate, args)


# =============================================================================
# Knowledge Base Management
# =============================================================================

## Create and load a named knowledge base.
func create_knowledge_base(kb_name: String, code: String) -> bool:
	knowledge_bases[kb_name] = code
	return load_code(code)


## Switch to a previously created knowledge base.
func switch_knowledge_base(kb_name: String) -> bool:
	if kb_name in knowledge_bases:
		return load_code(knowledge_bases[kb_name])
	return false


## Get the list of available knowledge bases.
func list_knowledge_bases() -> Array:
	return knowledge_bases.keys()


func _exit_tree() -> void:
	if engine:
		engine.cleanup()
	engine = null
