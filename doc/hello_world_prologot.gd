# Minimal script-only sample. For a runnable scene with nodes, see:
#   demos/mini_dungeon/  (make run-mini-dungeon)

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

	var parent = prolog.predicate("parent")
	var grandparent = prolog.predicate("grandparent")
	var child = prolog.variable("Child")
	var ancestor = prolog.variable("Ancestor")

	if prolog.solve(grandparent.call("tom", "ann")).has_solution():
		print("Tom is Ann's grandparent!")

	for solution in prolog.solve(parent.call(ancestor, child)):
		print("Parent: ", solution.get(ancestor), " -> ", solution.get(child))

	var via = prolog.variable("Via")
	if prolog.solve(parent.call("tom", via).conjunction(parent.call(via, "ann"))).has_solution():
		print("Tom is Ann's grandparent via a conjunction")

func _exit_tree():
	# Clean up the Prolog engine
	if prolog:
		prolog.cleanup()
