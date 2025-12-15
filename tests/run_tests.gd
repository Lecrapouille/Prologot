# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# Test runner script. Add this to a scene's root node to run all tests.
# Usage: godot --headless --script tests/run_tests.gd

extends SceneTree

func _init() -> void:
	# Load and run the test script
	var test_script := load("res://tests/test_prologot.gd")

	if test_script:
		var test_node := Node.new()
		test_node.set_script(test_script)
		root.add_child(test_node)

		# Wait for tests to complete
		await get_tree().create_timer(2.0).timeout
	else:
		print("ERROR: Could not load test script")

	quit()