# This demonstrates the basic usage of Prologot in a Godot project.

extends Node

var prolog: Prologot

func _ready():
	# Create a new Prologot instance
	prolog = Prologot.new()

	# Initialize the Prolog engine
	if not prolog.initialize():
		push_error("Failed to initialize Prologot: " + prolog.get_last_error())
		return

	# Load facts
	prolog.consult_string("""
		parent(tom, bob).
		parent(bob, ann).
		grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
	""")

	var parent = prolog.predicate("parent", 2)
	var grandparent = prolog.predicate("grandparent", 2)
	var child = prolog.variable("Child")
	var ancestor = prolog.variable("Ancestor")

	if prolog.succeeds(grandparent.bind("tom", "ann")):
		print("Tom is Ann's grandparent!")

	for solution in prolog.solve_all(parent.bind(ancestor, child)):
		print("Parent: ", solution.get(ancestor), " -> ", solution.get(child))

	var via = prolog.variable("Via")
	if prolog.succeeds(parent.bind("tom", via).conjunction(parent.bind(via, "ann"))):
		print("Tom is Ann's grandparent via a conjunction")

func _exit_tree():
	# Clean up the Prolog engine
	if prolog:
		prolog.cleanup()
