###############################################################################
# Prologot Showcase Script (English version)
#
# This script creates an interactive interface to navigate and run various
# Prolog code examples. It loads examples from a JSON file and lets users
# view the Prolog code and query results in a graphical interface.
###############################################################################

extends Node2D

# Main Prologot instance to execute Prolog queries
var prolog: Prologot

# References to UI elements
@onready var title_label = $CanvasLayer/MarginContainer/VBoxContainer/HeaderPanel/MarginContainer/HBoxContainer/TitleLabel
@onready var counter_label = $CanvasLayer/MarginContainer/VBoxContainer/HeaderPanel/MarginContainer/HBoxContainer/CounterLabel
@onready var code_display = $CanvasLayer/MarginContainer/VBoxContainer/ContentSplit/CodePanel/MarginContainer/VBoxContainer/CodeDisplay
@onready var result_display = $CanvasLayer/MarginContainer/VBoxContainer/ContentSplit/ResultPanel/MarginContainer/VBoxContainer/ResultDisplay
@onready var execute_button = $CanvasLayer/MarginContainer/VBoxContainer/ButtonBar/ExecuteButton
@onready var prev_button = $CanvasLayer/MarginContainer/VBoxContainer/ButtonBar/PrevButton
@onready var next_button = $CanvasLayer/MarginContainer/VBoxContainer/ButtonBar/NextButton

# Example data loaded from examples.json
var examples: Array = []
# Index of the currently displayed example
var current_index: int = 0

###############################################################################
# Initialization
###############################################################################
func _ready() -> void:
	# Set the background color of the scene
	RenderingServer.set_default_clear_color(Color(0.15, 0.15, 0.18))

	# Initialize Prologot instance to run queries
	prolog = Prologot.new()
	if not prolog.initialize():
		push_error("Failed to initialize Prologot: " + prolog.get_last_error())
		return

	# Apply the visual theme to all UI elements
	apply_theme()

	# Load examples configuration from examples.json
	if not load_examples():
		return

	# Load and display the first example
	load_example(0)

###############################################################################
# Called when the node is removed from the scene tree.
# Cleans up Prologot resources to free memory
###############################################################################
func _exit_tree() -> void:
	if prolog:
		prolog.cleanup()

###############################################################################
# Load the list of examples from the JSON config file
###############################################################################
func load_examples() -> bool:
	var file = FileAccess.open("res://examples/examples.json", FileAccess.READ)
	if file:
		var json = JSON.new()
		var parse_result = json.parse(file.get_as_text())
		if parse_result == OK:
			examples = json.data
			if examples.size() == 0:
				push_error("No examples loaded!")
				return false
		else:
			push_error("Failed to parse examples.json: " + json.get_error_message())
			return false
		file.close()
	else:
		push_error("Failed to open examples.json")
		return false

	print("Loaded %d examples" % examples.size())
	return true

###############################################################################
# Load and display a specific example in the UI
#
# @param index: The index of the example to load in the examples array
###############################################################################
func load_example(index: int) -> void:
	# Check index validity
	if index < 0 or index >= examples.size():
		return

	current_index = index
	var example = examples[index]

	# Update UI elements
	title_label.text = example["title"]
	counter_label.text = "%d/%d" % [index + 1, examples.size()]
	result_display.text = "Click 'Execute' to see results..."

	# Load and display the corresponding Prolog code
	var prolog_file = example["file"]
	var code = load_prolog_file(prolog_file)
	if example.has("description"):
		# If a description is available, display it with the code
		code_display.text = example["description"] + "\n\n" + code
	else:
		code_display.text = code

	# Update state of navigation buttons
	# Disable "Previous" if at the first example
	prev_button.disabled = (index == 0)
	# Disable "Next" if at the last example
	next_button.disabled = (index == examples.size() - 1)

	# Reset Prologot state for a clean environment
	prolog.cleanup()
	prolog.initialize()

