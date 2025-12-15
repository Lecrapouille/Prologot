# MIT License
# Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
#
# Prologot - SWI-Prolog integration for Godot 4
#
# This dock provides an interactive Prolog console in the Godot editor.
# Users can execute queries, load files, and manage Prolog code.

@tool
extends VBoxContainer

## Input field for Prolog queries.
## Users can type Prolog queries here and execute them with Enter or the Execute button.
var query_input: LineEdit

## Text area displaying query results.
## Shows the output of executed queries in a read-only text area.
var result_output: TextEdit

## List showing loaded predicates.
## Displays all predicates currently available in the Prolog knowledge base.
var predicates_list: ItemList

## Text area for entering Prolog code.
## Allows users to write Prolog code directly in the editor and load it.
var code_input: TextEdit

## Prologot engine instance (set by the plugin).
## This is the Prolog engine used for executing queries in the editor dock.
## Set by the plugin when the dock is created.
var engine = null


###############################################################################
## Initialize the dock when it enters the scene tree.
###############################################################################
func _ready() -> void:
	name = "Prologot Console"
	_build_ui()

###############################################################################
## Builds the dock UI components.
##
## Constructs the entire user interface for the Prologot console dock.
## The UI is organized into sections separated by horizontal separators:
## - Title header
## - Query input and results
## - Action buttons
## - Code input area
## - Predicates list
###############################################################################
func _build_ui() -> void:
	# Create and add the title label
	var title := Label.new()
	title.text = "Prologot Console"
	title.add_theme_font_size_override("font_size", 16)
	add_child(title)

	# Query and results section
	add_child(HSeparator.new())
	_build_query_section()
	_build_result_section()
	_build_action_buttons()

	# Code input section
	add_child(HSeparator.new())
	_build_code_section()

	# Predicates list section
	add_child(HSeparator.new())
	_build_predicates_section()

###############################################################################
## Builds the query input section.
##
## Creates a horizontal container with a label, text input field for Prolog
## queries, and an Execute button. Users can submit queries by pressing Enter
## in the input field or clicking the Execute button.
###############################################################################
func _build_query_section() -> void:
	var query_container := HBoxContainer.new()

	# Label for the query input
	var query_label := Label.new()
	query_label.text = "Query:"
	query_label.custom_minimum_size.x = 60
	query_container.add_child(query_label)

	# Text input field for entering Prolog queries
	query_input = LineEdit.new()
	query_input.placeholder_text = "e.g., parent(X, bob)"
	query_input.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	# Connect Enter key press to execute query
	query_input.text_submitted.connect(_on_query_submitted)
	query_container.add_child(query_input)

	# Execute button
	var query_btn := Button.new()
	query_btn.text = "Execute"
	query_btn.pressed.connect(_on_query_button_pressed)
	query_container.add_child(query_btn)

	add_child(query_container)

###############################################################################
## Builds the result display section.
##
## Creates a read-only text area that displays the results of executed Prolog
## queries. The area expands vertically to fill available space and shows
## formatted query results.
###############################################################################
func _build_result_section() -> void:
	var result_label := Label.new()
	result_label.text = "Results:"
	add_child(result_label)

	# Read-only text area for displaying query results
	result_output = TextEdit.new()
	result_output.editable = false
	result_output.custom_minimum_size.y = 100
	result_output.size_flags_vertical = Control.SIZE_EXPAND_FILL
	add_child(result_output)

###############################################################################
## Builds the action buttons section.
##
## Creates a horizontal container with action buttons for common operations:
## - Clear: Clears the results display
## - Load .pl file: Opens a file dialog to load Prolog files
###############################################################################
func _build_action_buttons() -> void:
	var actions_container := HBoxContainer.new()

	# Button to clear the results output
	var clear_btn := Button.new()
	clear_btn.text = "Clear"
	clear_btn.pressed.connect(_on_clear_pressed)
	actions_container.add_child(clear_btn)

	# Button to load a Prolog file from disk
	var consult_file_btn := Button.new()
	consult_file_btn.text = "Load .pl file"
	consult_file_btn.pressed.connect(_on_consult_file_pressed)
	actions_container.add_child(consult_file_btn)

	add_child(actions_container)

