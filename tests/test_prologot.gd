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
	test_solve_and_query_text()
	test_lists_atoms_and_variants()
	test_consult_file_standalone()
	test_prolog_term_factories()
	test_prolog_variable()
	test_prolog_predicate()
	test_prolog_goal_composition()
	test_prolog_solution()
	test_solve_prolog_goal()

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


## Setup a fresh Prolog engine for testing.
func setup_prolog() -> bool:
	prolog = Prologot.new()
	return prolog.initialize()


## Cleanup the Prolog engine.
func teardown_prolog() -> void:
	if prolog:
		prolog.cleanup()
		prolog = null


## Build a PrologGoal from a functor name and arguments.
func goal(functor: String, args: Array = []) -> PrologGoal:
	return prolog.predicate(functor, args.size()).bindv(args)


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

	# Test 6: Cleanup
	prolog.cleanup()
	assert_false(prolog.is_initialized(), "Engine not initialized after cleanup")

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

	assert_true(prolog.succeeds(goal("animal", ["dog"])), "Query animal(dog) succeeds")
	assert_false(prolog.succeeds(goal("animal", ["fish"])), "Query animal(fish) fails")

	var animal := prolog.variable()
	var animals := prolog.solve_all(goal("animal", [animal]))
	assert_true(animals.size() >= 3, "solve_all returns multiple results")
	print("    Animals found: ", animals)

	var one_animal: Variant = prolog.solve_one(goal("animal", [animal]))
	assert_true(one_animal != null, "solve_one returns a result")
	print("    First animal: ", one_animal)

	var no_result: Variant = prolog.solve_one(goal("animal", ["unicorn"]))
	assert_true(no_result == null, "solve_one returns null when no solution")

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
	assert_true(prolog.succeeds(goal("likes", ["mary", "food"])), "Fact likes(mary, food) exists")
	assert_true(prolog.succeeds(goal("likes", ["mary", "wine"])), "Fact likes(mary, wine) exists")

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
	assert_true(prolog.succeeds(goal("grandparent", ["tom", "ann"])), "Grandparent rule works")
	assert_true(prolog.succeeds(goal("grandparent", ["tom", "pat"])), "Grandparent rule - second grandchild")
	assert_false(prolog.succeeds(goal("grandparent", ["bob", "ann"])), "Non-grandparent correctly fails")

	# Test sibling rule
	assert_true(prolog.succeeds(goal("sibling", ["bob", "liz"])), "Sibling rule works")
	assert_true(prolog.succeeds(goal("sibling", ["ann", "pat"])), "Sibling rule - second pair")

	teardown_prolog()


# =============================================================================
# Test: Dynamic Assertions
# =============================================================================