###############################################################################
# Load the content of a Prolog file from the filesystem
#
# @param path: The path to the Prolog file to load
# @return: The file content as string, or an error message
###############################################################################
func load_prolog_file(path: String) -> String:
	var file = FileAccess.open(path, FileAccess.READ)
	if file:
		var content = file.get_as_text()
		file.close()
		return content
	return "Error loading file: " + path

###############################################################################
# Event handler called when the "Execute" button is pressed
# Loads the current example's Prolog file and runs the relevant queries
###############################################################################
func _on_execute_button_pressed() -> void:
	result_display.text = ""

	var example = examples[current_index]
	var prolog_file = example["file"]

	# Load the Prolog file into Prologot
	# res:// paths are automatically handled
	if not prolog.consult_file(prolog_file):
		result_display.text = "❌ ERROR\n\nFailed to load file:\n%s" % prolog_file
		return

	# Run specific queries based on the selected example
	match current_index:
		0: execute_basic_queries()
		1: execute_facts_and_rules()
		2: execute_dynamic_assertions()
		3: execute_complex_queries()
		4: execute_pathfinding()
		5: execute_ai_behavior()

###############################################################################
# Run basic Prolog queries to demonstrate fundamental features:
# simple true/false query and retrieving all solutions with variables
###############################################################################
func execute_basic_queries() -> void:
	result_display.text += "🔍 Query: parent(tom, bob)\n"
	var result = prolog.query("parent(tom, bob)")
	if result:
		result_display.text += "✓ TRUE\n"
		result_display.text += "  → Tom is Bob's parent\n\n"
	else:
		result_display.text += "✗ FALSE\n\n"

	result_display.text += "🔍 Query: parent(tom, X)\n"
	result_display.text += "  Find all Tom's children\n\n"
	var children = prolog.query_all("parent(tom, X)")
	result_display.text += "📋 Solutions found: %d\n" % children.size()
	for i in range(children.size()):
		result_display.text += "  %d. %s\n" % [i + 1, format_solution(children[i])]

###############################################################################
# Demonstrate using Prolog rules, including recursive rules for expressing
# complex relations between entities
###############################################################################
func execute_facts_and_rules() -> void:
	# Grandparent derived rule based on basic facts
	result_display.text += "🔍 Query: grandparent(tom, ann)\n"
	var is_grandparent = prolog.query("grandparent(tom, ann)")
	result_display.text += "%s\n" % ("✓ TRUE - Rule matched!" if is_grandparent else "✗ FALSE")
	result_display.text += "  → Tom is Ann's grandparent\n\n"

	# Recursive rule to find all descendants
	result_display.text += "🔍 Query: ancestor(tom, X)\n"
	result_display.text += "  Recursive rule test\n\n"
	var descendants = prolog.query_all("ancestor(tom, X)")
	result_display.text += "📋 Descendants: %d\n" % descendants.size()
	for i in range(descendants.size()):
		result_display.text += "  %d. %s\n" % [i + 1, format_solution(descendants[i])]

	# Retrieve siblings
	result_display.text += "\n🔍 Query: sibling(bob, X)\n"
	var siblings = prolog.query_all("sibling(bob, X)")
	result_display.text += "📋 Siblings: %d\n" % siblings.size()
	for i in range(siblings.size()):
		result_display.text += "  %d. %s\n" % [i + 1, format_solution(siblings[i])]

