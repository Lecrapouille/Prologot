# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# Unit tests for the Prologot extension.
# Run these tests by adding this script to a Node in a test scene.

extends Node

## Signal emitted when all tests are completed
signal tests_finished(exit_code: int)

## The Prologot engine instance for testing.
var prolog: Prologot

## Count of passed tests.
var tests_passed: int = 0

## Count of failed tests.
var tests_failed: int = 0

## Total tests run.
var tests_total: int = 0


func _ready() -> void:
	print("=".repeat(60))
	print("Prologot Unit Tests")
	print("=".repeat(60))

	run_all_tests()

	print("")
	print("=".repeat(60))
	print("Test Results: %d passed, %d failed, %d total" % [tests_passed, tests_failed, tests_total])
	print("=".repeat(60))

	# Determine exit code (0 = success, 1 = failure)
	var exit_code := 0 if tests_failed == 0 else 1

	if tests_failed > 0:
		push_error("Some tests failed!")
	else:
		print("All tests passed!")

	# Emit signal to notify test runner
	tests_finished.emit(exit_code)


## Run all test suites.
func run_all_tests() -> void:
	test_initialization()
	test_basic_queries()
	test_fact_management()
	test_rules()
	test_dynamic_assertions()
	test_complex_queries()
	test_type_conversion()
	test_euclidean_distance()
	test_tracking_with_distance()
	test_error_handling()
	test_object_solve_api()
	test_lists_atoms_and_variants()
	test_consult_file_standalone()
	test_prolog_term_factories()
	test_prolog_variable()
	test_prolog_predicate()
	test_named_variables_and_arity()
	test_prolog_goal_composition()
	test_prolog_solution()
	test_solve_prolog_goal()
	test_atom_versus_string()
	test_assert_fact_goal()
	test_prolog_object()
	test_editor_query()
	test_expose_godot_members()
	test_prolog_knowledge_and_node()

	# Demo examples tests
	test_demo_01_basic_queries()
	test_demo_02_facts_and_rules()
	test_demo_03_dynamic_assertions()
	test_demo_04_complex_queries()
	test_demo_05_pathfinding()
	test_demo_06_ai_behavior()


# =============================================================================
# Test Helpers
# =============================================================================

## Assert that a condition is true.
func assert_true(condition: bool, message: String) -> void:
	tests_total += 1
	if condition:
		tests_passed += 1
		print("  ✓ PASS: %s" % message)
	else:
		tests_failed += 1
		print("  ✗ FAIL: %s" % message)


## Assert that a condition is false.
func assert_false(condition: bool, message: String) -> void:
	assert_true(not condition, message)


## Assert that two values are equal.
func assert_equal(actual: Variant, expected: Variant, message: String) -> void:
	tests_total += 1
	if actual == expected:
		tests_passed += 1
		print("  ✓ PASS: %s" % message)
	else:
		tests_failed += 1
		print("  ✗ FAIL: %s (expected %s, got %s)" % [message, str(expected), str(actual)])


## Assert that a value is not null/empty.
func assert_not_empty(value: Variant, message: String) -> void:
	tests_total += 1
	var is_empty := false

	if value == null:
		is_empty = true
	elif value is String and value.is_empty():
		is_empty = true
	elif value is Array and value.is_empty():
		is_empty = true
	elif value is Dictionary and value.is_empty():
		is_empty = true

	if not is_empty:
		tests_passed += 1
		print("  ✓ PASS: %s" % message)
	else:
		tests_failed += 1
		print("  ✗ FAIL: %s (value is empty)" % message)


## Attach a Prologot handle. SWI-Prolog itself is process-global.
func setup_prolog() -> bool:
	prolog = Prologot.new()
	return prolog.initialize()


## Detach the handle. The last cleanup() resets the user knowledge base.
func teardown_prolog() -> void:
	if prolog:
		prolog.cleanup()
		prolog = null


## Build a PrologGoal from a functor name and arguments.
func goal(functor: String, args: Array = []) -> PrologGoal:
	return prolog.predicate(functor).callv(args)


# =============================================================================
# Test: Initialization
# =============================================================================

func test_initialization() -> void:
	print("\n[Test Suite: Initialization]")

	# Test 1: Create instance
	prolog = Prologot.new()
	assert_true(prolog != null, "Prologot instance created")

	# Test 2: Check not initialized before calling initialize()
	assert_false(prolog.is_initialized(), "Engine not initialized before initialize()")

	# Test 3: Initialize
	var init_result: bool = prolog.initialize()
	assert_true(init_result, "Engine initialization succeeds")

	# Test 4: Check initialized
	assert_true(prolog.is_initialized(), "Engine reports initialized")

	# Test 5: Double initialization should succeed (idempotent)
	var reinit_result: bool = prolog.initialize()
	assert_true(reinit_result, "Re-initialization succeeds (idempotent)")

	assert_true(prolog.consult_string("lifecycle_marker(1)."), "consult a marker fact")
	assert_true(prolog.solve(goal("lifecycle_marker", [1])).has_solution(), "marker is queryable")

	# Test 6: A second handle shares the process-global engine
	var other := Prologot.new()
	assert_false(other.is_initialized(), "Second instance starts uninitialized")
	assert_true(other.initialize(), "Second instance attaches to the running engine")
	assert_true(other.is_initialized(), "Second instance reports initialized")
	assert_true(
		other.solve(other.predicate("lifecycle_marker").call(1)).has_solution(),
		"Second instance sees the shared knowledge base"
	)

	# Test 7: cleanup() detaches this handle; last handle resets user predicates
	prolog.cleanup()
	assert_false(prolog.is_initialized(), "First handle is detached after cleanup()")
	assert_true(other.is_initialized(), "Second handle stays initialized")
	assert_true(
		other.solve(other.predicate("lifecycle_marker").call(1)).has_solution(),
		"Shared knowledge remains while another handle is live"
	)

	other.cleanup()
	assert_false(other.is_initialized(), "Last handle is detached after cleanup()")
	assert_true(prolog.initialize(), "Re-attach after last cleanup")
	assert_false(
		prolog.solve(goal("lifecycle_marker", [1])).has_solution(),
		"Last cleanup resets the user knowledge base"
	)

	prolog.cleanup()
	prolog = null


# =============================================================================
# Test: Basic Queries
# =============================================================================

