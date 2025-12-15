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

var dock: Control
var editor_engine = null  # Prologot instance for the editor


func _enter_tree() -> void:
	# Add the autoload singleton for game runtime
	add_autoload_singleton("PrologotEngine", "res://addons/prologot/prologot_singleton.gd")

	# Create Prologot instance for the editor dock
	if ClassDB.class_exists("Prologot"):
		editor_engine = ClassDB.instantiate("Prologot")
		if editor_engine.initialize():
			print("Prologot: Editor engine initialized")
		else:
			push_error("Prologot: Failed to initialize editor engine")
			editor_engine = null
	else:
		push_error("Prologot: GDExtension not loaded")

	# Add the editor dock and pass the engine reference
	dock = PrologotDock.new()
	dock.engine = editor_engine
	add_control_to_dock(DOCK_SLOT_RIGHT_BL, dock)

	print("Prologot: Plugin enabled")


func _exit_tree() -> void:
	# Cleanup editor engine
	if editor_engine:
		editor_engine.cleanup()
		editor_engine = null

	# Remove the dock
	if dock:
		remove_control_from_docks(dock)
		dock.queue_free()

	# Remove the autoload (for game runtime)
	remove_autoload_singleton("PrologotEngine")

	print("Prologot: Plugin disabled")


func _has_main_screen() -> bool:
	return false


func _get_plugin_name() -> String:
	return "Prologot"


func _get_plugin_icon() -> Texture2D:
	return get_editor_interface().get_base_control().get_theme_icon("Script", "EditorIcons")