###############################################################################
## Builds the quick code input section.
##
## Creates a multi-line text editor for writing Prolog code directly in the
## dock. The editor includes syntax highlighting for Prolog and a button to
## load the written code into the engine.
###############################################################################
func _build_code_section() -> void:
	var code_label := Label.new()
	code_label.text = "Quick Prolog Code:"
	add_child(code_label)

	# Multi-line text editor with Prolog syntax highlighting
	code_input = TextEdit.new()
	code_input.placeholder_text = "Write Prolog code here..."
	code_input.custom_minimum_size.y = 80
	code_input.syntax_highlighter = _create_prolog_highlighter()
	add_child(code_input)

	# Button to load the code from the text editor into Prolog
	var consult_string_btn := Button.new()
	consult_string_btn.text = "Load Code"
	consult_string_btn.pressed.connect(_on_consult_string_pressed)
	add_child(consult_string_btn)

###############################################################################
## Builds the predicates list section.
##
## Creates a list widget that displays all predicates currently loaded in the
## Prolog knowledge base. Includes a refresh button to update the list after
## loading new code or files.
###############################################################################
func _build_predicates_section() -> void:
	var pred_label := Label.new()
	pred_label.text = "Loaded Predicates:"
	add_child(pred_label)

	# List widget showing all available predicates
	predicates_list = ItemList.new()
	predicates_list.custom_minimum_size.y = 100
	predicates_list.size_flags_vertical = Control.SIZE_EXPAND_FILL
	add_child(predicates_list)

	# Button to refresh the predicates list
	var refresh_btn := Button.new()
	refresh_btn.text = "Refresh"
	refresh_btn.pressed.connect(_on_refresh_predicates)
	add_child(refresh_btn)

###############################################################################
## Event handler for query submission.
##
## Called when the user presses Enter in the query input field.
##
## @param query: The query string from the input field
###############################################################################
func _on_query_submitted(query: String) -> void:
	_execute_query(query)

###############################################################################
## Event handler for query button pressed.
##
## Called when the user clicks the Execute button. Reads the query from the
## input field and executes it.
###############################################################################
func _on_query_button_pressed() -> void:
	_execute_query(query_input.text)

###############################################################################
## Executes a Prolog query and displays the results.
##
## Executes the given Prolog query using the engine and formats the results
## in a readable way. Displays "false." if no solutions are found, or lists
## all solutions with their numbers if found.
##
## @param query: The Prolog query string to execute
###############################################################################
func _execute_query(query: String) -> void:
	# Skip empty queries
	if query.is_empty():
		return

	# Check if engine is available
	if not engine:
		_append_result("❌ Error: Prologot engine not available")
		return

	# Display the query in Prolog format
	_append_result("\n?- " + query)

	# Execute the query and get all solutions
	var results = engine.query_all(query)
	if results.is_empty():
		# No solutions found
		_append_result("false.")
	else:
		# Display each solution found
		for i in results.size():
			_append_result("  Solution %d: %s" % [i + 1, _format_result(results[i])])
		# Show summary
		_append_result("true. (%d solution(s))" % results.size())

###############################################################################
## Formats a Prolog result for display.
##
## Converts a Prolog result (which may be a compound term, list, atom, etc.)
## into a human-readable string representation. Handles recursive formatting
## for nested structures.
##
## @param result: The Prolog result to format (Variant type)
## @return: A formatted string representation of the result
###############################################################################
func _format_result(result) -> String:
	if result == null:
		return "null"
	if result is Dictionary:
		# Compound term: {"functor": "name", "args": [...]}
		# Format as functor(arg1, arg2, ...)
		if result.has("functor") and result.has("args"):
			var functor = str(result["functor"])
			var args_arr = result["args"]
			# Handle zero-argument terms (just show the functor)
			if args_arr.size() == 0:
				return functor
			# Recursively format each argument
			var args = []
			for arg in args_arr:
				args.append(_format_result(arg))
			return "%s(%s)" % [functor, ", ".join(args)]
		# Fallback for other dictionary types
		return str(result)
	if result is Array:
		# Prolog list: format as [elem1, elem2, ...]
		var items = []
		for item in result:
			items.append(_format_result(item))
		return "[%s]" % ", ".join(items)
	# For simple types (String, int, float, etc.), just convert to string
	return str(result)

###############################################################################
## Appends text to the result output.
##
## Adds a line of text to the results display and automatically scrolls to
## the bottom so the latest output is visible.
##
## @param text: The text to append to the results display
###############################################################################
func _append_result(text: String) -> void:
	result_output.text += text + "\n"
	# Auto-scroll to bottom to show latest output
	result_output.scroll_vertical = result_output.get_line_count()

