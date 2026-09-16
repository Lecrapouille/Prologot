# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Forwards to a Prologot handle. Shared by the autoload and PrologotNode
# so the GDScript API is written once.

extends Node

## Prologot handle (autoload-owned, node-owned, or injected).
var engine: Object = null


func _engine_or_error():
	if engine:
		return engine
	push_error("Prologot: Engine not initialized")
	return null


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

###############################################################################
## Open a lazy query for goal (has_solution / first / for / all).
## max_solutions 0 means unlimited.
###############################################################################
func solve(goal, max_solutions: int = 0):
	var e = _engine_or_error()
	return e.solve(goal, max_solutions) if e else null


###############################################################################
## Load a Prolog file (res://, user://, or filesystem path).
## Clauses are added to the current knowledge base.
##
## @param path: e.g. "res://rules/game_logic.pl"
## @return: true if the file was loaded
###############################################################################
func consult_file(path: String) -> bool:
	var e = _engine_or_error()
	return e.consult_file(path) if e else false


###############################################################################
## Load Prolog source from a string (facts, rules). Added, not replaced.
##
## @param code: Prolog code
## @return: true if the code was loaded
###############################################################################
func consult_string(code: String) -> bool:
	var e = _engine_or_error()
	return e.consult_string(code) if e else false

func assert_fact(goal) -> bool:
	var e = _engine_or_error()
	return e.assert_fact(goal) if e else false

###############################################################################
## Retract one matching clause. Accepts a PrologGoal from predicate.call().
###############################################################################
func retract_fact(fact) -> bool:
	var e = _engine_or_error()
	return e.retract_fact(fact) if e else false

func retract_all(pattern) -> bool:
	var e = _engine_or_error()
	return e.retract_all(pattern) if e else false

func expose_property(godot_class: String, property: String, pred: String = "") -> bool:
	var e = _engine_or_error()
	return e.expose_property(godot_class, property, pred) if e else false

func expose_method(godot_class: String, method: String, pred: String = "") -> bool:
	var e = _engine_or_error()
	return e.expose_method(godot_class, method, pred) if e else false

func unexpose(pred: String, arity: int) -> bool:
	var e = _engine_or_error()
	return e.unexpose(pred, arity) if e else false

func list_exposed() -> Array:
	return engine.list_exposed() if engine else []

###############################################################################
## Abolish user predicates added via consult_* / assert_fact.
## The handle stays attached; SWI keeps running.
###############################################################################
func clear_knowledge() -> bool:
	var e = _engine_or_error()
	return e.clear_knowledge() if e else false


###############################################################################
## Last error from the Prologot handle, or "Engine not initialized".
###############################################################################
func get_last_error() -> String:
	return engine.get_last_error() if engine else "Engine not initialized"