###############################################################################
# Demonstrate dynamically modifying the Prolog fact base at runtime:
# adding, removing, and updating facts
###############################################################################
func execute_dynamic_assertions() -> void:
	result_display.text += "➕ Adding facts:\n"
	prolog.add_fact("game_state(level, 1)")
	prolog.add_fact("game_state(score, 0)")
	prolog.add_fact("game_state(health, 100)")
	result_display.text += "  ✓ game_state(level, 1)\n"
	result_display.text += "  ✓ game_state(score, 0)\n"
	result_display.text += "  ✓ game_state(health, 100)\n\n"

	# Query for a single value (query_one returns Variant or null)
	result_display.text += "🔍 Query: game_state(level, X)\n"
	var level = prolog.query_one("game_state(level, X)")
	if level:
		result_display.text += "📌 Result = %s\n\n" % format_value(level)
	else:
		result_display.text += "📌 No result\n\n"

	# Update: remove previous score and add new one
	result_display.text += "🔄 Updating score:\n"
	prolog.retract_fact("game_state(score, 0)")
	prolog.add_fact("game_state(score, 150)")
	result_display.text += "  ✗ Retracted: game_state(score, 0)\n"
	result_display.text += "  ✓ Added: game_state(score, 150)\n\n"

	# Check update
	var score = prolog.query_one("game_state(score, X)")
	if score:
		result_display.text += "📌 Result = %s\n\n" % format_value(score)
	else:
		result_display.text += "📌 No result\n\n"

	# Cleanup: remove all game_state facts matching the pattern
	result_display.text += "🗑️  Cleanup: retract_all(game_state(_,_))\n"
	prolog.retract_all("game_state(_,_)")
	result_display.text += "✓ All game_state facts removed\n"

###############################################################################
# Demonstrate complex queries using Prolog predicates and functions
# for business rule calculations and decisions
###############################################################################
func execute_complex_queries() -> void:
	# Predicate test: can this weapon one-shot the enemy?
	result_display.text += "🎯 one_shot_kill(axe, goblin)\n"
	var can_kill = prolog.call_predicate("one_shot_kill", ["axe", "goblin"])
	result_display.text += "%s Can axe one-shot goblin\n\n" % ("✓" if can_kill else "✗")

	# Call function: compute weapon damage
	result_display.text += "⚔️  damage(sword, orc, D)\n"
	var damage = prolog.call_function("damage", ["sword", "orc"])
	result_display.text += "📊 Damage = %s\n\n" % format_value(damage)

	# Weapon analysis versus a specific enemy
	result_display.text += "🗡️  Weapon Analysis vs Goblin:\n"
	result_display.text += "─────────────────────────\n"
	for weapon in ["sword", "axe", "bow"]:
		var dmg = prolog.call_function("damage", [weapon, "goblin"])
		var kills = prolog.call_predicate("one_shot_kill", [weapon, "goblin"])
		var kill_icon = "💀" if kills else "❌"
		result_display.text += "  %s %s: %s dmg %s\n" % [
			kill_icon,
			weapon.capitalize().rpad(6),
			str(dmg).lpad(2),
			"(kills)" if kills else ""
		]

###############################################################################
# Demonstrate graph pathfinding with Prolog, showing possible routes and costs
###############################################################################
func execute_pathfinding() -> void:
	# Find all paths from node 'a' to 'f'
	result_display.text += "🗺️  Find: path(a, f, Path, Cost)\n"
	result_display.text += "   From node 'a' to 'f'\n\n"
	var paths = prolog.query_all("path(a, f, Path, Cost)")
	result_display.text += "📋 Paths found: %d\n\n" % paths.size()

	# Show up to 5 paths (for readability)
	for i in range(min(paths.size(), 5)):
		var path_info = format_solution(paths[i])
		result_display.text += "  %d. %s\n" % [i + 1, path_info]

	# Display graph structure (all edges)
	result_display.text += "\n🔗 Graph edges:\n"
	result_display.text += "─────────────────────\n"
	var edges = prolog.query_all("edge(X, Y, C)")
	for edge in edges:
		var formatted = format_solution(edge)
		result_display.text += "  %s\n" % formatted

