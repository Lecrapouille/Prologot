# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# Test runner script. Add this to a scene's root node to run all tests.
# Usage: godot --headless --path tests -s run_tests.gd

extends SceneTree

func _init() -> void:
	# Load and run the test script
	var test_script := load("res://test_prologot.gd")

	if test_script:
		var test_node := Node.new()
		test_node.set_script(test_script)
		root.add_child(test_node)

		# Connect to the tests_finished signal
		if test_node.has_signal("tests_finished"):
			test_node.tests_finished.connect(_on_tests_finished)

		# Fallback timeout (10 seconds) in case signal never fires
		var timeout_timer := create_timer(10.0)
		timeout_timer.timeout.connect(_on_timeout)
	else:
		print("ERROR: Could not load test script")
		quit(1)


func _on_tests_finished(exit_code: int) -> void:
	quit(exit_code)


func _on_timeout() -> void:
	push_error("ERROR: Tests timed out after 10 seconds!")
	quit(1)