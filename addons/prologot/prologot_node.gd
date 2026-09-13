# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# Scene-tree node: drop it in a scene, assign a PrologKnowledge, query from children.

@tool
class_name PrologotNode
extends Node

## Inspectable knowledge (files + inline Prolog). Assign a PrologKnowledge.
@export var knowledge: Resource

## Extra .pl files consulted after the Resource.
@export var consult_files: PackedStringArray = PackedStringArray()

## Optional SWI-Prolog home when this node creates its own engine.
@export var swipl_home: String = ""

## If true, start() runs from _ready() at runtime (not in the editor).
@export var auto_start: bool = true

## Reuse /root/PrologotEngine when the plugin autoload is present.
@export var use_autoload: bool = true

## The Prologot instance used by this node (autoload or owned).
var engine = null

var _owns_engine: bool = false


func _ready() -> void:
	if Engine.is_editor_hint():
		return
	if auto_start:
		start()


func _exit_tree() -> void:
	if _owns_engine and engine:
		engine.cleanup()
	if _owns_engine:
		engine = null
		_owns_engine = false


func _get_configuration_warnings() -> PackedStringArray:
	if knowledge == null and consult_files.is_empty():
		return PackedStringArray([
			"Assign a PrologKnowledge resource or at least one consult file."
		])
	return PackedStringArray()


## Initializes the engine if needed and loads knowledge / files.
func start() -> bool:
	if not _ensure_engine():
		return false
	return _consult()


## Inject an already initialized engine (tests, shared singleton).
func set_engine(value) -> void:
	if _owns_engine and engine and engine != value:
		engine.cleanup()
	engine = value
	_owns_engine = false


func _ensure_engine() -> bool:
	if engine != null and engine.is_initialized():
		return true

	if use_autoload:
		var autoload = get_node_or_null("/root/PrologotEngine")
		if autoload != null and autoload.get("engine") != null and autoload.engine.is_initialized():
			engine = autoload.engine
			_owns_engine = false
			return true

	if not ClassDB.class_exists("Prologot"):
		push_error("PrologotNode: GDExtension not loaded")
		return false

	engine = ClassDB.instantiate("Prologot")
	var options := {}
	var home := swipl_home
	if home.is_empty():
		var os_map := {"Linux": "linux", "Windows": "windows", "macOS": "macos"}
		home = "res://bin/" + os_map.get(OS.get_name(), OS.get_name().to_lower()) + "/swipl"
	if DirAccess.dir_exists_absolute(home) or DirAccess.dir_exists_absolute(ProjectSettings.globalize_path(home)):
		options["home"] = home
	if not engine.initialize(options):
		push_error("PrologotNode: failed to initialize Prolog")
		engine = null
		return false
	_owns_engine = true
	return true


func _consult() -> bool:
	if engine == null:
		return false
	var ok := true
	if knowledge != null and knowledge.has_method("load_into"):
		ok = knowledge.load_into(engine) and ok
	for path in consult_files:
		if String(path).strip_edges().is_empty():
			continue
		ok = engine.consult_file(path) and ok
	return ok


func atom(name: String):
	return engine.atom(name) if engine else null

func integer(value: int):
	return engine.integer(value) if engine else null

func real(value: float):
	return engine.real(value) if engine else null

func string(value: String):
	return engine.string(value) if engine else null

func nil():
	return engine.nil() if engine else null

func list(items: Array):
	return engine.list(items) if engine else null

func compound(functor: String, args: Array):
	return engine.compound(functor, args) if engine else null

func variable(name: String = ""):
	return engine.variable(name) if engine else null

func anonymous():
	return engine.anonymous() if engine else null

func predicate(name: String):
	return engine.predicate(name) if engine else null

func object(value):
	return engine.object(value) if engine else null

func solve(goal):
	return engine.solve(goal) if engine else null

func consult_file(path: String) -> bool:
	return engine.consult_file(path) if engine else false

func consult_string(code: String) -> bool:
	return engine.consult_string(code) if engine else false

func assert_fact(goal) -> bool:
	return engine.assert_fact(goal) if engine else false

func retract_fact(fact) -> bool:
	return engine.retract_fact(fact) if engine else false

func retract_all(pattern) -> bool:
	return engine.retract_all(pattern) if engine else false

func expose_property(godot_class: String, property: String, pred: String = "") -> bool:
	return engine.expose_property(godot_class, property, pred) if engine else false

func expose_method(godot_class: String, method: String, pred: String = "") -> bool:
	return engine.expose_method(godot_class, method, pred) if engine else false

func unexpose(pred: String, arity: int) -> bool:
	return engine.unexpose(pred, arity) if engine else false

func list_exposed() -> Array:
	return engine.list_exposed() if engine else []

func get_last_error() -> String:
	return engine.get_last_error() if engine else "Engine not initialized"
