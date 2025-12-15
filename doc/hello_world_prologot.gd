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

	# Execute a query
	if prolog.query("grandparent(tom, ann)"):
		print("Tom is Ann's grandparent!")

	# Get all solutions (legacy format)
	var results = prolog.query_all("parent(X, Y)")
	print("Parent relationships: ", results)

	# Get all solutions with variable extraction (new format)
	var results_dict = prolog.query_all("parent", ["X", "Y"])
	for result in results_dict:
		print("Parent: ", result["X"], " -> ", result["Y"])

func _exit_tree():
	# Clean up the Prolog engine
	if prolog:
		prolog.cleanup()
