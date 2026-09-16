# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Scene-tree node: drop it in a scene, assign a PrologKnowledge, query from children.
# solve / consult / atom / … come from prologot_facade.gd.

@tool
class_name PrologotNode
extends "res://addons/prologot/prologot_facade.gd"

const PrologotBoot = preload("res://addons/prologot/prologot_boot.gd")

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

	engine = PrologotBoot.create_engine(swipl_home)
	if engine == null:
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
