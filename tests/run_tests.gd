# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# Test runner script. Add this to a scene's root node to run all tests.
# Usage: godot --headless --path tests -s run_tests.gd

extends SceneTree


func _init() -> void:
	var test_script := load("res://test_prologot.gd")
	if test_script == null:
		_finish(1, "ERROR: Could not load test script")
		return

	var test_node := Node.new()
	test_node.set_script(test_script)
	# Connect before add_child: _ready() may run immediately and emit.
	if test_node.has_signal("tests_finished"):
		test_node.tests_finished.connect(_on_tests_finished)
	root.add_child(test_node)

	create_timer(90.0).timeout.connect(_on_timeout)


func _on_tests_finished(exit_code: int) -> void:
	_finish(exit_code, "")


func _on_timeout() -> void:
	_finish(1, "ERROR: Tests timed out after 90 seconds!")


func _finish(exit_code: int, message: String) -> void:
	if not message.is_empty():
		push_error(message)
	_write_ci_result(exit_code)
	quit(exit_code)


func _write_ci_result(exit_code: int) -> void:
	var path := ProjectSettings.globalize_path("res://") + "ci_result.txt"
	var file := FileAccess.open(path, FileAccess.WRITE)
	if file:
		file.store_string("PROLOGOT_CI_RESULT=%d\n" % exit_code)
		file.close()
	print("PROLOGOT_CI_RESULT=%d" % exit_code)
