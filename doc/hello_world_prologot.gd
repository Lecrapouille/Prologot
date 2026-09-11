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

	# High-level API: structured terms (no Prolog source parsing)
	if prolog.solve("grandparent", ["tom", "ann"]):
		print("Tom is Ann's grandparent!")

	var results_dict = prolog.solve_all("parent", ["X", "Y"])
	for result in results_dict:
		print("Parent: ", result["X"], " -> ", result["Y"])

	# Low-level API: Prolog source text (conjunctions, operators, REPL)
	if prolog.query_text("parent(tom, X), parent(X, ann)"):
		print("Tom is Ann's grandparent via a conjunction")

	var results = prolog.query_text_all("parent(X, Y)")
	print("Parent relationships: ", results)

func _exit_tree():
	# Clean up the Prolog engine
	if prolog:
		prolog.cleanup()
