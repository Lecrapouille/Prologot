# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# Unit tests for the Prologot extension.
# Run these tests by adding this script to a Node in a test scene.

extends Node

## The Prologot engine instance for testing.
var prolog: Prologot

## Count of passed tests.
var tests_passed: int = 0

## Count of failed tests.
var tests_failed: int = 0

## Total tests run.
var tests_total: int = 0


func _ready() -> void:
	print("=" * 60)
	print("Prologot Unit Tests - Version 0.1.0")
	print("=" * 60)

	run_all_tests()

	print("")
	print("=" * 60)
	print("Test Results: %d passed, %d failed, %d total" % [tests_passed, tests_failed, tests_total])
	print("=" * 60)

	# Exit with appropriate code
	if tests_failed > 0:
		push_error("Some tests failed!")
	else:
		print("All tests passed!")


## Run all test suites.
func run_all_tests() -> void:
	test_initialization()
	test_basic_queries()
	test_fact_management()
	test_rules()
	test_dynamic_assertions()
	test_complex_queries()
	test_type_conversion()
	test_error_handling()


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
	var init_result := prolog.initialize()
	assert_true(init_result, "Engine initialization succeeds")

	# Test 4: Check initialized
	assert_true(prolog.is_initialized(), "Engine reports initialized")

	# Test 5: Double initialization should succeed (idempotent)
	var reinit_result := prolog.initialize()
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

	# Test simple query - true
	assert_true(prolog.query("animal(dog)"), "Query animal(dog) succeeds")

	# Test simple query - false
	assert_false(prolog.query("animal(fish)"), "Query animal(fish) fails")

	# Test query_all - returns Array of Variants
	var animals := prolog.query_all("animal(X)")
	assert_true(animals.size() >= 3, "query_all returns multiple results")
	print("    Animals found: ", animals)

	# Test query_one - returns Variant or null
	var one_animal := prolog.query_one("animal(X)")
	assert_true(one_animal != null, "query_one returns a result")
	print("    First animal: ", one_animal)

	# Test query_one with no solution
	var no_result := prolog.query_one("animal(unicorn)")
	assert_true(no_result == null, "query_one returns null when no solution")

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
	assert_true(prolog.query("likes(mary, food)"), "Fact likes(mary, food) exists")
	assert_true(prolog.query("likes(mary, wine)"), "Fact likes(mary, wine) exists")

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
	assert_true(prolog.query("grandparent(tom, ann)"), "Grandparent rule works")
	assert_true(prolog.query("grandparent(tom, pat)"), "Grandparent rule - second grandchild")
	assert_false(prolog.query("grandparent(bob, ann)"), "Non-grandparent correctly fails")

	# Test sibling rule
	assert_true(prolog.query("sibling(bob, liz)"), "Sibling rule works")
	assert_true(prolog.query("sibling(ann, pat)"), "Sibling rule - second pair")

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
	var assert_result := prolog.assert_fact("score(player1, 100)")
	assert_true(assert_result, "assert_fact succeeds")

	# Verify fact exists
	assert_true(prolog.query("score(player1, 100)"), "Asserted fact exists")

	# Test retract_fact
	var retract_result := prolog.retract_fact("score(player1, 100)")
	assert_true(retract_result, "retract_fact succeeds")

	# Verify fact removed
	assert_false(prolog.query("score(player1, 100)"), "Retracted fact no longer exists")

	# Test retract_all
	prolog.assert_fact("temp(a)")
	prolog.assert_fact("temp(b)")
	prolog.assert_fact("temp(c)")

	assert_true(prolog.query("temp(_)"), "Multiple temp facts exist")

	prolog.retract_all("temp(_)")
	assert_false(prolog.query("temp(_)"), "All temp facts removed")

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
	assert_true(prolog.query("weak(goblin)"), "Goblin is weak")
	assert_false(prolog.query("weak(dragon)"), "Dragon is not weak")
	assert_true(prolog.query("strong(dragon)"), "Dragon is strong")

	# Test query_all with complex results
	var weak_enemies := prolog.query_all("weak(X)")
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
	prolog.assert_fact("number_test(42)")
	assert_true(prolog.query("number_test(42)"), "Integer fact works")

	# Test string/atom handling
	prolog.assert_fact("name_test(hello)")
	assert_true(prolog.query("name_test(hello)"), "Atom fact works")

	# Test call_predicate with arguments
	prolog.consult_string("""
		add(X, Y, Z) :- Z is X + Y.
	""")

	# Note: call_predicate is for predicates without return value
	# call_function is for getting results
	var sum_result := prolog.call_function("add", [10, 20])
	# The result should be 30 if the predicate works correctly

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
	var bad_query := prolog.query("this is not valid prolog")
	assert_false(bad_query, "Invalid query returns false")

	# Test empty query
	var empty_query := prolog.query("")
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

