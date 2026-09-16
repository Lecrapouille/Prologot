# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# Runtime autoload registered by the editor plugin as PrologotEngine.
# consult / solve / atom / … are defined on prologot_facade.gd.

extends "res://addons/prologot/prologot_facade.gd"

const PrologotBoot = preload("res://addons/prologot/prologot_boot.gd")

## Named Prolog source strings for create / switch / list_knowledge_base.
## Keys are names, values are Prolog code. switch_knowledge_base() wipes
## user clauses before loading the chosen source.
var knowledge_bases: Dictionary = {}


###############################################################################
## Initialize the Prologot singleton.
##
## Called when the autoload enters the scene tree (game / F5, not the editor).
## Instantiates a Prologot handle and attach()es to the process-global SWI
## engine (same engine as the editor dock if the plugin already started it).
## engine stays null if the GDExtension failed to load.
###############################################################################
func _ready() -> void:
	engine = PrologotBoot.create_engine()


###############################################################################
## Detach the Prologot handle.
##
## Does not call PL_cleanup(): the editor dock may still be attached to the
## same SWI engine. The last handle in the process resets the user knowledge
## base (see Prologot.cleanup()).
###############################################################################
func _exit_tree() -> void:
	if engine:
		engine.cleanup()
	engine = null


###############################################################################
## Create and load a named knowledge base.
##
## Stores a Prolog code string under kb_name and consult_string()s it
## immediately (adds clauses; does not wipe). Use switch_knowledge_base()
## to replace the user knowledge base with one of these stored sources.
## Useful for game modes, scenarios, or AI configurations.
##
## @param kb_name: Unique name for this knowledge base
## @param code: Prolog source (facts, rules, …)
## @return: true if the code was stored and consulted
###############################################################################
func create_knowledge_base(kb_name: String, code: String) -> bool:
	knowledge_bases[kb_name] = code
	return consult_string(code)


###############################################################################
## Switch to a previously created knowledge base.
##
## clear_knowledge() abolishes user predicates added via consult_* /
## assert_fact, then consult_string()s the stored source. Previous mode
## clauses are gone. Returns false if kb_name was never created.
##
## @param kb_name: Name passed to create_knowledge_base()
## @return: true if the wipe and reload succeeded
###############################################################################
func switch_knowledge_base(kb_name: String) -> bool:
	if kb_name not in knowledge_bases:
		return false
	if not clear_knowledge():
		return false
	return consult_string(knowledge_bases[kb_name])


###############################################################################
## List stored knowledge base names.
##
## Does not inspect SWI: only names registered with create_knowledge_base().
## Useful for UI dropdowns or debugging.
##
## @return: Array of knowledge base name strings
###############################################################################
func list_knowledge_bases() -> Array:
	return knowledge_bases.keys()
