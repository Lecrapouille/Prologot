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
var query_input: LineEdit

## Text area displaying query results.
var result_output: TextEdit

## List showing loaded predicates.
var predicates_list: ItemList

## Text area for entering Prolog code.
var code_input: TextEdit

## Prologot engine instance (set by the plugin).
var engine = null


func _ready() -> void:
	name = "Prologot Console"
	_build_ui()


## Builds the dock UI components.
func _build_ui() -> void:
	# Title
	var title := Label.new()
	title.text = "Prologot Console"
	title.add_theme_font_size_override("font_size", 16)
	add_child(title)

	add_child(HSeparator.new())

	# Query section
	_build_query_section()

	# Result section
	_build_result_section()

	# Action buttons
	_build_action_buttons()

	add_child(HSeparator.new())

	# Quick code section
	_build_code_section()

	add_child(HSeparator.new())

	# Predicates list section
	_build_predicates_section()


## Builds the query input section.
func _build_query_section() -> void:
	var query_container := HBoxContainer.new()

	var query_label := Label.new()
	query_label.text = "Query:"
	query_label.custom_minimum_size.x = 60
	query_container.add_child(query_label)

	query_input = LineEdit.new()
	query_input.placeholder_text = "e.g., parent(X, bob)"
	query_input.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	query_input.text_submitted.connect(_on_query_submitted)
	query_container.add_child(query_input)

	var query_btn := Button.new()
	query_btn.text = "Execute"
	query_btn.pressed.connect(_on_query_button_pressed)
	query_container.add_child(query_btn)

	add_child(query_container)


## Builds the result display section.
func _build_result_section() -> void:
	var result_label := Label.new()
	result_label.text = "Results:"
	add_child(result_label)

	result_output = TextEdit.new()
	result_output.editable = false
	result_output.custom_minimum_size.y = 100
	result_output.size_flags_vertical = Control.SIZE_EXPAND_FILL
	add_child(result_output)


## Builds the action buttons section.
func _build_action_buttons() -> void:
	var actions_container := HBoxContainer.new()

	var clear_btn := Button.new()
	clear_btn.text = "Clear"
	clear_btn.pressed.connect(_on_clear_pressed)
	actions_container.add_child(clear_btn)

	var load_file_btn := Button.new()
	load_file_btn.text = "Load .pl file"
	load_file_btn.pressed.connect(_on_load_file_pressed)
	actions_container.add_child(load_file_btn)

	add_child(actions_container)


## Builds the quick code input section.
func _build_code_section() -> void:
	var code_label := Label.new()
	code_label.text = "Quick Prolog Code:"
	add_child(code_label)

	code_input = TextEdit.new()
	code_input.placeholder_text = "Write Prolog code here..."
	code_input.custom_minimum_size.y = 80
	code_input.syntax_highlighter = _create_prolog_highlighter()
	add_child(code_input)

	var load_code_btn := Button.new()
	load_code_btn.text = "Load Code"
	load_code_btn.pressed.connect(_on_load_code_pressed)
	add_child(load_code_btn)


## Builds the predicates list section.
func _build_predicates_section() -> void:
	var pred_label := Label.new()
	pred_label.text = "Loaded Predicates:"
	add_child(pred_label)

	predicates_list = ItemList.new()
	predicates_list.custom_minimum_size.y = 100
	predicates_list.size_flags_vertical = Control.SIZE_EXPAND_FILL
	add_child(predicates_list)

	var refresh_btn := Button.new()
	refresh_btn.text = "Refresh"
	refresh_btn.pressed.connect(_on_refresh_predicates)
	add_child(refresh_btn)


# =============================================================================
# Event Handlers
# =============================================================================

func _on_query_submitted(query: String) -> void:
	_execute_query(query)


func _on_query_button_pressed() -> void:
	_execute_query(query_input.text)


## Executes a Prolog query and displays the results.
func _execute_query(query: String) -> void:
	if query.is_empty():
		return

	if not engine:
		_append_result("❌ Error: Prologot engine not available")
		return

	_append_result("\n?- " + query)

	var results = engine.query_all(query)
	if results.is_empty():
		_append_result("false.")
	else:
		for i in results.size():
			_append_result("  Solution %d: %s" % [i + 1, _format_result(results[i])])
		_append_result("true. (%d solution(s))" % results.size())


## Formats a Prolog result for display.
func _format_result(result) -> String:
	if result == null:
		return "null"
	if result is Dictionary:
		# Compound term: {"functor": "name", "args": [...]}
		if result.has("functor") and result.has("args"):
			var functor = str(result["functor"])
			var args_arr = result["args"]
			if args_arr.size() == 0:
				return functor
			var args = []
			for arg in args_arr:
				args.append(_format_result(arg))
			return "%s(%s)" % [functor, ", ".join(args)]
		return str(result)
	if result is Array:
		# Prolog list: [elem1, elem2, ...]
		var items = []
		for item in result:
			items.append(_format_result(item))
		return "[%s]" % ", ".join(items)
	return str(result)


## Appends text to the result output.
func _append_result(text: String) -> void:
	result_output.text += text + "\n"
	result_output.scroll_vertical = result_output.get_line_count()


func _on_clear_pressed() -> void:
	result_output.text = ""


func _on_load_file_pressed() -> void:
	var dialog := EditorFileDialog.new()
	dialog.file_mode = EditorFileDialog.FILE_MODE_OPEN_FILE
	dialog.add_filter("*.pl", "Prolog Files")
	dialog.file_selected.connect(_on_file_selected)
	add_child(dialog)
	dialog.popup_centered_ratio(0.6)


func _on_file_selected(path: String) -> void:
	if not engine:
		_append_result("❌ Error: Prologot engine not available")
		return

	if engine.consult(path):
		_append_result("✓ File loaded: " + path)
		_on_refresh_predicates()
	else:
		_append_result("✗ Error loading: " + path)
		var err = engine.get_last_error()
		if not err.is_empty():
			_append_result("  → " + err)


func _on_load_code_pressed() -> void:
	var code := code_input.text
	if code.is_empty():
		return

	if not engine:
		_append_result("❌ Error: Prologot engine not available")
		return

	if engine.consult_string(code):
		_append_result("✓ Code loaded successfully")
		_on_refresh_predicates()
	else:
		_append_result("✗ Error loading code")


func _on_refresh_predicates() -> void:
	predicates_list.clear()

	if not engine:
		return

	var preds = engine.list_predicates()
	for pred in preds:
		predicates_list.add_item(str(pred))


# =============================================================================
# Helper Functions
# =============================================================================

## Creates a basic syntax highlighter for Prolog code.
func _create_prolog_highlighter() -> SyntaxHighlighter:
	var highlighter := CodeHighlighter.new()

	# Keywords
	highlighter.add_keyword_color(":-", Color.CORAL)
	highlighter.add_keyword_color("?-", Color.CORAL)
	highlighter.add_keyword_color("!", Color.CORAL)
	highlighter.add_keyword_color("true", Color.GREEN)
	highlighter.add_keyword_color("false", Color.RED)
	highlighter.add_keyword_color("fail", Color.RED)
	highlighter.add_keyword_color("is", Color.CORAL)
	highlighter.add_keyword_color("not", Color.CORAL)

	# Comments
	highlighter.add_color_region("%", "", Color.GRAY, true)
	highlighter.add_color_region("/*", "*/", Color.GRAY)

	# Strings
	highlighter.add_color_region("\"", "\"", Color.YELLOW)
	highlighter.add_color_region("'", "'", Color.YELLOW)

	return highlighter