###############################################################################
# Demonstrate an AI decision system using Prolog that chooses actions
# based on health and enemy distance
###############################################################################
func execute_ai_behavior() -> void:
	# Define test scenarios for the AI
	var scenarios = [
		{"health": 15, "distance": 8, "desc": "Low health", "icon": "🩸"},
		{"health": 50, "distance": 2, "desc": "Close enemy", "icon": "⚔️"},
		{"health": 50, "distance": 8, "desc": "Medium range", "icon": "🎯"},
		{"health": 100, "distance": 15, "desc": "Far distance", "icon": "👁️"}
	]

	result_display.text += "🤖 AI Decision Tests:\n"
	result_display.text += "─────────────────────────\n"

	# For each scenario, ask Prolog for the action decision
	for scenario in scenarios:
		var action = prolog.call_function("decide_action", [scenario["health"], scenario["distance"]])
		var action_str = format_value(action)
		var action_icon = get_action_icon(action_str)

		# Display the result
		result_display.text += "\n%s %s\n" % [scenario["icon"], scenario["desc"]]
		result_display.text += "  HP: %d | Dist: %d\n" % [scenario["health"], scenario["distance"]]
		result_display.text += "  → %s %s\n" % [action_icon, action_str.to_upper()]

###############################################################################
# Return the emoji icon matching an AI action name
#
# @param action: The action name (flee, attack, chase, patrol)
# @return: The emoji corresponding to the action
###############################################################################
func get_action_icon(action: String) -> String:
	match action:
		"flee": return "🏃"
		"attack": return "⚔️"
		"chase": return "🏃‍♂️"
		"patrol": return "🚶"
		_: return "❓"

###############################################################################
# Format a Prolog solution as a human-readable string.
# Handles different Prolog data types:
# - Atom: simple string (e.g. "tom")
# - Compound term: dictionary {"functor": "name", "args": [...]}
# - List: array [elem1, elem2, ...]
#
# @param solution: The Prolog solution to format
# @return: Solution formatted as string
###############################################################################
func format_solution(solution) -> String:
	# Handle possible Prolog formats:
	# - Atom: String
	# - Compound term: Dictionary {"functor": "name", "args": [...]}
	# - List: Array
	if solution is Dictionary and solution.has("functor") and solution.has("args"):
		var functor = solution["functor"]
		var args = []
		for arg in solution["args"]:
			args.append(format_value(arg))
		return "%s(%s)" % [functor, ", ".join(args)]
	return format_value(solution)

###############################################################################
# Format any Prolog value as a string.
# Handles null, Dictionary, Array, and core types.
#
# @param value: Value to format
# @return: String representation
###############################################################################
func format_value(value) -> String:
	if value == null:
		return "null"
	if value is Dictionary:
		if value.has("functor") and value.has("args"):
			return format_solution(value)
		return str(value)
	if value is Array:
		var items = []
		for item in value:
			items.append(format_value(item))
		return "[%s]" % ", ".join(items)
	return str(value)

###############################################################################
# Navigation button event handlers
###############################################################################

# Handler for "Previous" button: load previous example
func _on_prev_button_pressed() -> void:
	if current_index > 0:
		load_example(current_index - 1)

# Handler for "Next" button: load next example
func _on_next_button_pressed() -> void:
	if current_index < examples.size() - 1:
		load_example(current_index + 1)

