# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Shared bootstrap for a Prologot handle (editor plugin, autoload, PrologotNode).
# SWI-Prolog is process-global: each call creates a handle that attach()es to
# the same engine. cleanup() on a handle does not call PL_cleanup().

extends RefCounted

## Bundled SWI home under res://bin/<os>/swipl, or empty if that folder is absent.
static func swipl_home() -> String:
	var os_map := {"Linux": "linux", "Windows": "windows", "macOS": "macos"}
	return "res://bin/" + os_map.get(OS.get_name(), OS.get_name().to_lower()) + "/swipl"


## Options for Prologot.initialize(). Only sets "home" when the path exists.
static func initialize_options(home: String = "") -> Dictionary:
	var options := {}
	var path := home if not home.is_empty() else swipl_home()
	if path.is_empty():
		return options
	if DirAccess.dir_exists_absolute(path) or \
			DirAccess.dir_exists_absolute(ProjectSettings.globalize_path(path)):
		options["home"] = path
	return options


## Instantiates Prologot and initialize()s it. Returns null on failure.
static func create_engine(home: String = ""):
	if not ClassDB.class_exists("Prologot"):
		push_error("Prologot: GDExtension not loaded. Make sure bin/prologot.gdextension exists in your project.")
		return null
	var engine = ClassDB.instantiate("Prologot")
	if engine == null:
		push_error("Prologot: ClassDB.instantiate(Prologot) failed")
		return null
	if not engine.initialize(initialize_options(home)):
		push_error("Prologot: Failed to initialize Prolog engine")
		return null
	return engine