func test_basic_queries() -> void:
	print("\n[Test Suite: Basic Queries]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Load test facts
	var load_result := prolog.consult_string("""
		animal(dog).
		animal(cat).
		animal(bird).
		color(red).
		color(blue).
	""")
	assert_true(load_result, "Load basic facts")

	assert_true(prolog.solve(goal("animal", ["dog"])).has_solution(), "Query animal(dog) succeeds")
	assert_false(prolog.solve(goal("animal", ["fish"])).has_solution(), "Query animal(fish) fails")

	var animal := prolog.variable()
	var animals := prolog.solve(goal("animal", [animal])).all()
	assert_true(animals.size() >= 3, "solve().all() returns multiple results")
	print("    Animals found: ", animals)

	var one_animal: Variant = prolog.solve(goal("animal", [animal])).first()
	assert_true(one_animal != null, "solve().first() returns a result")
	print("    First animal: ", one_animal)

	var no_result: Variant = prolog.solve(goal("animal", ["unicorn"])).first()
	assert_true(no_result == null, "solve().first() returns null when no solution")

	teardown_prolog()


# =============================================================================
# Test: Fact Management
# =============================================================================

func test_fact_management() -> void:
	print("\n[Test Suite: Fact Management]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Test consult_string
	var consult_result := prolog.consult_string("""
		likes(mary, food).
		likes(mary, wine).
	""")
	assert_true(consult_result, "consult_string succeeds")

	# Verify facts loaded
	assert_true(prolog.solve(goal("likes", ["mary", "food"])).has_solution(), "Fact likes(mary, food) exists")
	assert_true(prolog.solve(goal("likes", ["mary", "wine"])).has_solution(), "Fact likes(mary, wine) exists")

	teardown_prolog()


# =============================================================================
# Test: Rules
# =============================================================================

func test_rules() -> void:
	print("\n[Test Suite: Rules]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Load family relationships
	prolog.consult_string("""
		parent(tom, bob).
		parent(tom, liz).
		parent(bob, ann).
		parent(bob, pat).

		grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
		sibling(X, Y) :- parent(P, X), parent(P, Y), X \\= Y.
	""")

	# Test rule evaluation
	assert_true(prolog.solve(goal("grandparent", ["tom", "ann"])).has_solution(), "Grandparent rule works")
	assert_true(prolog.solve(goal("grandparent", ["tom", "pat"])).has_solution(), "Grandparent rule - second grandchild")
	assert_false(prolog.solve(goal("grandparent", ["bob", "ann"])).has_solution(), "Non-grandparent correctly fails")

	# Test sibling rule
	assert_true(prolog.solve(goal("sibling", ["bob", "liz"])).has_solution(), "Sibling rule works")
	assert_true(prolog.solve(goal("sibling", ["ann", "pat"])).has_solution(), "Sibling rule - second pair")

	teardown_prolog()


# =============================================================================
# Test: Dynamic Assertions
# =============================================================================

func test_dynamic_assertions() -> void:
	print("\n[Test Suite: Dynamic Assertions]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var assert_result := prolog.assert_fact(goal("score", ["player1", 100]))
	assert_true(assert_result, "assert_fact succeeds")

	assert_true(prolog.solve(goal("score", ["player1", 100])).has_solution(), "Asserted fact exists")

	var retract_result := prolog.retract_fact(goal("score", ["player1", 100]))
	assert_true(retract_result, "retract_fact succeeds")

	assert_false(prolog.solve(goal("score", ["player1", 100])).has_solution(), "Retracted fact no longer exists")

	prolog.assert_fact(goal("temp", ["a"]))
	prolog.assert_fact(goal("temp", ["b"]))
	prolog.assert_fact(goal("temp", ["c"]))

	assert_true(prolog.solve(goal("temp", [prolog.anonymous()])).has_solution(), "Multiple temp facts exist")

	prolog.retract_all(goal("temp", [prolog.anonymous()]))
	assert_false(prolog.solve(goal("temp", [prolog.anonymous()])).has_solution(), "All temp facts removed")

	teardown_prolog()


# =============================================================================
# Test: Complex Queries
# =============================================================================

func test_complex_queries() -> void:
	print("\n[Test Suite: Complex Queries]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Load game-like data
	prolog.consult_string("""
		enemy(goblin, 10, 5).
		enemy(orc, 25, 10).
		enemy(dragon, 100, 50).

		weak(Name) :- enemy(Name, HP, _), HP < 20.
		strong(Name) :- enemy(Name, HP, _), HP >= 50.
	""")

	# Test computed queries
	assert_true(prolog.solve(goal("weak", ["goblin"])).has_solution(), "Goblin is weak")
	assert_false(prolog.solve(goal("weak", ["dragon"])).has_solution(), "Dragon is not weak")
	assert_true(prolog.solve(goal("strong", ["dragon"])).has_solution(), "Dragon is strong")

	# Test solve().all() with complex results
	var weak_enemies := prolog.solve(goal("weak", [prolog.variable("X")])).all()
	assert_true(weak_enemies.size() >= 1, "At least one weak enemy found")

	teardown_prolog()


# =============================================================================
# Test: Type Conversion
# =============================================================================

func test_type_conversion() -> void:
	print("\n[Test Suite: Type Conversion]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	prolog.assert_fact(goal("number_test", [42]))
	assert_true(prolog.solve(goal("number_test", [42])).has_solution(), "Integer fact works")

	prolog.assert_fact(goal("negative_test", [-15]))
	assert_true(prolog.solve(goal("negative_test", [-15])).has_solution(), "Negative integer fact works")

	prolog.assert_fact(goal("float_test", [3.14]))
	assert_true(prolog.solve(goal("float_test", [3.14])).has_solution(), "Float fact works")

	prolog.assert_fact(goal("coords", [10, 20, 30]))
	assert_true(prolog.solve(goal("coords", [10, 20, 30])).has_solution(), "Multiple numeric arguments work")

	prolog.assert_fact(goal("player_data", ["alice", 100, 25.5]))
	assert_true(prolog.solve(goal("player_data", ["alice", 100, 25.5])).has_solution(), "Mixed atom and numeric arguments work")

	prolog.assert_fact(goal("large_number", [999999]))
	assert_true(prolog.solve(goal("large_number", [999999])).has_solution(), "Large number fact works")

	prolog.assert_fact(goal("zero_test", [0]))
	assert_true(prolog.solve(goal("zero_test", [0])).has_solution(), "Zero value fact works")

	prolog.assert_fact(goal("name_test", ["hello"]))
	assert_true(prolog.solve(goal("name_test", ["hello"])).has_solution(), "Atom fact works")

	prolog.consult_string("""
		add(X, Y, Z) :- Z is X + Y.
	""")

	var sum_var := prolog.variable("Z")
	var sum_sol: PrologSolution = prolog.solve(goal("add", [10, 20, sum_var])).first()
	assert_equal(sum_sol.get(sum_var), 30, "add/3 binds the result (10 + 20 = 30)")

	teardown_prolog()


# =============================================================================
# Test: Euclidean Distance Calculation
# =============================================================================

func test_euclidean_distance() -> void:
	print("\n[Test Suite: Euclidean Distance]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Load position facts and distance rule
	prolog.consult_string("""
		% Position facts: position(Location, X, Y, Z)
		position(zone_1, 0, 0, 0).
		position(zone_2, 3, 4, 0).
		position(zone_3, 1, 1, 1).
		position(origin, 0, 0, 0).
		position(point_a, 10, 0, 0).

		% Euclidean distance calculation
		distance(Loc1, Loc2, Distance) :-
			position(Loc1, X1, Y1, Z1),
			position(Loc2, X2, Y2, Z2),
			DX is X2 - X1,
			DY is Y2 - Y1,
			DZ is Z2 - Z1,
			Distance is sqrt(DX*DX + DY*DY + DZ*DZ).
	""")

	var distance_d := prolog.variable("D")
	var result1: PrologSolution = prolog.solve(goal("distance", ["zone_1", "zone_2", distance_d])).first()
	assert_true(result1 != null, "Distance query returns a result")
	assert_true(abs(result1.get(distance_d) - 5.0) < 0.001, "Distance zone_1 to zone_2 is ~5.0")

	var exact_match := prolog.solve(goal("distance", ["zone_1", "zone_2", 5])).has_solution()
	print("    Exact match (distance = 5): ", exact_match, " (may be false due to float precision)")

	assert_true(result1.get(distance_d) >= 4.99 and result1.get(distance_d) <= 5.01, "Distance within tolerance range [4.99, 5.01]")

	var result2: PrologSolution = prolog.solve(goal("distance", ["origin", "point_a", distance_d])).first()
	assert_true(result2 != null, "Distance origin to point_a calculated")

	var result3: PrologSolution = prolog.solve(goal("distance", ["zone_1", "zone_1", distance_d])).first()
	assert_true(abs(result3.get(distance_d)) < 0.001, "Distance from point to itself is 0")

	var result4: Variant = prolog.solve(goal("distance", ["zone_1", "zone_3", distance_d])).first()
	assert_true(result4 != null, "3D distance calculated")

	teardown_prolog()


# =============================================================================
# Test: Tracking with Distance
# =============================================================================

func test_tracking_with_distance() -> void:
	print("\n[Test Suite: Tracking with Distance]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Load the can_track rule that uses distance
	prolog.consult_string("""
		% Tracking rule: can_track if distance < 5
		can_track(Alien, Target) :-
			at(Alien, AlienLoc),
			at(Target, TargetLoc),
			distance(AlienLoc, TargetLoc, Dist),
			Dist < 5.
	""")

	assert_true(prolog.assert_fact(goal("at", ["alien_1", "zone_1"])), "Add fact: at(alien_1, zone_1)")
	assert_true(prolog.assert_fact(goal("at", ["guard_1", "zone_2"])), "Add fact: at(guard_1, zone_2)")
	assert_true(prolog.assert_fact(goal("distance", ["zone_1", "zone_2", 3])), "Add fact: distance(zone_1, zone_2, 3)")

	# Test 1: can_track should succeed (distance 3 < 5)
	var can_track_result := prolog.solve(goal("can_track", ["alien_1", "guard_1"])).has_solution()
	assert_true(can_track_result, "can_track(alien_1, guard_1) succeeds (distance 3 < 5)")

	# Test 2: Add a target too far away
	assert_true(prolog.assert_fact(goal("at", ["guard_2", "zone_3"])), "Add fact: at(guard_2, zone_3)")
	assert_true(prolog.assert_fact(goal("distance", ["zone_1", "zone_3", 10])), "Add fact: distance(zone_1, zone_3, 10)")

	# Test 3: can_track should fail (distance 10 >= 5)
	var cannot_track_result := prolog.solve(goal("can_track", ["alien_1", "guard_2"])).has_solution()
	assert_false(cannot_track_result, "can_track(alien_1, guard_2) fails (distance 10 >= 5)")

	# Test 4: Add another alien and guard at same location (distance 0 < 5)
	assert_true(prolog.assert_fact(goal("at", ["alien_2", "zone_1"])), "Add fact: at(alien_2, zone_1)")
	assert_true(prolog.assert_fact(goal("at", ["guard_3", "zone_1"])), "Add fact: at(guard_3, zone_1)")
	assert_true(prolog.assert_fact(goal("distance", ["zone_1", "zone_1", 0])), "Add fact: distance(zone_1, zone_1, 0)")

	var same_location_result := prolog.solve(goal("can_track", ["alien_2", "guard_3"])).has_solution()
	assert_true(same_location_result, "can_track(alien_2, guard_3) succeeds (same location, distance 0 < 5)")

	# Test 5: Edge case - distance exactly 5 should fail (< 5, not <= 5)
	assert_true(prolog.assert_fact(goal("at", ["guard_4", "zone_4"])), "Add fact: at(guard_4, zone_4)")
	assert_true(prolog.assert_fact(goal("distance", ["zone_1", "zone_4", 5])), "Add fact: distance(zone_1, zone_4, 5)")

	var exact_boundary_result := prolog.solve(goal("can_track", ["alien_1", "guard_4"])).has_solution()
	assert_false(exact_boundary_result, "can_track(alien_1, guard_4) fails (distance 5 is not < 5)")

	# Test 6: Verify facts exist
	assert_true(prolog.solve(goal("at", ["alien_1", "zone_1"])).has_solution(), "Fact at(alien_1, zone_1) exists")
	assert_true(prolog.solve(goal("distance", ["zone_1", "zone_2", 3])).has_solution(), "Fact distance(zone_1, zone_2, 3) exists")

	teardown_prolog()


# =============================================================================
# Test: Error Handling
# =============================================================================

func test_error_handling() -> void:
	print("\n[Test Suite: Error Handling]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Editor console parser: syntax errors must not crash
	var bad_query: Array = prolog._editor_query("this is not valid prolog")
	assert_true(bad_query.is_empty(), "Invalid query returns no solutions")

	var empty_query: Array = prolog._editor_query("")
	assert_true(empty_query.is_empty(), "Empty query returns no solutions")

	# Test retract non-existent fact
	var retract_missing := prolog.retract_fact(goal("nonexistent_fact", ["x"]))
	assert_false(retract_missing, "Retracting non-existent fact returns false")

	# Test consult_string with syntax error - should fail gracefully
	var bad_code := prolog.consult_string("invalid prolog code here")
	assert_false(bad_code, "Invalid code returns false")
	assert_true(prolog.get_last_error().length() > 0, "get_last_error returns error message")
	print("    Last error: ", prolog.get_last_error())

	teardown_prolog()


# =============================================================================
# Test: public object solve API (no string / Dictionary goals)
# =============================================================================

func test_object_solve_api() -> void:
	print("\n[Test Suite: object solve API]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	assert_true(prolog.consult_string("""
		parent(tom, bob).
		parent(tom, liz).
		parent(bob, ann).
	"""), "Load family facts")

	var parent := prolog.predicate("parent")
	assert_true(prolog.solve(parent.call("tom", "bob")).has_solution(), "has_solution parent(tom, bob)")
	assert_false(prolog.solve(parent.call("bob", "tom")).has_solution(), "has_solution parent(bob, tom) fails")

	var found := prolog.solve(parent.call("tom", "bob"))
	assert_equal(found.all().size(), 1, "solve returns one PrologSolution for a ground fact")
	assert_true(prolog.solve(parent.call("bob", "tom")).has_solution() == false, "solve is empty when the goal fails")

	var child := prolog.variable("Child")
	var children := prolog.solve(parent.call("tom", child)).all()
	assert_equal(children.size(), 2, "solve().all() returns both children of tom")
	var names := []
	for solution in children:
		assert_true(solution.has(child), "each solution binds the variable object")
		names.append(solution.get(child))
	assert_true("bob" in names and "liz" in names, "solve().all() binds the child variable")

	var first_child: PrologSolution = prolog.solve(parent.call("tom", child)).first()
	assert_true(first_child != null and first_child.has(child), "solve().first() returns a PrologSolution")

	var via := prolog.variable()
	var grandchild := prolog.variable()
	assert_true(
		prolog.solve(parent.call("tom", via).conjunction(parent.call(via, grandchild))).has_solution(),
		"conjunction of two parent goals succeeds"
	)

	# A String passed to call() is always an atom, never a variable
	assert_true(prolog.solve(parent.call("tom", "bob")).has_solution(), "lowercase strings are atoms")
	assert_true(prolog.solve(parent.call("tom", "X")).has_solution() == false, "uppercase string 'X' is an atom, not a variable")

	teardown_prolog()


# =============================================================================
# Test: Lists, atoms vs strings, compound Variants
# =============================================================================

func test_lists_atoms_and_variants() -> void:
	print("\n[Test Suite: Lists, atoms, Variant compounds]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	assert_true(prolog.consult_string("""
		nums([1, 2, 3]).
		nested([[a, b], [c]]).
		empty_list([]).
		named(hello).
	"""), "Load list and atom facts")

	var list_var := prolog.variable("L")
	var nums: PrologSolution = prolog.solve(goal("nums", [list_var])).first()
	assert_true(nums != null, "nums/1 returns a solution")
	assert_true(nums.get(list_var) is Array, "Prolog list becomes a Godot Array")
	assert_equal(nums.get(list_var), [1, 2, 3], "List [1, 2, 3] round-trips")

	var empty: PrologSolution = prolog.solve(goal("empty_list", [list_var])).first()
	assert_equal(empty.get(list_var), [], "Empty Prolog list becomes []")

	var nested: PrologSolution = prolog.solve(goal("nested", [list_var])).first()
	assert_equal(nested.get(list_var), [["a", "b"], ["c"]], "Nested lists convert recursively")

	# GDScript strings become Prolog atoms (not Prolog strings)
	assert_true(prolog.solve(goal("named", ["hello"])).has_solution(), "Atom hello matches string 'hello'")
	assert_false(prolog.solve(goal("named", ["goodbye"])).has_solution(), "Missing atom fact fails")
	assert_true(
		prolog.solve(goal("named", [prolog.atom("hello")])).has_solution(),
		"prolog.atom() matches the same atom as a String"
	)

	assert_true(prolog.solve(goal("member", [2, [1, 2, 3]])).has_solution(), "member/2 accepts a Godot Array")
	assert_false(prolog.solve(goal("member", [9, [1, 2, 3]])).has_solution(), "member/2 fails for a missing element")

	var list_n := prolog.variable("N")
	var list_sol: PrologSolution = prolog.solve(goal("length", [[1, 2, 3, 4], list_n])).first()
	assert_equal(list_sol.get(list_n), 4, "length/2 binds the list length")

	assert_true(prolog.consult_string("""
		nest(0, leaf).
		nest(N, [T]) :- integer(N), N > 0, M is N - 1, nest(M, T).
		cyclic(L) :- L = [a|L].
	"""), "Load nest/2 and cyclic/1")

	var nest_var := prolog.variable("T")
	var nest8: PrologSolution = prolog.solve(goal("nest", [8, nest_var])).first()
	var walked: Variant = nest8.get(nest_var)
	for _i in 8:
		assert_true(walked is Array and walked.size() == 1, "depth-8 nest is a chain of singleton arrays")
		walked = walked[0]
	assert_equal(walked, "leaf", "depth-8 nest reaches leaf")

	var cycle: PrologSolution = prolog.solve(goal("cyclic", [list_var])).first()
	assert_true(cycle != null, "cyclic/1 succeeds in Prolog")
	assert_true(cycle.get(list_var) == null, "cyclic list does not convert (would not terminate)")

	teardown_prolog()


# =============================================================================
# Test: consult_file outside the demo suites
# =============================================================================

func test_consult_file_standalone() -> void:
	print("\n[Test Suite: consult_file]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var loaded := prolog.consult_file("res://fixtures/animals.pl")
	assert_true(loaded, "consult_file loads tests/fixtures/animals.pl")
	assert_true(prolog.solve(goal("animal", ["dog"])).has_solution(), "Fact from consulted file is available")
	assert_true(prolog.solve(goal("animal", ["cat"])).has_solution(), "Second fact from consulted file is available")
	assert_false(prolog.solve(goal("animal", ["fish"])).has_solution(), "Missing fact from consulted file fails")

	var missing := prolog.consult_file("res://fixtures/does_not_exist.pl")
	assert_false(missing, "consult_file of a missing file returns false")
	assert_true(prolog.get_last_error().length() > 0, "Missing file sets get_last_error()")

	teardown_prolog()


func test_prolog_term_factories() -> void:
	print("\n[Test Suite: PrologTerm factories]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var tom := prolog.atom("tom")
	assert_true(tom != null and tom.is_atom(), "atom() creates an atom term")
	assert_equal(tom.get_kind(), "atom", "atom kind name")
	assert_equal(tom.get_atom(), "tom", "atom value")
	assert_equal(tom.as_text(), "tom", "atom as_text")

	var n := prolog.integer(42)
	assert_true(n.is_integer(), "integer() creates an integer term")
	assert_equal(n.get_integer(), 42, "integer value")

	var pi := prolog.real(3.14)
	assert_true(pi.is_float(), "real() creates a float term")
	assert_true(abs(pi.get_real() - 3.14) < 0.0001, "real value")

	var hello := prolog.string("hello")
	assert_true(hello.is_string(), "string() creates a Prolog string term")
	assert_equal(hello.get_string(), "hello", "string value")
	assert_equal(hello.as_text(), "\"hello\"", "string as_text quotes the value")

	var empty := prolog.nil()
	assert_true(empty.is_nil(), "nil() creates []")
	assert_equal(empty.as_text(), "[]", "nil as_text")

	var items := prolog.list([1, "bob"])
	assert_true(items.is_list(), "list() creates a list term")
	assert_equal(items.get_args().size(), 2, "list arity")
	assert_equal(prolog.list([]).is_nil(), true, "empty list() is nil")

	var parent := prolog.compound("parent", ["tom", "bob"])
	assert_true(parent.is_compound(), "compound() creates a compound term")
	assert_equal(parent.get_functor(), "parent", "compound functor")
	assert_equal(parent.get_args(), ["tom", "bob"], "compound args")
	assert_equal(parent.as_text(), "parent(tom, bob)", "compound as_text")

	teardown_prolog()


func test_prolog_variable() -> void:
	print("\n[Test Suite: PrologVariable]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var child := prolog.variable()
	var named := prolog.variable("Child")
	var other := prolog.variable()
	var anon := prolog.anonymous()

	assert_true(child != null and child.is_variable(), "variable() creates a PrologVariable")
	assert_true(named.get_name() == "Child", "named variable keeps its debug name")
	assert_true(child.get_id() != other.get_id(), "each variable() call has a distinct identity")
	assert_true(child.get_id() == child.get_id(), "the same object keeps a stable id")
	assert_true(anon.is_anonymous(), "anonymous() is anonymous")
	assert_false(named.is_anonymous(), "a named variable is not anonymous")
	assert_true(child != named, "different variable objects are not equal")

	teardown_prolog()


func test_prolog_predicate() -> void:
	print("\n[Test Suite: PrologPredicate]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var parent := prolog.predicate("parent")
	assert_true(parent != null, "predicate() creates a PrologPredicate")
	assert_equal(parent.get_name(), "parent", "predicate name")
	assert_equal(parent.as_text(), "parent", "predicate debug text is the functor")

	var child := prolog.variable("Child")
	var parent_goal: PrologGoal = parent.call("tom", child)
	assert_true(parent_goal != null, "call() returns a goal")
	assert_equal(parent_goal.get_functor(), "parent", "goal functor")
	assert_equal(parent_goal.get_arity(), 2, "arity comes from call() arguments")
	assert_true(parent_goal.get_args()[1] == child, "call() keeps the variable object")

	var unary: PrologGoal = parent.call("tom")
	assert_true(unary != null and unary.get_arity() == 1, "same predicate object can build parent/1")

	var via_array: PrologGoal = parent.callv(["tom", child])
	assert_true(via_array != null and via_array.get_functor() == "parent", "callv() accepts an Array")

	teardown_prolog()


func test_named_variables_and_arity() -> void:
	print("\n[Test Suite: named variables and multi-arg call]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	assert_true(prolog.consult_string("""
		foo(tom, a, john, b).
		foo(tom, x, john, y).
		route(a, b, c, d, e, f).
	"""), "Load multi-argument facts")

	var a := prolog.variable("A")
	var b := prolog.variable("B")
	var other_a := prolog.variable("A")
	assert_true(a.get_id() != other_a.get_id(), "two variable(\"A\") objects stay distinct")

	var foo := prolog.predicate("foo")
	var names := []
	for solution in prolog.solve(foo.call("tom", a, "john", b)):
		assert_true(solution.has(a) and solution.has(b), "solution is keyed by variable objects")
		assert_false(solution.has(other_a), "a different variable(\"A\") is not in the solution")
		names.append([solution.get(a), solution.get(b)])
	assert_equal(names.size(), 2, "foo/4 yields two solutions")
	assert_true(["a", "b"] in names and ["x", "y"] in names, "named variables bind positionally")

	var v1 := prolog.variable("P1")
	var v2 := prolog.variable("P2")
	var v3 := prolog.variable("P3")
	var v4 := prolog.variable("P4")
	var v5 := prolog.variable("P5")
	var v6 := prolog.variable("P6")
	var six: PrologSolution = prolog.solve(
		prolog.predicate("route").call(v1, v2, v3, v4, v5, v6)
	).first()
	assert_true(six != null, "route/6 has a solution")
	assert_equal([six.get(v1), six.get(v2), six.get(v3), six.get(v4), six.get(v5), six.get(v6)],
		["a", "b", "c", "d", "e", "f"], "six named variables bind in order")

	var animal := prolog.predicate("animal")
	prolog.assert_fact(animal.call("dog"))
	prolog.assert_fact(animal.call("cat", "black"))
	assert_true(prolog.solve(animal.call("dog")).has_solution(), "animal/1 from a shared predicate object")
	assert_true(prolog.solve(animal.call("cat", "black")).has_solution(), "animal/2 from the same object")
	assert_false(prolog.solve(animal.call("dog", "black")).has_solution(), "animal/2 does not match animal/1")

	teardown_prolog()


func test_prolog_goal_composition() -> void:
	print("\n[Test Suite: PrologGoal composition]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var parent := prolog.predicate("parent")
	var child := prolog.variable("Child")
	var grand := prolog.variable("Grand")
	var left: PrologGoal = parent.call("tom", child)
	var right: PrologGoal = parent.call(child, grand)

	var both := left.conjunction(right)
	assert_true(both != null, "conjunction() builds a goal")
	assert_equal(both.get_functor(), ",", "conjunction uses ','/2")
	assert_equal(both.get_arity(), 2, "conjunction has two goals")

	var either := left.disjunction(right)
	assert_equal(either.get_functor(), ";", "disjunction uses ';'/2")

	var not_goal := left.negated()
	assert_equal(not_goal.get_functor(), "\\+", "negated uses \\+/1")
	assert_equal(not_goal.get_arity(), 1, "negated wraps one goal")

	assert_true(prolog.consult_string("""
		parent(tom, bob).
		parent(bob, ann).
		animal(dog).
		animal(cat).
	"""), "Load composition facts")

	var animal := prolog.predicate("animal")

	assert_true(
		prolog.solve(parent.call("tom", child).conjunction(parent.call(child, grand))).has_solution(),
		"conjunction finds tom -> bob -> ann"
	)
	assert_true(
		prolog.solve(animal.call("dog").disjunction(animal.call("unicorn"))).has_solution(),
		"disjunction succeeds if either goal succeeds"
	)
	assert_true(
		prolog.solve(animal.call("unicorn").negated()).has_solution(),
		"negated succeeds when the goal fails"
	)
	assert_false(
		prolog.solve(animal.call("dog").negated()).has_solution(),
		"negated fails when the goal succeeds"
	)

	teardown_prolog()


func test_prolog_solution() -> void:
	print("\n[Test Suite: PrologSolution]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var child := prolog.variable("Child")
	var unused := prolog.variable("Unused")
	var solution: PrologSolution = ClassDB.instantiate("PrologSolution")
	solution.put(child, "bob")

	assert_true(solution.has(child), "has() is true for a bound variable")
	assert_false(solution.has(unused), "has() is false for an unbound variable")
	assert_equal(solution.get(child), "bob", "get() returns the bound value")
	assert_true(solution.get(unused) == null, "get() on a missing variable is null")
	assert_true(solution.get_bindings().has(child), "bindings is keyed by the variable object")
	assert_equal(solution.get_bindings()[child], "bob", "bindings[variable] returns the value")
	assert_equal(solution.values(), ["bob"], "values() lists bound values")

	teardown_prolog()


func test_solve_prolog_goal() -> void:
	print("\n[Test Suite: solve(PrologGoal)]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	assert_true(prolog.consult_string("""
		parent(tom, bob).
		parent(tom, liz).
		parent(bob, ann).
	"""), "Load family facts")

	var parent := prolog.predicate("parent")
	var child := prolog.variable("Child")

	assert_true(prolog.solve(parent.call("tom", "bob")).has_solution(), "has_solution parent(tom, bob)")
	assert_false(prolog.solve(parent.call("bob", "tom")).has_solution(), "has_solution fails for missing fact")
	assert_equal(prolog.solve(parent.call("tom", "bob")).all().size(), 1, "solve(goal) returns solutions")
	assert_true(prolog.solve(parent.call("bob", "tom")).has_solution() == false, "solve(goal) is empty when there is no solution")

	var solutions := prolog.solve(parent.call("tom", child)).all()
	assert_equal(solutions.size(), 2, "solve().all() returns two children")
	var names := []
	for solution in solutions:
		assert_true(solution.has(child), "each solution binds the variable object")
		names.append(solution.get(child))
	assert_true("bob" in names and "liz" in names, "bindings are bob and liz")

	var first: PrologSolution = prolog.solve(parent.call("tom", child)).first()
	assert_true(first != null and first.has(child), "solve().first() returns a PrologSolution")

	var none: Variant = prolog.solve(parent.call("ann", child)).first()
	assert_true(none == null, "solve().first() is null when there is no solution")

	var grandchild := prolog.variable("Grand")
	var chain: PrologGoal = parent.call("tom", child).conjunction(parent.call(child, grandchild))
	var chained := prolog.solve(chain).all()
	assert_equal(chained.size(), 1, "conjunction finds tom -> bob -> ann")
	assert_equal(chained[0].get(child), "bob", "shared variable stays bound across the conjunction")
	assert_equal(chained[0].get(grandchild), "ann", "second variable is bound")

	var n := prolog.variable("N")
	var between := prolog.predicate("between")
	var first_n: PrologSolution = prolog.solve(between.call(1, 1000000, n)).first()
	assert_true(first_n != null, "first() on a huge domain returns")
	assert_equal(first_n.get(n), 1, "first() pulls only the first between/3 answer")

	var seen := 0
	for sol in prolog.solve(between.call(1, 1000000, n)):
		seen += 1
		if seen == 3:
			break
	assert_equal(seen, 3, "break stops after three pulls")

	var capped := prolog.solve(between.call(1, 1000000, n), 5).all()
	assert_equal(capped.size(), 5, "solve(goal, 5) caps all()")
	assert_equal(capped[0].get(n), 1, "capped first value is 1")
	assert_equal(capped[4].get(n), 5, "capped last value is 5")

	teardown_prolog()


func test_atom_versus_string() -> void:
	print("\n[Test Suite: atom vs Prolog string]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	assert_true(prolog.consult_string("""
		named(hello).
		msg("hello").
	"""), "Load atom and string facts")

	assert_true(prolog.solve(goal("named", ["hello"])).has_solution(), "String call() is an atom")
	assert_false(
		prolog.solve(goal("named", [prolog.string("hello")])).has_solution(),
		"prolog.string() does not match an atom"
	)
	assert_true(
		prolog.solve(goal("msg", [prolog.string("hello")])).has_solution(),
		"prolog.string() matches a Prolog string fact"
	)
	assert_false(prolog.solve(goal("msg", ["hello"])).has_solution(), "atom hello does not match \"hello\"")

	var value := prolog.variable()
	var atom_sol: PrologSolution = prolog.solve(goal("named", [value])).first()
	assert_equal(atom_sol.get(value), "hello", "atom comes back as a Godot String")

	var str_sol: PrologSolution = prolog.solve(goal("msg", [value])).first()
	var bound = str_sol.get(value)
	assert_true(bound is PrologTerm and bound.is_string(), "Prolog string comes back as PrologTerm")
	assert_equal(bound.get_string(), "hello", "Prolog string contents")

	teardown_prolog()


func test_assert_fact_goal() -> void:
	print("\n[Test Suite: assert_fact / retract on PrologGoal]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var score := prolog.predicate("score")
	assert_true(prolog.assert_fact(score.call("p1", 10)), "assert_fact adds a goal")
	assert_true(prolog.solve(score.call("p1", 10)).has_solution(), "asserted goal is queryable")
	assert_true(prolog.retract_fact(score.call("p1", 10)), "retract_fact accepts a goal")
	assert_false(prolog.solve(score.call("p1", 10)).has_solution(), "retracted goal is gone")

	prolog.assert_fact(score.call("a", 1))
	prolog.assert_fact(score.call("b", 2))
	assert_true(prolog.retract_all(score.call(prolog.anonymous(), prolog.anonymous())), "retract_all(goal)")
	assert_false(prolog.solve(score.call(prolog.anonymous(), prolog.anonymous())).has_solution(), "all score facts removed")

	teardown_prolog()


func test_prolog_object() -> void:
	print("\n[Test Suite: PrologObject]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var node := Node.new()
	node.name = "Player"
	var handle: PrologObject = prolog.object(node)
	assert_true(handle != null and handle.is_valid(), "object() wraps a live Node")
	assert_equal(handle.get_object(), node, "get_object() returns the same Node")
	assert_true(handle.equals(prolog.object(node)), "same instance id compares equal")

	var at := prolog.predicate("at")
	assert_true(prolog.assert_fact(at.call(handle, "zone_1")), "assert a fact with a PrologObject")
	assert_true(prolog.solve(at.call(handle, "zone_1")).has_solution(), "query with the same handle")
	assert_true(prolog.solve(at.call(node, "zone_1")).has_solution(), "call() auto-wraps a Node")

	var place := prolog.variable()
	var solution: PrologSolution = prolog.solve(at.call(handle, place)).first()
	assert_equal(solution.get(place), "zone_1", "object fact binds other arguments")

	var who := prolog.variable()
	var found: PrologSolution = prolog.solve(at.call(who, "zone_1")).first()
	var bound_obj = found.get(who)
	assert_true(bound_obj is PrologObject, "blob comes back as PrologObject")
	assert_true(bound_obj.equals(handle), "round-trip keeps the instance id")

	node.free()
	assert_false(handle.is_valid(), "handle is invalid after the Node is freed")

	teardown_prolog()


func test_editor_query() -> void:
	print("\n[Test Suite: editor console bindings]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	assert_true(prolog.consult_string("""
		parent(tom, bob).
		parent(tom, liz).
	"""), "Load family facts")

	var rows: Array = prolog._editor_query("parent(tom, X)")
	assert_equal(rows.size(), 2, "named query returns both children")
	var names := []
	for row in rows:
		names.append(row["X"])
	assert_true("bob" in names and "liz" in names, "bindings use source variable names")

	var ground: Array = prolog._editor_query("parent(tom, bob)")
	assert_equal(ground.size(), 1, "ground success is one empty binding set")
	if ground.size() > 0:
		assert_true(ground[0].is_empty(), "ground success has no variables")

	var bad: Array = prolog._editor_query("this is not valid")
	assert_true(bad.is_empty(), "parse error returns no solutions")
	assert_true(prolog.get_last_error().length() > 0, "parse error is stored")

	teardown_prolog()


func test_expose_godot_members() -> void:
	print("\n[Test Suite: expose_property / expose_method]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var node := Node.new()
	node.name = "Player"

	assert_true(prolog.expose_property("Node", "name", "node_name"), "expose Node.name as node_name/2")
	var name_var := prolog.variable()
	var named: PrologSolution = prolog.solve(prolog.predicate("node_name").call(node, name_var)).first()
	assert_true(named != null, "node_name/2 returns a solution")
	if named != null:
		assert_equal(named.get(name_var), "Player", "reads Node.name")
	assert_true(prolog.solve(prolog.predicate("node_name").call(node, "Player")).has_solution(), "ground name matches")
	assert_false(prolog.solve(prolog.predicate("node_name").call(node, "Enemy")).has_solution(), "ground name mismatch fails")

	assert_true(prolog.expose_method("Object", "get_class", "godot_class"), "expose Object.get_class")
	var class_var := prolog.variable()
	var typed: PrologSolution = prolog.solve(prolog.predicate("godot_class").call(node, class_var)).first()
	assert_true(typed != null, "godot_class/2 returns a solution")
	assert_equal(typed.get(class_var), "Node", "get_class is Node")

	var sprite := Node2D.new()
	sprite.position = Vector2(3, 4)
	assert_true(prolog.expose_property("Node2D", "position", "node2d_position"), "expose Node2D.position")
	var pos_var := prolog.variable()
	var placed: PrologSolution = prolog.solve(prolog.predicate("node2d_position").call(sprite, pos_var)).first()
	assert_true(placed != null, "node2d_position/2 returns a solution")
	var pos: Variant = placed.get(pos_var)
	assert_true(pos is Array and pos.size() == 2, "Vector2 becomes a 2-element list")
	assert_true(is_equal_approx(float(pos[0]), 3.0) and is_equal_approx(float(pos[1]), 4.0), "position is [3, 4]")
	assert_false(
		prolog.solve(prolog.predicate("node2d_position").call(node, prolog.anonymous())).has_solution(),
		"Node is rejected by the Node2D class filter"
	)

	var pack := Resource.new()
	pack.resource_name = "loot"
	assert_true(prolog.expose_property("Resource", "resource_name", "res_name"), "expose Resource.resource_name")
	assert_true(prolog.solve(prolog.predicate("res_name").call(pack, "loot")).has_solution(), "reads a Resource property")

	assert_false(prolog.expose_property("Node", "name", "name"), "refuse reserved predicate name/2")
	assert_false(prolog.expose_property("Node", "name", "NodeName"), "refuse non-lowercase functor")

	var exposed: Array = prolog.list_exposed()
	assert_true(exposed.size() >= 4, "list_exposed() lists the wrappers")
	assert_true(prolog.unexpose("node_name", 2), "unexpose node_name/2")
	assert_false(prolog.solve(prolog.predicate("node_name").call(node, "Player")).has_solution(), "wrapper is gone")

	node.free()
	sprite.free()
	teardown_prolog()


func test_prolog_knowledge_and_node() -> void:
	print("\n[Test Suite: PrologKnowledge / PrologotNode]")

	var kb_script = load("res://addons/prologot/prolog_knowledge.gd")
	var node_script = load("res://addons/prologot/prologot_node.gd")
	if kb_script == null or node_script == null:
		print("  ✗ SKIP: addons/prologot not linked into the tests project")
		return

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var kb = kb_script.new()
	kb.code = "parent(tom, bob)."
	assert_true(kb.load_into(prolog), "PrologKnowledge.load_into() consults inline code")
	assert_true(prolog.solve(prolog.predicate("parent").call("tom", "bob")).has_solution(), "resource clauses are queryable")

	var host = node_script.new()
	host.auto_start = false
	host.set_engine(prolog)
	var extra = kb_script.new()
	extra.code = "parent(bob, ann)."
	host.knowledge = extra
	assert_true(host.start(), "PrologotNode.start() loads its knowledge")
	assert_true(host.solve(host.predicate("parent").call("bob", "ann")).has_solution(), "node forwards solve API")

	host.free()
	teardown_prolog()


# =============================================================================
# Demo Examples Tests
# =============================================================================
# These tests validate the Prolog examples in demos/showcases/examples/
# to ensure they work correctly and serve as integration tests.

## Path to demo examples folder.
const DEMOS_PATH := "res://../demos/showcases/examples/"


## Test Demo 01: Basic Queries - Family Relationships
func test_demo_01_basic_queries() -> void:
	print("\n[Test Suite: Demo 01 - Basic Queries]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Load the demo file
	var consult_result := prolog.consult_file(DEMOS_PATH + "01_basic_queries.pl")
	if not consult_result:
		print("  ✗ SKIP: Could not load 01_basic_queries.pl (file not found?)")
		print("    Error: ", prolog.get_last_error())
		teardown_prolog()
		return
	assert_true(consult_result, "Load 01_basic_queries.pl")

	# Test parent facts
	assert_true(prolog.solve(goal("parent", ["tom", "bob"])).has_solution(), "parent(tom, bob) exists")
	assert_true(prolog.solve(goal("parent", ["tom", "liz"])).has_solution(), "parent(tom, liz) exists")
	assert_true(prolog.solve(goal("parent", ["bob", "ann"])).has_solution(), "parent(bob, ann) exists")
	assert_true(prolog.solve(goal("parent", ["bob", "pat"])).has_solution(), "parent(bob, pat) exists")
	assert_true(prolog.solve(goal("parent", ["pat", "jim"])).has_solution(), "parent(pat, jim) exists")

	# Test non-existing relationships
	assert_false(prolog.solve(goal("parent", ["bob", "tom"])).has_solution(), "parent(bob, tom) should not exist")
	assert_false(prolog.solve(goal("parent", ["jim", "pat"])).has_solution(), "parent(jim, pat) should not exist")

	# Query all children of tom
	var tom_children := prolog.solve(goal("parent", ["tom", prolog.variable("X")])).all()
	assert_equal(tom_children.size(), 2, "Tom has 2 children")
	print("    Tom's children: ", tom_children)

	# Query all children of bob
	var bob_children := prolog.solve(goal("parent", ["bob", prolog.variable("X")])).all()
	assert_equal(bob_children.size(), 2, "Bob has 2 children")

	teardown_prolog()


## Test Demo 02: Facts and Rules - Grandparents & Ancestors
func test_demo_02_facts_and_rules() -> void:
	print("\n[Test Suite: Demo 02 - Facts and Rules]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Load the demo file
	var consult_result := prolog.consult_file(DEMOS_PATH + "02_facts_and_rules.pl")
	if not consult_result:
		print("  ✗ SKIP: Could not load 02_facts_and_rules.pl")
		teardown_prolog()
		return
	assert_true(consult_result, "Load 02_facts_and_rules.pl")

	# Test grandparent rule
	assert_true(prolog.solve(goal("grandparent", ["tom", "ann"])).has_solution(), "Tom is grandparent of Ann")
	assert_true(prolog.solve(goal("grandparent", ["tom", "pat"])).has_solution(), "Tom is grandparent of Pat")
	assert_true(prolog.solve(goal("grandparent", ["bob", "jim"])).has_solution(), "Bob is grandparent of Jim")
	assert_false(prolog.solve(goal("grandparent", ["tom", "bob"])).has_solution(), "Tom is NOT grandparent of Bob")

	# Test sibling rule
	assert_true(prolog.solve(goal("sibling", ["bob", "liz"])).has_solution(), "Bob and Liz are siblings")
	assert_true(prolog.solve(goal("sibling", ["ann", "pat"])).has_solution(), "Ann and Pat are siblings")
	assert_false(prolog.solve(goal("sibling", ["bob", "bob"])).has_solution(), "Bob is not sibling of himself")

	# Test ancestor rule (recursive)
	assert_true(prolog.solve(goal("ancestor", ["tom", "bob"])).has_solution(), "Tom is ancestor of Bob")
	assert_true(prolog.solve(goal("ancestor", ["tom", "ann"])).has_solution(), "Tom is ancestor of Ann")
	assert_true(prolog.solve(goal("ancestor", ["tom", "jim"])).has_solution(), "Tom is ancestor of Jim (via bob->pat)")
	assert_true(prolog.solve(goal("ancestor", ["bob", "jim"])).has_solution(), "Bob is ancestor of Jim")

	# Query all grandchildren of tom
	var tom_grandchildren := prolog.solve(goal("grandparent", ["tom", prolog.variable("X")])).all()
	assert_true(tom_grandchildren.size() >= 2, "Tom has at least 2 grandchildren")
	print("    Tom's grandchildren: ", tom_grandchildren)

	teardown_prolog()


## Test Demo 03: Dynamic Assertions - Game State
func test_demo_03_dynamic_assertions() -> void:
	print("\n[Test Suite: Demo 03 - Dynamic Assertions]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Load the demo file (declares dynamic predicate)
	var consult_result := prolog.consult_file(DEMOS_PATH + "03_dynamic_assertions.pl")
	if not consult_result:
		print("  ✗ SKIP: Could not load 03_dynamic_assertions.pl")
		teardown_prolog()
		return
	assert_true(consult_result, "Load 03_dynamic_assertions.pl")

	# Initially no game_state facts
	assert_false(prolog.solve(goal("game_state", [prolog.anonymous(), prolog.anonymous()])).has_solution(), "No game_state facts initially")

	assert_true(prolog.assert_fact(goal("game_state", ["player_health", 100])), "Assert player_health")
	assert_true(prolog.assert_fact(goal("game_state", ["player_score", 0])), "Assert player_score")
	assert_true(prolog.assert_fact(goal("game_state", ["level", 1])), "Assert level")

	# Verify states exist
	assert_true(prolog.solve(goal("game_state", ["player_health", 100])).has_solution(), "player_health is 100")
	assert_true(prolog.solve(goal("game_state", ["player_score", 0])).has_solution(), "player_score is 0")
	assert_true(prolog.solve(goal("game_state", ["level", 1])).has_solution(), "level is 1")

	# Update a state (retract and reassert)
	assert_true(prolog.retract_fact(goal("game_state", ["player_score", 0])), "Retract old score")
	assert_true(prolog.assert_fact(goal("game_state", ["player_score", 100])), "Assert new score")
	assert_true(prolog.solve(goal("game_state", ["player_score", 100])).has_solution(), "player_score updated to 100")

	# Query all game states
	var all_states := prolog.solve(goal("game_state", [prolog.variable("Key"), prolog.variable("Value")])).all()
	assert_equal(all_states.size(), 3, "3 game states exist")
	print("    Game states: ", all_states)

	# Retract all game states
	prolog.retract_all(goal("game_state", [prolog.anonymous(), prolog.anonymous()]))
	assert_false(prolog.solve(goal("game_state", [prolog.anonymous(), prolog.anonymous()])).has_solution(), "All game_state facts removed")

	teardown_prolog()


## Test Demo 04: Complex Queries - Combat System
func test_demo_04_complex_queries() -> void:
	print("\n[Test Suite: Demo 04 - Complex Queries (Combat)]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Load the demo file
	var consult_result := prolog.consult_file(DEMOS_PATH + "04_complex_queries.pl")
	if not consult_result:
		print("  ✗ SKIP: Could not load 04_complex_queries.pl")
		teardown_prolog()
		return
	assert_true(consult_result, "Load 04_complex_queries.pl")

	# Test enemy facts
	assert_true(prolog.solve(goal("enemy", ["goblin", 10, 5, 2])).has_solution(), "Goblin stats exist")
	assert_true(prolog.solve(goal("enemy", ["orc", 25, 12, 5])).has_solution(), "Orc stats exist")
	assert_true(prolog.solve(goal("enemy", ["dragon", 100, 30, 15])).has_solution(), "Dragon stats exist")

	# Test weapon facts
	assert_true(prolog.solve(goal("weapon", ["sword", 10])).has_solution(), "Sword damage is 10")
	assert_true(prolog.solve(goal("weapon", ["axe", 15])).has_solution(), "Axe damage is 15")
	assert_true(prolog.solve(goal("weapon", ["bow", 8])).has_solution(), "Bow damage is 8")

	# Test damage calculation: damage = weapon_dmg - defense
	# Sword (10) vs Goblin (def 2) = 8 damage
	assert_true(prolog.solve(goal("damage", ["sword", "goblin", 8])).has_solution(), "Sword deals 8 damage to goblin")
	# Axe (15) vs Orc (def 5) = 10 damage
	assert_true(prolog.solve(goal("damage", ["axe", "orc", 10])).has_solution(), "Axe deals 10 damage to orc")
	# Bow (8) vs Dragon (def 15) = -7 damage (negative, ineffective)
	assert_true(prolog.solve(goal("damage", ["bow", "dragon", -7])).has_solution(), "Bow deals -7 damage to dragon")

	# Test one_shot_kill: axe (15) vs goblin (10 HP, 2 def) = 13 dmg >= 10 HP
	assert_true(prolog.solve(goal("one_shot_kill", ["axe", "goblin"])).has_solution(), "Axe can one-shot goblin")
	# Sword (10) vs goblin (10 HP, 2 def) = 8 dmg < 10 HP
	assert_false(prolog.solve(goal("one_shot_kill", ["sword", "goblin"])).has_solution(), "Sword cannot one-shot goblin")
	# No weapon can one-shot dragon
	assert_false(prolog.solve(goal("one_shot_kill", ["sword", "dragon"])).has_solution(), "Sword cannot one-shot dragon")
	assert_false(prolog.solve(goal("one_shot_kill", ["axe", "dragon"])).has_solution(), "Axe cannot one-shot dragon")

	# Query all enemies (anonymous variables are displayed as "null")
	var all_enemies := prolog.solve(goal("enemy", [prolog.variable("Name"), prolog.anonymous(), prolog.anonymous(), prolog.anonymous()])).all()
	assert_equal(all_enemies.size(), 3, "3 enemy types exist")
	print("    Enemies: ", all_enemies)

	teardown_prolog()


## Test Demo 05: Pathfinding - Graph Traversal
func test_demo_05_pathfinding() -> void:
	print("\n[Test Suite: Demo 05 - Pathfinding]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Load the demo file
	var consult_result := prolog.consult_file(DEMOS_PATH + "05_pathfinding.pl")
	if not consult_result:
		print("  ✗ SKIP: Could not load 05_pathfinding.pl")
		teardown_prolog()
		return
	assert_true(consult_result, "Load 05_pathfinding.pl")

	# Test edge facts
	assert_true(prolog.solve(goal("edge", ["a", "b", 1])).has_solution(), "Edge a->b exists with cost 1")
	assert_true(prolog.solve(goal("edge", ["b", "c", 2])).has_solution(), "Edge b->c exists with cost 2")
	assert_true(prolog.solve(goal("edge", ["e", "f", 1])).has_solution(), "Edge e->f exists with cost 1")

	# Test bidirectional connected predicate
	assert_true(prolog.solve(goal("connected", ["a", "b", 1])).has_solution(), "a connected to b")
	assert_true(prolog.solve(goal("connected", ["b", "a", 1])).has_solution(), "b connected to a (bidirectional)")

	# Test path finding - simple path a to b
	assert_true(prolog.solve(goal("path", ["a", "b", prolog.anonymous(), prolog.anonymous()])).has_solution(), "Path from a to b exists")

	# Test path finding - longer path a to f
	assert_true(prolog.solve(goal("path", ["a", "f", prolog.anonymous(), prolog.anonymous()])).has_solution(), "Path from a to f exists")

	# Query a specific path with cost
	var path_result: Variant = prolog.solve(goal("path", ["a", "f", prolog.variable("Path"), prolog.variable("Cost")])).first()
	assert_true(path_result != null, "Found path from a to f")
	print("    Path a->f: ", path_result)

	# Test that cycle detection works (no infinite loops)
	# Just verify the query completes without hanging
	var paths := prolog.solve(goal("path", ["a", "e", prolog.variable("Path"), prolog.variable("Cost")])).all()
	assert_true(paths.size() >= 1, "At least one path from a to e")
	print("    Paths a->e: ", paths)

	teardown_prolog()


## Test Demo 06: AI Behavior - Decision Making
func test_demo_06_ai_behavior() -> void:
	print("\n[Test Suite: Demo 06 - AI Behavior]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Load the demo file
	var consult_result := prolog.consult_file(DEMOS_PATH + "06_ai_behavior.pl")
	if not consult_result:
		print("  ✗ SKIP: Could not load 06_ai_behavior.pl")
		teardown_prolog()
		return
	assert_true(consult_result, "Load 06_ai_behavior.pl")

	# Test state facts
	assert_true(prolog.solve(goal("state", ["patrol"])).has_solution(), "patrol state exists")
	assert_true(prolog.solve(goal("state", ["chase"])).has_solution(), "chase state exists")
	assert_true(prolog.solve(goal("state", ["attack"])).has_solution(), "attack state exists")
	assert_true(prolog.solve(goal("state", ["flee"])).has_solution(), "flee state exists")

	# Test should_* predicates
	assert_true(prolog.solve(goal("should_chase", [5])).has_solution(), "should_chase at distance 5")
	assert_false(prolog.solve(goal("should_chase", [15])).has_solution(), "should NOT chase at distance 15")
	assert_true(prolog.solve(goal("should_attack", [2])).has_solution(), "should_attack at distance 2")
	assert_false(prolog.solve(goal("should_attack", [5])).has_solution(), "should NOT attack at distance 5")
	assert_true(prolog.solve(goal("should_flee", [10])).has_solution(), "should_flee at health 10")
	assert_false(prolog.solve(goal("should_flee", [50])).has_solution(), "should NOT flee at health 50")

	# Test decide_action - priority: flee > attack > chase > patrol
	# Low health -> flee (regardless of distance)
	assert_true(prolog.solve(goal("decide_action", ["flee", 10, 2])).has_solution(), "Flee when health=10")
	# Good health, close distance -> attack
	assert_true(prolog.solve(goal("decide_action", ["attack", 100, 2])).has_solution(), "Attack when close")
	# Good health, medium distance -> chase
	assert_true(prolog.solve(goal("decide_action", ["chase", 100, 5])).has_solution(), "Chase when medium distance")
	# Good health, far distance -> patrol
	assert_true(prolog.solve(goal("decide_action", ["patrol", 100, 20])).has_solution(), "Patrol when far")

	# Query all states
	var all_states := prolog.solve(goal("state", [prolog.variable("S")])).all()
	assert_equal(all_states.size(), 4, "4 AI states exist")
	print("    AI states: ", all_states)

	teardown_prolog()