###############################################################################
# Apply the visual theme to all UI elements (colors, styles, etc.)
###############################################################################
func apply_theme() -> void:
	# Header panel style
	var header_panel = $CanvasLayer/MarginContainer/VBoxContainer/HeaderPanel
	var header_style = StyleBoxFlat.new()
	header_style.bg_color = Color(0.25, 0.35, 0.5)
	header_style.set_corner_radius_all(8)
	header_panel.add_theme_stylebox_override("panel", header_style)
	title_label.add_theme_color_override("font_color", Color.WHITE)
	counter_label.add_theme_color_override("font_color", Color(0.9, 0.9, 1.0))

	# Prolog code panel style
	var code_panel = $CanvasLayer/MarginContainer/VBoxContainer/ContentSplit/CodePanel
	var code_panel_style = StyleBoxFlat.new()
	code_panel_style.bg_color = Color(0.2, 0.22, 0.25)
	code_panel_style.set_corner_radius_all(8)
	code_panel.add_theme_stylebox_override("panel", code_panel_style)

	var code_label = $CanvasLayer/MarginContainer/VBoxContainer/ContentSplit/CodePanel/MarginContainer/VBoxContainer/CodeLabel
	code_label.add_theme_color_override("font_color", Color(0.8, 0.9, 1.0))

	# Code display area (terminal-like dark background)
	var code_style = StyleBoxFlat.new()
	code_style.bg_color = Color(0.12, 0.12, 0.15)
	code_style.set_corner_radius_all(5)
	code_display.add_theme_stylebox_override("normal", code_style)
	code_display.add_theme_color_override("font_color", Color(0.85, 0.95, 0.85))

	# Result panel style
	var result_panel = $CanvasLayer/MarginContainer/VBoxContainer/ContentSplit/ResultPanel
	var result_panel_style = StyleBoxFlat.new()
	result_panel_style.bg_color = Color(0.2, 0.22, 0.25)
	result_panel_style.set_corner_radius_all(8)
	result_panel.add_theme_stylebox_override("panel", result_panel_style)

	var result_label = $CanvasLayer/MarginContainer/VBoxContainer/ContentSplit/ResultPanel/MarginContainer/VBoxContainer/ResultLabel
	result_label.add_theme_color_override("font_color", Color(1.0, 0.9, 0.7))

	# Result display area
	var result_style = StyleBoxFlat.new()
	result_style.bg_color = Color(0.12, 0.12, 0.15)
	result_style.set_corner_radius_all(5)
	result_display.add_theme_stylebox_override("normal", result_style)
	result_display.add_theme_color_override("font_color", Color(0.9, 0.95, 0.9))

	# Button styles
	# Execute button: green (main action)
	style_button(execute_button, Color(0.2, 0.7, 0.3), Color(0.25, 0.8, 0.35), Color(0.15, 0.6, 0.25))
	# Prev/Next buttons: blue (navigation)
	style_button(prev_button, Color(0.3, 0.4, 0.6), Color(0.35, 0.5, 0.7), Color(0.25, 0.3, 0.5))
	style_button(next_button, Color(0.3, 0.4, 0.6), Color(0.35, 0.5, 0.7), Color(0.25, 0.3, 0.5))

###############################################################################
# Apply a custom style to a button with colors for different states
# (normal, hover, pressed, disabled)
#
# @param button: Button to style
# @param normal_color: Color for normal state
# @param hover_color: Color for mouse hover state
# @param pressed_color: Color for pressed state
###############################################################################
func style_button(button: Button, normal_color: Color, hover_color: Color, pressed_color: Color) -> void:
	# Normal state
	var normal_style = StyleBoxFlat.new()
	normal_style.bg_color = normal_color
	normal_style.set_corner_radius_all(8)

	# Hover state
	var hover_style = StyleBoxFlat.new()
	hover_style.bg_color = hover_color
	hover_style.set_corner_radius_all(8)

	# Pressed state
	var pressed_style = StyleBoxFlat.new()
	pressed_style.bg_color = pressed_color
	pressed_style.set_corner_radius_all(8)

	# Disabled state
	var disabled_style = StyleBoxFlat.new()
	disabled_style.bg_color = Color(0.3, 0.3, 0.35)
	disabled_style.set_corner_radius_all(8)

	# Assign styles
	button.add_theme_stylebox_override("normal", normal_style)
	button.add_theme_stylebox_override("hover", hover_style)
	button.add_theme_stylebox_override("pressed", pressed_style)
	button.add_theme_stylebox_override("disabled", disabled_style)
	button.add_theme_color_override("font_color", Color.WHITE)
	button.add_theme_color_override("font_disabled_color", Color(0.5, 0.5, 0.55))