func test_dynamic_assertions() -> void:
	print("\n[Test Suite: Dynamic Assertions]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Test assert_fact
	var assert_result := prolog.add_fact("score(player1, 100)")
	assert_true(assert_result, "assert_fact succeeds")

	# Verify fact exists
	assert_true(prolog.succeeds(goal("score", ["player1", 100])), "Asserted fact exists")

	# Test retract_fact
	var retract_result := prolog.retract_fact("score(player1, 100)")
	assert_true(retract_result, "retract_fact succeeds")

	# Verify fact removed
	assert_false(prolog.succeeds(goal("score", ["player1", 100])), "Retracted fact no longer exists")

	# Test retract_all
	prolog.add_fact("temp(a)")
	prolog.add_fact("temp(b)")
	prolog.add_fact("temp(c)")

	assert_true(prolog.succeeds(goal("temp", [prolog.anonymous()])), "Multiple temp facts exist")

	prolog.retract_all("temp(_)")
	assert_false(prolog.succeeds(goal("temp", [prolog.anonymous()])), "All temp facts removed")

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
	assert_true(prolog.succeeds(goal("weak", ["goblin"])), "Goblin is weak")
	assert_false(prolog.succeeds(goal("weak", ["dragon"])), "Dragon is not weak")
	assert_true(prolog.succeeds(goal("strong", ["dragon"])), "Dragon is strong")

	# Test query_text_all with complex results
	var weak_enemies := prolog.solve_all(goal("weak", [prolog.variable("X")]))
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

	# Test integer handling
	prolog.add_fact("number_test(42)")
	assert_true(prolog.succeeds(goal("number_test", [42])), "Integer fact works")

	# Test negative integer handling
	prolog.add_fact("negative_test(-15)")
	assert_true(prolog.succeeds(goal("negative_test", [-15])), "Negative integer fact works")

	# Test floating point handling
	prolog.add_fact("float_test(3.14)")
	assert_true(prolog.succeeds(goal("float_test", [3.14])), "Float fact works")

	# Test multiple numeric arguments
	prolog.add_fact("coords(10, 20, 30)")
	assert_true(prolog.succeeds(goal("coords", [10, 20, 30])), "Multiple numeric arguments work")

	# Test mixed arguments (atoms and numbers)
	prolog.add_fact("player_data(alice, 100, 25.5)")
	assert_true(prolog.succeeds(goal("player_data", ["alice", 100, 25.5])), "Mixed atom and numeric arguments work")

	# Test large numbers
	prolog.add_fact("large_number(999999)")
	assert_true(prolog.succeeds(goal("large_number", [999999])), "Large number fact works")

	# Test zero
	prolog.add_fact("zero_test(0)")
	assert_true(prolog.succeeds(goal("zero_test", [0])), "Zero value fact works")

	# Test string/atom handling
	prolog.add_fact("name_test(hello)")
	assert_true(prolog.succeeds(goal("name_test", ["hello"])), "Atom fact works")

	# Test call_predicate with arguments
	prolog.consult_string("""
		add(X, Y, Z) :- Z is X + Y.
	""")

	# Note: call_predicate is for predicates without return value
	# call_function is for getting results
	var sum_result: Variant = prolog.call_function("add", [10, 20])
	assert_equal(sum_result, 30, "call_function returns correct result (10 + 20 = 30)")

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
	var result1: PrologSolution = prolog.solve_one(goal("distance", ["zone_1", "zone_2", distance_d]))
	assert_true(result1 != null, "Distance query returns a result")
	assert_true(abs(result1.get(distance_d) - 5.0) < 0.001, "Distance zone_1 to zone_2 is ~5.0")

	var exact_match := prolog.succeeds(goal("distance", ["zone_1", "zone_2", 5]))
	print("    Exact match (distance = 5): ", exact_match, " (may be false due to float precision)")

	assert_true(result1.get(distance_d) >= 4.99 and result1.get(distance_d) <= 5.01, "Distance within tolerance range [4.99, 5.01]")

	var result2: PrologSolution = prolog.solve_one(goal("distance", ["origin", "point_a", distance_d]))
	assert_true(result2 != null, "Distance origin to point_a calculated")

	var result3: PrologSolution = prolog.solve_one(goal("distance", ["zone_1", "zone_1", distance_d]))
	assert_true(abs(result3.get(distance_d)) < 0.001, "Distance from point to itself is 0")

	var result4: Variant = prolog.solve_one(goal("distance", ["zone_1", "zone_3", distance_d]))
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

	# Add facts dynamically using assert_fact
	assert_true(prolog.add_fact("at(alien_1, zone_1)"), "Add fact: at(alien_1, zone_1)")
	assert_true(prolog.add_fact("at(guard_1, zone_2)"), "Add fact: at(guard_1, zone_2)")
	assert_true(prolog.add_fact("distance(zone_1, zone_2, 3)"), "Add fact: distance(zone_1, zone_2, 3)")

	# Test 1: can_track should succeed (distance 3 < 5)
	var can_track_result := prolog.succeeds(goal("can_track", ["alien_1", "guard_1"]))
	assert_true(can_track_result, "can_track(alien_1, guard_1) succeeds (distance 3 < 5)")

	# Test 2: Add a target too far away
	assert_true(prolog.add_fact("at(guard_2, zone_3)"), "Add fact: at(guard_2, zone_3)")
	assert_true(prolog.add_fact("distance(zone_1, zone_3, 10)"), "Add fact: distance(zone_1, zone_3, 10)")

	# Test 3: can_track should fail (distance 10 >= 5)
	var cannot_track_result := prolog.succeeds(goal("can_track", ["alien_1", "guard_2"]))
	assert_false(cannot_track_result, "can_track(alien_1, guard_2) fails (distance 10 >= 5)")

	# Test 4: Add another alien and guard at same location (distance 0 < 5)
	assert_true(prolog.add_fact("at(alien_2, zone_1)"), "Add fact: at(alien_2, zone_1)")
	assert_true(prolog.add_fact("at(guard_3, zone_1)"), "Add fact: at(guard_3, zone_1)")
	assert_true(prolog.add_fact("distance(zone_1, zone_1, 0)"), "Add fact: distance(zone_1, zone_1, 0)")

	var same_location_result := prolog.succeeds(goal("can_track", ["alien_2", "guard_3"]))
	assert_true(same_location_result, "can_track(alien_2, guard_3) succeeds (same location, distance 0 < 5)")

	# Test 5: Edge case - distance exactly 5 should fail (< 5, not <= 5)
	assert_true(prolog.add_fact("at(guard_4, zone_4)"), "Add fact: at(guard_4, zone_4)")
	assert_true(prolog.add_fact("distance(zone_1, zone_4, 5)"), "Add fact: distance(zone_1, zone_4, 5)")

	var exact_boundary_result := prolog.succeeds(goal("can_track", ["alien_1", "guard_4"]))
	assert_false(exact_boundary_result, "can_track(alien_1, guard_4) fails (distance 5 is not < 5)")

	# Test 6: Verify facts exist
	assert_true(prolog.succeeds(goal("at", ["alien_1", "zone_1"])), "Fact at(alien_1, zone_1) exists")
	assert_true(prolog.succeeds(goal("distance", ["zone_1", "zone_2", 3])), "Fact distance(zone_1, zone_2, 3) exists")

	teardown_prolog()


# =============================================================================
# Test: Error Handling
# =============================================================================

func test_error_handling() -> void:
	print("\n[Test Suite: Error Handling]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	# Test query with syntax error (should not crash)
	var bad_query := prolog.query_text("this is not valid prolog")
	assert_false(bad_query, "Invalid query returns false")

	# Test empty query
	var empty_query := prolog.query_text("")
	assert_false(empty_query, "Empty query returns false")

	# Test retract non-existent fact
	var retract_missing := prolog.retract_fact("nonexistent_fact(x)")
	assert_false(retract_missing, "Retracting non-existent fact returns false")

	# Test consult_string with syntax error - should fail gracefully
	var bad_code := prolog.consult_string("invalid prolog code here")
	assert_false(bad_code, "Invalid code returns false")
	assert_true(prolog.get_last_error().length() > 0, "get_last_error returns error message")
	print("    Last error: ", prolog.get_last_error())

	teardown_prolog()


# =============================================================================
# Test: solve() (structured) vs query_text() (Prolog source)
# =============================================================================

func test_solve_and_query_text() -> void:
	print("\n[Test Suite: solve / query_text API]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	assert_true(prolog.consult_string("""
		parent(tom, bob).
		parent(tom, liz).
		parent(bob, ann).
	"""), "Load family facts")

	# High-level structured API
	assert_true(prolog.solve("parent", ["tom", "bob"]), "solve parent(tom, bob) succeeds")
	assert_false(prolog.solve("parent", ["bob", "tom"]), "solve parent(bob, tom) fails")

	var children := prolog.solve_all("parent", ["tom", "X"])
	assert_equal(children.size(), 2, "solve_all returns both children of tom")
	assert_true(children[0]["X"] == "bob" or children[1]["X"] == "bob", "solve_all binds X to bob")
	assert_true(children[0]["X"] == "liz" or children[1]["X"] == "liz", "solve_all binds X to liz")

	var first_child: Variant = prolog.solve_one("parent", ["tom", "X"])
	assert_true(first_child != null and first_child.has("X"), "solve_one returns {X: ...}")

	# Compound Dictionary form: solve(parent(tom, child))
	assert_true(
		prolog.solve({"functor": "parent", "args": ["tom", "bob"]}),
		"solve compound Dictionary succeeds"
	)

	var dict_children := prolog.solve_all({"functor": "parent", "args": ["tom", "X"]})
	assert_equal(dict_children.size(), 2, "solve_all from Dictionary extracts X")

	var via := prolog.variable("X")
	var child_y := prolog.variable("Y")
	assert_true(
		prolog.succeeds(goal("parent", ["tom", via]).conjunction(goal("parent", [via, child_y]))),
		"conjunction of two parent goals succeeds"
	)

	var child_x := prolog.variable("X")
	var text_results := prolog.solve_all(goal("parent", ["tom", child_x]))
	assert_equal(text_results.size(), 2, "solve_all returns two children of tom")
	assert_true(text_results[0].has(child_x), "each solution binds the variable object")

	# solve() must reject Prolog source strings
	assert_false(prolog.solve("parent(tom, bob)"), "solve rejects a Prolog source string")
	assert_true(prolog.get_last_error().length() > 0, "solve source-string error is reported")

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
	var nums: PrologSolution = prolog.solve_one(goal("nums", [list_var]))
	assert_true(nums != null, "nums/1 returns a solution")
	assert_true(nums.get(list_var) is Array, "Prolog list becomes a Godot Array")
	assert_equal(nums.get(list_var), [1, 2, 3], "List [1, 2, 3] round-trips")

	var empty: PrologSolution = prolog.solve_one(goal("empty_list", [list_var]))
	assert_equal(empty.get(list_var), [], "Empty Prolog list becomes []")

	var nested: PrologSolution = prolog.solve_one(goal("nested", [list_var]))
	assert_equal(nested.get(list_var), [["a", "b"], ["c"]], "Nested lists convert recursively")

	# GDScript strings become Prolog atoms (not Prolog strings)
	assert_true(prolog.succeeds(goal("named", ["hello"])), "Atom hello matches string 'hello'")
	assert_true(prolog.solve("named", ["hello"]), "solve() treats lowercase strings as atoms")

	# Compound Variant form used by solve()
	assert_true(
		prolog.solve({"functor": "named", "args": ["hello"]}),
		"Dictionary {functor, args} is accepted by solve()"
	)
	assert_false(
		prolog.solve({"functor": "named", "args": ["goodbye"]}),
		"Compound Variant fails when the fact does not exist"
	)

	# member/2 via call_predicate with a Godot Array
	assert_true(prolog.call_predicate("member", [2, [1, 2, 3]]), "member/2 accepts a Godot Array")
	assert_false(prolog.call_predicate("member", [9, [1, 2, 3]]), "member/2 fails for a missing element")

	var list_len: Variant = prolog.call_function("length", [[1, 2, 3, 4]])
	assert_equal(list_len, 4, "call_function length/2 on a list")

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
	assert_true(prolog.succeeds(goal("animal", ["dog"])), "Fact from consulted file is available")
	assert_true(prolog.succeeds(goal("animal", ["cat"])), "Second fact from consulted file is available")
	assert_false(prolog.succeeds(goal("animal", ["fish"])), "Missing fact from consulted file fails")

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

	var parent := prolog.predicate("parent", 2)
	assert_true(parent != null, "predicate() creates a PrologPredicate")
	assert_equal(parent.get_name(), "parent", "predicate name")
	assert_equal(parent.get_arity(), 2, "predicate arity")
	assert_equal(parent.as_text(), "parent/2", "predicate debug text")

	var child := prolog.variable("Child")
	var goal := parent.bind("tom", child)
	assert_true(goal != null, "bind() with matching arity returns a goal")
	assert_equal(goal.get_functor(), "parent", "bound goal functor")
	assert_equal(goal.get_arity(), 2, "bound goal arity")
	assert_true(goal.get_args()[1] == child, "bind() keeps the variable object")

	var bad := parent.bind("tom")
	assert_true(bad == null, "bind() rejects the wrong arity")

	var via_array := parent.bindv(["tom", child])
	assert_true(via_array != null and via_array.get_functor() == "parent", "bindv() accepts an Array")

	teardown_prolog()


func test_prolog_goal_composition() -> void:
	print("\n[Test Suite: PrologGoal composition]")

	if not setup_prolog():
		print("  ✗ SKIP: Could not initialize Prolog")
		return

	var parent := prolog.predicate("parent", 2)
	var child := prolog.variable("Child")
	var grand := prolog.variable("Grand")
	var left := parent.bind("tom", child)
	var right := parent.bind(child, grand)

	var both := left.conjunction(right)
	assert_true(both != null, "conjunction() builds a goal")
	assert_equal(both.get_functor(), ",", "conjunction uses ','/2")
	assert_equal(both.get_arity(), 2, "conjunction has two goals")

	var either := left.disjunction(right)
	assert_equal(either.get_functor(), ";", "disjunction uses ';'/2")

	var not_goal := left.negated()
	assert_equal(not_goal.get_functor(), "\\+", "negated uses \\+/1")
	assert_equal(not_goal.get_arity(), 1, "negated wraps one goal")

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

	var parent := prolog.predicate("parent", 2)
	var child := prolog.variable("Child")

	assert_true(prolog.succeeds(parent.bind("tom", "bob")), "succeeds parent(tom, bob)")
	assert_false(prolog.succeeds(parent.bind("bob", "tom")), "succeeds fails for missing fact")
	assert_true(prolog.solve(parent.bind("tom", "bob")), "solve(goal) is true when solutions exist")

	var solutions := prolog.solve_all(parent.bind("tom", child))
	assert_equal(solutions.size(), 2, "solve_all(goal) returns two children")
	var names := []
	for solution in solutions:
		assert_true(solution.has(child), "each solution binds the variable object")
		names.append(solution.get(child))
	assert_true("bob" in names and "liz" in names, "bindings are bob and liz")

	var first: PrologSolution = prolog.solve_one(parent.bind("tom", child))
	assert_true(first != null and first.has(child), "solve_one(goal) returns a PrologSolution")

	var none: Variant = prolog.solve_one(parent.bind("ann", child))
	assert_true(none == null, "solve_one(goal) is null when there is no solution")

	var grandchild := prolog.variable("Grand")
	var chain := parent.bind("tom", child).conjunction(parent.bind(child, grandchild))
	var chained := prolog.solve_all(chain)
	assert_equal(chained.size(), 1, "conjunction finds tom -> bob -> ann")
	assert_equal(chained[0].get(child), "bob", "shared variable stays bound across the conjunction")
	assert_equal(chained[0].get(grandchild), "ann", "second variable is bound")

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
	assert_true(prolog.succeeds(goal("parent", ["tom", "bob"])), "parent(tom, bob) exists")
	assert_true(prolog.succeeds(goal("parent", ["tom", "liz"])), "parent(tom, liz) exists")
	assert_true(prolog.succeeds(goal("parent", ["bob", "ann"])), "parent(bob, ann) exists")
	assert_true(prolog.succeeds(goal("parent", ["bob", "pat"])), "parent(bob, pat) exists")
	assert_true(prolog.succeeds(goal("parent", ["pat", "jim"])), "parent(pat, jim) exists")

	# Test non-existing relationships
	assert_false(prolog.succeeds(goal("parent", ["bob", "tom"])), "parent(bob, tom) should not exist")
	assert_false(prolog.succeeds(goal("parent", ["jim", "pat"])), "parent(jim, pat) should not exist")

	# Query all children of tom
	var tom_children := prolog.solve_all(goal("parent", ["tom", prolog.variable("X")]))
	assert_equal(tom_children.size(), 2, "Tom has 2 children")
	print("    Tom's children: ", tom_children)

	# Query all children of bob
	var bob_children := prolog.solve_all(goal("parent", ["bob", prolog.variable("X")]))
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
	assert_true(prolog.succeeds(goal("grandparent", ["tom", "ann"])), "Tom is grandparent of Ann")
	assert_true(prolog.succeeds(goal("grandparent", ["tom", "pat"])), "Tom is grandparent of Pat")
	assert_true(prolog.succeeds(goal("grandparent", ["bob", "jim"])), "Bob is grandparent of Jim")
	assert_false(prolog.succeeds(goal("grandparent", ["tom", "bob"])), "Tom is NOT grandparent of Bob")

	# Test sibling rule
	assert_true(prolog.succeeds(goal("sibling", ["bob", "liz"])), "Bob and Liz are siblings")
	assert_true(prolog.succeeds(goal("sibling", ["ann", "pat"])), "Ann and Pat are siblings")
	assert_false(prolog.succeeds(goal("sibling", ["bob", "bob"])), "Bob is not sibling of himself")

	# Test ancestor rule (recursive)
	assert_true(prolog.succeeds(goal("ancestor", ["tom", "bob"])), "Tom is ancestor of Bob")
	assert_true(prolog.succeeds(goal("ancestor", ["tom", "ann"])), "Tom is ancestor of Ann")
	assert_true(prolog.succeeds(goal("ancestor", ["tom", "jim"])), "Tom is ancestor of Jim (via bob->pat)")
	assert_true(prolog.succeeds(goal("ancestor", ["bob", "jim"])), "Bob is ancestor of Jim")

	# Query all grandchildren of tom
	var tom_grandchildren := prolog.solve_all(goal("grandparent", ["tom", prolog.variable("X")]))
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
	assert_false(prolog.succeeds(goal("game_state", [prolog.anonymous(), prolog.anonymous()])), "No game_state facts initially")

	# Add game states dynamically
	assert_true(prolog.add_fact("game_state(player_health, 100)"), "Assert player_health")
	assert_true(prolog.add_fact("game_state(player_score, 0)"), "Assert player_score")
	assert_true(prolog.add_fact("game_state(level, 1)"), "Assert level")

	# Verify states exist
	assert_true(prolog.succeeds(goal("game_state", ["player_health", 100])), "player_health is 100")
	assert_true(prolog.succeeds(goal("game_state", ["player_score", 0])), "player_score is 0")
	assert_true(prolog.succeeds(goal("game_state", ["level", 1])), "level is 1")

	# Update a state (retract and reassert)
	assert_true(prolog.retract_fact("game_state(player_score, 0)"), "Retract old score")
	assert_true(prolog.add_fact("game_state(player_score, 100)"), "Assert new score")
	assert_true(prolog.succeeds(goal("game_state", ["player_score", 100])), "player_score updated to 100")

	# Query all game states
	var all_states := prolog.solve_all(goal("game_state", [prolog.variable("Key"), prolog.variable("Value")]))
	assert_equal(all_states.size(), 3, "3 game states exist")
	print("    Game states: ", all_states)

	# Retract all game states
	prolog.retract_all("game_state(_, _)")
	assert_false(prolog.succeeds(goal("game_state", [prolog.anonymous(), prolog.anonymous()])), "All game_state facts removed")

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
	assert_true(prolog.succeeds(goal("enemy", ["goblin", 10, 5, 2])), "Goblin stats exist")
	assert_true(prolog.succeeds(goal("enemy", ["orc", 25, 12, 5])), "Orc stats exist")
	assert_true(prolog.succeeds(goal("enemy", ["dragon", 100, 30, 15])), "Dragon stats exist")

	# Test weapon facts
	assert_true(prolog.succeeds(goal("weapon", ["sword", 10])), "Sword damage is 10")
	assert_true(prolog.succeeds(goal("weapon", ["axe", 15])), "Axe damage is 15")
	assert_true(prolog.succeeds(goal("weapon", ["bow", 8])), "Bow damage is 8")

	# Test damage calculation: damage = weapon_dmg - defense
	# Sword (10) vs Goblin (def 2) = 8 damage
	assert_true(prolog.succeeds(goal("damage", ["sword", "goblin", 8])), "Sword deals 8 damage to goblin")
	# Axe (15) vs Orc (def 5) = 10 damage
	assert_true(prolog.succeeds(goal("damage", ["axe", "orc", 10])), "Axe deals 10 damage to orc")
	# Bow (8) vs Dragon (def 15) = -7 damage (negative, ineffective)
	assert_true(prolog.succeeds(goal("damage", ["bow", "dragon", -7])), "Bow deals -7 damage to dragon")

	# Test one_shot_kill: axe (15) vs goblin (10 HP, 2 def) = 13 dmg >= 10 HP
	assert_true(prolog.succeeds(goal("one_shot_kill", ["axe", "goblin"])), "Axe can one-shot goblin")
	# Sword (10) vs goblin (10 HP, 2 def) = 8 dmg < 10 HP
	assert_false(prolog.succeeds(goal("one_shot_kill", ["sword", "goblin"])), "Sword cannot one-shot goblin")
	# No weapon can one-shot dragon
	assert_false(prolog.succeeds(goal("one_shot_kill", ["sword", "dragon"])), "Sword cannot one-shot dragon")
	assert_false(prolog.succeeds(goal("one_shot_kill", ["axe", "dragon"])), "Axe cannot one-shot dragon")

	# Query all enemies (anonymous variables are displayed as "null")
	var all_enemies := prolog.solve_all(goal("enemy", [prolog.variable("Name"), prolog.anonymous(), prolog.anonymous(), prolog.anonymous()]))
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
	assert_true(prolog.succeeds(goal("edge", ["a", "b", 1])), "Edge a->b exists with cost 1")
	assert_true(prolog.succeeds(goal("edge", ["b", "c", 2])), "Edge b->c exists with cost 2")
	assert_true(prolog.succeeds(goal("edge", ["e", "f", 1])), "Edge e->f exists with cost 1")

	# Test bidirectional connected predicate
	assert_true(prolog.succeeds(goal("connected", ["a", "b", 1])), "a connected to b")
	assert_true(prolog.succeeds(goal("connected", ["b", "a", 1])), "b connected to a (bidirectional)")

	# Test path finding - simple path a to b
	assert_true(prolog.succeeds(goal("path", ["a", "b", prolog.anonymous(), prolog.anonymous()])), "Path from a to b exists")

	# Test path finding - longer path a to f
	assert_true(prolog.succeeds(goal("path", ["a", "f", prolog.anonymous(), prolog.anonymous()])), "Path from a to f exists")

	# Query a specific path with cost
	var path_result: Variant = prolog.solve_one(goal("path", ["a", "f", prolog.variable("Path"), prolog.variable("Cost")]))
	assert_true(path_result != null, "Found path from a to f")
	print("    Path a->f: ", path_result)

	# Test that cycle detection works (no infinite loops)
	# Just verify the query completes without hanging
	var paths := prolog.solve_all(goal("path", ["a", "e", prolog.variable("Path"), prolog.variable("Cost")]))
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
	assert_true(prolog.succeeds(goal("state", ["patrol"])), "patrol state exists")
	assert_true(prolog.succeeds(goal("state", ["chase"])), "chase state exists")
	assert_true(prolog.succeeds(goal("state", ["attack"])), "attack state exists")
	assert_true(prolog.succeeds(goal("state", ["flee"])), "flee state exists")

	# Test should_* predicates
	assert_true(prolog.succeeds(goal("should_chase", [5])), "should_chase at distance 5")
	assert_false(prolog.succeeds(goal("should_chase", [15])), "should NOT chase at distance 15")
	assert_true(prolog.succeeds(goal("should_attack", [2])), "should_attack at distance 2")
	assert_false(prolog.succeeds(goal("should_attack", [5])), "should NOT attack at distance 5")
	assert_true(prolog.succeeds(goal("should_flee", [10])), "should_flee at health 10")
	assert_false(prolog.succeeds(goal("should_flee", [50])), "should NOT flee at health 50")

	# Test decide_action - priority: flee > attack > chase > patrol
	# Low health -> flee (regardless of distance)
	assert_true(prolog.succeeds(goal("decide_action", ["flee", 10, 2])), "Flee when health=10")
	# Good health, close distance -> attack
	assert_true(prolog.succeeds(goal("decide_action", ["attack", 100, 2])), "Attack when close")
	# Good health, medium distance -> chase
	assert_true(prolog.succeeds(goal("decide_action", ["chase", 100, 5])), "Chase when medium distance")
	# Good health, far distance -> patrol
	assert_true(prolog.succeeds(goal("decide_action", ["patrol", 100, 20])), "Patrol when far")

	# Query all states
	var all_states := prolog.solve_all(goal("state", [prolog.variable("S")]))
	assert_equal(all_states.size(), 4, "4 AI states exist")
	print("    AI states: ", all_states)

	teardown_prolog()
