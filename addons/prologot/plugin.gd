# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# This is the main editor plugin that registers the Prologot dock
# and autoload singleton.

@tool
extends EditorPlugin

## Preloaded reference to the Prologot dock script
const PrologotDock = preload("res://addons/prologot/prologot_dock.gd")

## Reference to the dock control instance
var dock: Control

## Prologot engine instance for the editor.
## This is separate from the runtime singleton - it's only used in the editor
## for the interactive console dock.
var editor_engine = null

###############################################################################
## Called when the plugin is enabled/loaded in the editor.
##
## Sets up the Prologot plugin by:
## 1. Registering the autoload singleton for runtime use
## 2. Creating a separate Prologot engine instance for the editor dock
## 3. Creating and adding the interactive console dock to the editor
###############################################################################
func _enter_tree() -> void:
	# Register the autoload singleton that will be available at runtime
	# This allows game scripts to access PrologotEngine singleton during gameplay
	add_autoload_singleton("PrologotEngine", "res://addons/prologot/prologot_singleton.gd")

	# Create a separate Prologot instance for the editor dock
	# This allows testing and debugging Prolog code in the editor without
	# affecting the runtime singleton
	if ClassDB.class_exists("Prologot"):
		editor_engine = ClassDB.instantiate("Prologot")
		if editor_engine.initialize():
			print("Prologot: Editor engine initialized")
		else:
			push_error("Prologot: Failed to initialize editor engine")
			editor_engine = null
	else:
		push_error("Prologot: GDExtension not loaded")

	# Create the dock control and pass it the editor engine reference
	dock = PrologotDock.new()
	dock.engine = editor_engine
	# Add the dock to the right-bottom dock slot in the editor
	add_control_to_dock(DOCK_SLOT_RIGHT_BL, dock)

	print("Prologot: Plugin enabled")


###############################################################################
## Called when the plugin is disabled/unloaded from the editor.
##
## Cleans up all plugin resources:
## 1. Shuts down the editor Prologot engine
## 2. Removes and frees the dock control
## 3. Removes the autoload singleton registration
###############################################################################
func _exit_tree() -> void:
	# Cleanup the editor engine and free its resources
	if editor_engine:
		editor_engine.cleanup()
		editor_engine = null

	# Remove the dock from the editor and free it
	if dock:
		remove_control_from_docks(dock)
		dock.queue_free()

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
