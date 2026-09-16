# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# This is the main editor plugin that registers the Prologot dock
# and autoload singleton.

@tool
extends EditorPlugin

const PrologotDock = preload("res://addons/prologot/prologot_dock.gd")
const PrologotBoot = preload("res://addons/prologot/prologot_boot.gd")

var dock: Control

## Editor-only Prologot handle for the dock. Distinct from the runtime
## autoload PrologotEngine, but both attach to the same process-global SWI.
var editor_engine: Object = null

###############################################################################
## Called when the plugin is enabled/loaded in the editor.
##
## Sets up the Prologot plugin by:
## 1. Registering the autoload singleton for runtime use
## 2. Creating a separate Prologot engine instance for the editor dock
## 3. Creating and adding the interactive console dock to the editor
###############################################################################
func _enter_tree() -> void:
	# Runtime autoload (game / F5). Not @tool: it is not created in the editor tree.
	add_autoload_singleton("PrologotEngine", "res://addons/prologot/prologot_singleton.gd")

	# Dock handle. Same bundled SWI home as the autoload; initialize() attaches
	# if the editor already started SWI, otherwise starts it once.
	editor_engine = PrologotBoot.create_engine()
	if editor_engine:
		print("Prologot: Editor engine initialized")

	dock = PrologotDock.new()
	dock.set_engine(editor_engine)
	add_control_to_dock(DOCK_SLOT_RIGHT_BL, dock)

	var base := get_editor_interface().get_base_control()
	add_custom_type(
		"PrologotNode",
		"Node",
		preload("res://addons/prologot/prologot_node.gd"),
		base.get_theme_icon("Node", "EditorIcons")
	)
	add_custom_type(
		"PrologKnowledge",
		"Resource",
		preload("res://addons/prologot/prolog_knowledge.gd"),
		base.get_theme_icon("Resource", "EditorIcons")
	)

	print("Prologot: Plugin enabled")


###############################################################################
## Called when the plugin is disabled/unloaded from the editor.
##
## Cleans up all plugin resources:
## 1. Removes the dock
## 2. Detaches the editor Prologot handle (does not PL_cleanup)
## 3. Unregisters the autoload
###############################################################################
func _exit_tree() -> void:
	# Drop the dock reference first so it cannot query a freed handle.
	if dock:
		dock.set_engine(null)
		remove_control_from_docks(dock)
		dock.queue_free()
		dock = null

	# Detach this handle only. Does not PL_cleanup(); that is the GDExtension unload.
	if editor_engine:
		editor_engine.cleanup()
		editor_engine = null

	remove_custom_type("PrologotNode")
	remove_custom_type("PrologKnowledge")

	# Remove the autoload singleton registration
	# This prevents the singleton from being available in future editor sessions
	remove_autoload_singleton("PrologotEngine")

	print("Prologot: Plugin disabled")


###############################################################################
## EditorPlugin interface methods
###############################################################################

## Returns whether this plugin has a main screen.
## Prologot only provides a dock, not a main screen, so this returns false.
func _has_main_screen() -> bool:
	return false


## Returns the name of the plugin as it appears in the editor.
func _get_plugin_name() -> String:
	return "Prologot"


## Returns the icon to use for this plugin in the editor.
## Uses the default Script icon as a placeholder.
func _get_plugin_icon() -> Texture2D:
	return get_editor_interface().get_base_control().get_theme_icon("Script", "EditorIcons")
