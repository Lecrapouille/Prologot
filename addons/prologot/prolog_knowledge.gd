# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# Inspectable Resource: Prolog files and/or inline clauses to load into an engine.

@tool
class_name PrologKnowledge
extends Resource

## Prolog files consulted in order (res://, user://, or absolute).
@export var files: PackedStringArray = PackedStringArray()

## Extra clauses and rules, loaded after the files.
@export_multiline var code: String = ""


## Consults files then inline code into an initialized Prologot engine.
func load_into(engine) -> bool:
	if engine == null or not engine.is_initialized():
		push_error("PrologKnowledge: engine is not initialized")
		return false

	var ok := true
	for path in files:
		if String(path).strip_edges().is_empty():
			continue
		if not engine.consult_file(path):
			push_error("PrologKnowledge: failed to consult %s" % path)
			ok = false

	var src := code.strip_edges()
	if not src.is_empty() and not engine.consult_string(src):
		push_error("PrologKnowledge: failed to consult inline code")
		ok = false
	return ok