###############################################################################
## Event handler for clear button pressed.
###############################################################################
func _on_clear_pressed() -> void:
	result_output.text = ""

###############################################################################
## Event handler for load file button pressed.
##
## Opens a file dialog allowing the user to select a Prolog file (.pl) to load.
## The dialog is centered and takes up 60% of the screen size.
###############################################################################
func _on_consult_file_pressed() -> void:
	var dialog := EditorFileDialog.new()
	dialog.file_mode = EditorFileDialog.FILE_MODE_OPEN_FILE
	# Filter to show only Prolog files
	dialog.add_filter("*.pl", "Prolog Files")
	dialog.file_selected.connect(_on_file_selected)
	add_child(dialog)
	# Show the dialog centered, 60% of screen size
	dialog.popup_centered_ratio(0.6)

###############################################################################
## Event handler for file selected.
##
## Called when the user selects a file from the file dialog. Attempts to load
## the Prolog file into the engine and updates the predicates list on success.
## Displays error messages if loading fails.
##
## @param path: The file path selected by the user
###############################################################################
func _on_file_selected(path: String) -> void:
	if not engine:
		_append_result("❌ Error: Prologot engine not available")
		return

	# Try to load the Prolog file
	if engine.consult_file(path):
		_append_result("✓ File loaded: " + path)
		# Refresh the predicates list to show newly loaded predicates
		_on_refresh_predicates()
	else:
		# Display error message
		_append_result("✗ Error loading: " + path)
		var err = engine.get_last_error()
		if not err.is_empty():
			_append_result("  → " + err)

###############################################################################
## Event handler for load code button pressed.
##
## Loads the Prolog code from the code input text area into the engine.
## Validates that code exists and that the engine is available before
## attempting to load. Updates the predicates list on success.
###############################################################################
func _on_consult_string_pressed() -> void:
	var code := code_input.text
	# Skip if no code was entered
	if code.is_empty():
		return

	if not engine:
		_append_result("❌ Error: Prologot engine not available")
		return

	# Load the code from the text area
	if engine.consult_string(code):
		_append_result("✓ Code loaded successfully")
		# Refresh predicates list to show newly loaded predicates
		_on_refresh_predicates()
	else:
		_append_result("✗ Error loading code")

###############################################################################
## Event handler for refresh predicates button pressed.
##
## Updates the predicates list by querying the engine for all currently
## loaded predicates. Clears the old list first, then populates it with
## the new list. This should be called after loading new Prolog code.
###############################################################################
func _on_refresh_predicates() -> void:
	# Clear the existing list
	predicates_list.clear()

	if not engine:
		return

	# Get all predicates from the engine and add them to the list
	var preds = engine.list_predicates()
	for pred in preds:
		predicates_list.add_item(str(pred))

###############################################################################
## Creates a basic syntax highlighter for Prolog code.
##
## Configures syntax highlighting rules for Prolog code in the text editor.
## Highlights keywords, comments, and strings with different colors to
## improve code readability.
##
## @return: A configured SyntaxHighlighter instance for Prolog
###############################################################################
func _create_prolog_highlighter() -> SyntaxHighlighter:
	var highlighter := CodeHighlighter.new()

	# Prolog keywords and operators (highlighted in coral/orange)
	highlighter.add_keyword_color(":-", Color.CORAL) # Rule definition
	highlighter.add_keyword_color("?-", Color.CORAL) # Query prompt
	highlighter.add_keyword_color("!", Color.CORAL) # Cut operator
	highlighter.add_keyword_color("true", Color.GREEN) # Success
	highlighter.add_keyword_color("false", Color.RED) # Failure
	highlighter.add_keyword_color("fail", Color.RED) # Explicit failure
	highlighter.add_keyword_color("is", Color.CORAL) # Arithmetic assignment
	highlighter.add_keyword_color("not", Color.CORAL) # Negation

	# Comments (highlighted in gray)
	highlighter.add_color_region("%", "", Color.GRAY, true) # Line comments (% ...)
	highlighter.add_color_region("/*", "*/", Color.GRAY) # Block comments (/* ... */)

	# Strings (highlighted in yellow)
	highlighter.add_color_region("\"", "\"", Color.YELLOW) # Double-quoted strings
	highlighter.add_color_region("'", "'", Color.YELLOW) # Single-quoted strings

	return highlighter
