extends Node2D

# =============================================================================
# UI References
# =============================================================================
@onready var alien_sprite = $AlienWindow/AlienSprite
@onready var alien_name_label = $AlienWindow/NameLabel
@onready var alien_status_bar = $AlienWindow/StatusBar/StatusLabel
@onready var passport_panel = $DocumentsArea/PassportPanel
@onready var cargo_panel = $DocumentsArea/CargoPanel
@onready var law_display = $LawTerminal/LawText
@onready var console_log = $Console/LogText
@onready var approve_btn = $ActionPanel/ApproveButton
@onready var arrest_btn = $ActionPanel/ArrestButton
@onready var scan_btn = $ActionPanel/ScanButton
@onready var score_label = $HUD/ScorePanel/ScoreLabel
@onready var day_label = $HUD/DayPanel/DayLabel
@onready var error_indicator = $ErrorIndicator
@onready var error_text = $ErrorIndicator/ErrorText

# =============================================================================
# Game state variables
# =============================================================================
var prolog: Prologot
var current_alien: Dictionary
var score: int = 100
var day: int = 1
var processing: bool = false
var aliens_processed: int = 0

# =============================================================================
# PROLOGOT TUTORIAL #1: UNDERSTANDING THE RULE SYSTEM
# -----------------------------------------------------------------------------
# This game uses Prolog rules loaded from demos/galactic_customs/rules/galactic_customs.pl
# to make decisions about aliens. The system has three main components:
#
# 1. galactic_customs.pl FILE (loaded in load_current_day())
#    The comprehensive Prolog knowledge base containing:
#    - valid_species/1: tentaculien, robotic, gooey, crystalline, gaseous
#    - planet/2: mars(inner), europa/titan/ganymede/io(outer)
#    - banned_substance/1: plutonium, spice_melange, antimatter, nanobots
#    - taxable_substance/2: ore(50), water(10), electronic_components(30), medications(20)
#    - suspect/1: checks banned cargo OR outer planet without visa
#    - threat_level/2: calculates critical/high/medium/low threat
#    - requires_quarantine/1: europa origin OR gaseous species
#    - has_record/1: checks criminal_record database
#    - calculate_total_tax/2: sums taxes for all cargo
#    And many more predicates for complex decision making!
#
# 2. ALIEN DATABASE (below)
#    Each alien entry contains facts that will be converted to Prolog facts
#    at runtime using add_fact(). The galactic_customs.pl predicates then evaluate these facts.
#
# 3. DAILY MISSIONS (further below)
#    Each day adds specific rules that use or override the base galactic_customs.pl predicates.
#    For example, Day 1 uses suspect(X) and threat_level(X, Level) from galactic_customs.pl.
#
# RULE SYNTAX EXAMPLES FROM galactic_customs.pl:
#   suspect(X) :- has_cargo(X, C), banned_substance(C).
#   "X is suspect if X has cargo C AND C is a banned substance"
#
#   threat_level(X, critical) :- has_tentacles(X), has_cargo(X, C), banned_substance(C).
#   "X has critical threat if X has tentacles AND banned cargo"
#
# The game queries these predicates using query(), call_predicate(), etc.
# =============================================================================

# Alien database with all possible passengers
# NOTE: Now uses substances from galactic_customs.pl:
# - Banned: plutonium, spice_melange, antimatter, nanobots
# - Taxable: ore (50), water (10), electronic_components (30), medications (20)
var alien_database = [
	{
		"name": "zorglub",
		"species": "tentaculien",
		"origin": "mars",
		"has_visa": true,
		"has_tentacles": true,
		"has_permit": false,
		"cargo": ["water", "ore"],
		"sprite_index": 0
	},
	{
		"name": "bleep",
		"species": "robotic",
		"origin": "europa",
		"has_visa": true,
		"has_tentacles": false,
		"has_permit": true,
		"cargo": ["electronic_components"],
		"sprite_index": 1
	},
	{
		"name": "glorp",
		"species": "gooey",
		"origin": "titan",
		"has_visa": false,
		"has_tentacles": true,
		"has_permit": true,
		"cargo": ["plutonium", "medications"],
		"sprite_index": 2
	},
	{
		"name": "xylox",
		"species": "crystalline",
		"origin": "ganymede",
		"has_visa": true,
		"has_tentacles": false,
		"has_permit": false,
		"cargo": ["water", "medications"],
		"sprite_index": 3
	},
	{
		"name": "nebula",
		"species": "gaseous",
		"origin": "io",
		"has_visa": true,
		"has_tentacles": false,
		"has_permit": true,
		"cargo": ["spice_melange"], # spice_melange is banned!
		"sprite_index": 4
	}
]

# Each alien entry represents a passenger with:
# - name: unique identifier (lowercase in Prolog facts)
# - species: one of valid_species from galactic_customs.pl (tentaculien, robotic, gooey, crystalline, gaseous)
# - origin: planet from galactic_customs.pl (mars=inner, europa/titan/ganymede/io=outer)
# - has_visa: boolean indicating legal entry permission
# - has_tentacles: physical attribute used in threat_level calculation
# - has_permit: slime permit for tentacled aliens (affects threat_level)
# - cargo: items from galactic_customs.pl (banned: plutonium/spice_melange/antimatter/nanobots,
#          taxable: ore/water/electronic_components/medications)
# - sprite_index: visual representation index
# These properties are converted to Prolog facts when scanning an alien.
# galactic_customs.pl then evaluates: suspect(), threat_level(), requires_quarantine(), etc.

# =============================================================================
# DAILY MISSIONS - RULE-BASED DECISION MAKING
# -----------------------------------------------------------------------------
# Each mission defines the day's objective and the Prolog rules
# that implement it. Rules use Prolog syntax:
#
# - Predicates: authorize(X), dangerous(X), taxable(X)
# - Variables: X represents an alien name
# - Negation: \+ means "not" (e.g., \+ has_visa(X))
# - Implication: :- means "if" (left side true if right side true)
# - Conjunction: comma (,) means "and"
#
# EXAMPLE RULE BREAKDOWN:
#   authorize(X) :- has_visa(X), \+ has_cargo(X, forbidden_substance).
#   Reads as: "X is authorized IF X has a visa AND X does NOT have forbidden_substance"
#
# The rules are loaded into the Prolog engine each day and evaluated
# when you query predicates like authorize(name) or dangerous(name).
# ================================================================
var daily_missions = [
	{
		"mission": "Check all visas and refuse anyone carrying banned substances",
		"rules": """
% Day 1: Basic authorization using galactic_customs.pl predicates
% galactic_customs.pl already defines suspect(X) and banned_substance/1
authorize(X) :- has_visa(X), \\+ suspect(X).
dangerous(X) :- threat_level(X, critical).
dangerous(X) :- threat_level(X, high).
"""
	},
	{
		"mission": "All tentacled aliens are dangerous. Mars origin is prohibited",
		"rules": """
% Day 2: Stricter rules on tentacles
% Override threat_level rules for this day
authorize(X) :- has_visa(X), \\+ dangerous(X), \\+ origin(X, mars).
dangerous(X) :- has_tentacles(X).
prohibited(X) :- origin(X, mars).
"""
	},
	{
		"mission": "Europa quarantine in effect. Strict criminal checks",
		"rules": """
% Day 3: Use comprehensive rules from galactic_customs.pl
% galactic_customs.pl defines requires_quarantine(X), has_record(X), etc.
authorize(X) :- has_visa(X), \\+ requires_quarantine(X), \\+ has_record(X), \\+ suspect(X).
need_scan(X) :- requires_quarantine(X).
dangerous(X) :- has_record(X).
dangerous(X) :- threat_level(X, critical).
"""
	}
]

# Mission 1: Basic visa and cargo check
#   - Uses suspect(X) from galactic_customs.pl (checks banned substances + outer planets)
#   - Uses threat_level(X, Level) for danger assessment
#   - Authorize only if has visa and not suspect
#
# Mission 2: Stricter tentacle and origin rules
#   - All tentacled aliens are dangerous
#   - Mars origin is prohibited
#   - Only authorize if not dangerous and not from Mars
#
# Mission 3: Comprehensive security using galactic_customs.pl
#   - Uses requires_quarantine(X) - from Europa or gaseous species
#   - Uses has_record(X) - criminal background checks
#   - Uses suspect(X) - banned cargo or outer planet without visa
#   - Combines multiple predicates from galactic_customs.pl for complete security

# =============================================================================
# RULES FILE - COMPREHENSIVE PROLOG KNOWLEDGE BASE
# -----------------------------------------------------------------------------
# The game now loads rules directly from: demos/galactic_customs/rules/galactic_customs.pl
#
# This file contains the complete rule system including:
# - valid_species/1: All recognized alien species
# - planet/2: Planets and their classifications (inner/outer)
# - banned_substance/1: Illegal substances (plutonium, antimatter, etc.)
# - taxable_substance/2: Items requiring tax payment with amounts
# - suspect/1: Determines if an alien is suspicious
# - threat_level/2: Calculates threat levels (critical/high/medium/low)
# - requires_quarantine/1: Quarantine requirements
# - has_record/1: Criminal background checks
# - calculate_total_tax/2: Tax calculation for all cargo
# - decision_authorized/1, decision_refused/1: Complex decision logic
# - diplomatic_immunity/1: Special diplomatic status
# - And many more...
#
# The galactic_customs.pl file is loaded in load_current_day() using:
#   prolog.consult_file("res://rules/galactic_customs.pl")
#
# Daily mission rules are then added on top to modify behavior per day.
# =============================================================================

# =============================================================================
# Main entry point for the game, called automatically on startup
# =============================================================================
func _ready():
	init_prolog()
	init_ui()
	load_current_day()
	load_next_alien()
	update_hud()

func _exit_tree():
	cleanup_prolog()

# =============================================================================
# PROLOGOT TUTORIAL #1: INITIALIZE THE PROLOG ENGINE
# -----------------------------------------------------------------------------
# Demonstrates how to create and initialize the Prolog engine.
# Steps:
# 1. Create a new instance of Prologot.
# 2. Call initialize() to start the engine.
# 3. Verify with is_initialized().
# =============================================================================
func init_prolog():
	log_message("=== GALACTIC SYSTEM INITIALIZATION ===")

	# Create the Prolog engine instance
	prolog = Prologot.new()

	# Initialize the engine
	prolog.initialize({"home": "res://bin/swipl"})
	 	push_error("Failed to initialize Prologot: " + prolog.get_last_error())
 	return

	# Check if initialization was successful
	if prolog.is_initialized():
		log_message("[OK] Prolog engine initialized")
	else:
		log_message("[ERROR] Failed to initialize Prolog engine")

# =============================================================================
# PROLOGOT TUTORIAL #2: CLEANUP - CLEAN RESOURCES
# -----------------------------------------------------------------------------
# Always call cleanup() before destroying or exiting Prologot!
# Frees memory, closes native resources, prevents leaks.
# After cleanup(), call initialize() before next use.
# =============================================================================
func cleanup_prolog():
	if prolog and prolog.is_initialized():
		prolog.cleanup()
		log_message("[SYSTEM] Prolog cleanup done")

# =============================================================================
# Connect UI button signals to their handlers.
# -----------------------------------------------------------------------------
# Note: NinePatchRect uses gui_input instead of pressed.
# =============================================================================
func init_ui():
	approve_btn.gui_input.connect(_on_approve_gui_input)
	arrest_btn.gui_input.connect(_on_arrest_gui_input)
	scan_btn.gui_input.connect(_on_scan_gui_input)

# =============================================================================
# PROLOGOT TUTORIAL #3: LOADING PROLOG CODE FROM FILE
# -----------------------------------------------------------------------------
# Demonstrates two ways to load Prolog rules:
# 1. consult_file() - Load complete .pl files from disk
# 2. consult_string() - Load code strings for dynamic/daily rules
#
# Steps:
# 1. Clean up previous state
# 2. Re-initialize
# 3. Load comprehensive rules from galactic_customs.pl file
# 4. Load daily mission rules on top
#
# The galactic_customs.pl file contains the base knowledge base (facts about species,
# planets, substances, threat levels, etc.). Daily mission rules add or
# override specific predicates for each day's objectives.
# =============================================================================
func load_current_day():
	if day > daily_missions.size():
		log_message("[GAME] All missions completed!")
		return

	# Clean up previous engine state
	prolog.cleanup()

	# Re-initialize for a fresh state
	prolog.initialize({"home": "res://bin/swipl"})

	# Load comprehensive rules from the galactic_customs.pl file
	var rules_file_path = "res://rules/galactic_customs.pl"
	if not prolog.consult_file(rules_file_path):
		log_message("[ERROR] Failed to load galactic_customs.pl!")
		log_message("[ERROR] Game cannot function without base rules!")
		return
	else:
		log_message("[RULES] Loaded comprehensive rules from galactic_customs.pl")

	# Load daily mission rules (adds or overrides rules for the day)
	var daily_rules = daily_missions[day - 1].rules
	if not prolog.consult_string(daily_rules):
		log_message("[ERROR] Failed to load daily mission rules!")
	else:
		log_message("[MISSION] Day %d rules loaded" % day)

	display_mission()
	log_message("[LAW] Regulations loaded for day " + str(day))

# =============================================================================
# Display the day's mission in human language as well as rules
# -----------------------------------------------------------------------------
# Shows a clear description of the objective, then technical rules.
# ================================================================
func display_mission():
	var mission = daily_missions[day - 1]

	law_display.text = "[b]=== DAY " + str(day) + " MISSION ===[/b]\n\n"
	law_display.text += "[color=yellow]" + mission.mission + "[/color]\n\n"
	law_display.text += "[color=cyan][b]Technical rules:[/b][/color]\n"
	law_display.text += mission.rules

# =============================================================================
# Load a new random alien
# -----------------------------------------------------------------------------
# Handles the full cycle:
# 1. Clear previous alien facts
# 2. Pick a random alien
# 3. Show alien and their documents
# =============================================================================
func load_next_alien():
	if processing:
		return

	clear_alien_facts()
	current_alien = alien_database[randi() % alien_database.size()]

	display_alien()
	update_documents()
	log_message("\n[NEW] Incoming ship: " + current_alien.name)

# =============================================================================
# PROLOGOT TUTORIAL #4: RETRACT_ALL - CLEAR FACTS
# -----------------------------------------------------------------------------
# retract_all(predicate) clears ALL facts matching a predicate.
# For example, retract_all("has_visa") deletes has_visa for all aliens.
# This is essential between aliens to avoid fact buildup.
# =============================================================================
func clear_alien_facts():
	for fact in ["passenger", "has_visa", "has_tentacles", "has_slime_permit", "origin", "has_cargo"]:
		prolog.retract_all(fact)

# =============================================================================
# Visually displays the current alien
# -----------------------------------------------------------------------------
# Uses set_character() to update the character's sprite.
# =============================================================================
func display_alien():
	alien_name_label.text = current_alien.name
	alien_status_bar.text = "⚠ AWAITING SCAN ⚠"
	alien_status_bar.add_theme_color_override("font_color", Color(1, 0.7, 0.2))
	alien_sprite.set_character(current_alien.name)
	alien_sprite.set_modulate_color(Color.WHITE)

# =============================================================================
# Set alien sprite color according to status.
# Wrapper for different sprite implementations.
# set_modulate_color() should be on the alien sprite.
# =============================================================================
func set_alien_color(color: Color):
	alien_sprite.set_modulate_color(color)

# =============================================================================
# Show this alien's documents (passport & cargo)
# -----------------------------------------------------------------------------
# Updates interface labels with the alien's info: name, origin, visa, cargo list.
# =============================================================================
func update_documents():
	passport_panel.get_node("NameValue").text = current_alien.name
	passport_panel.get_node("OriginValue").text = current_alien.origin
	passport_panel.get_node("VisaValue").text = "YES" if current_alien.has_visa else "NO"

	var cargo_text = "Cargo:\n"
	for item in current_alien.cargo:
		cargo_text += "- " + item + "\n"
	cargo_panel.get_node("CargoList").text = cargo_text

# =============================================================================
# Scan the alien: demonstrates full Prologot functionality with galactic_customs.pl
# -----------------------------------------------------------------------------
# Orchestrates:
# 1. Add dynamic facts (passenger, visa, cargo, etc.)
# 2. Query galactic_customs.pl predicates: suspect(), threat_level(), requires_quarantine()
# 3. Check taxes using calculate_total_tax() from galactic_customs.pl
# 4. Check criminal records using has_record() from galactic_customs.pl
# 5. Multi-solution queries for cargo
# =============================================================================
func scan_alien():
	log_message("[SCAN] Analysis in progress...")

	add_alien_facts()
	check_alien_status()
	check_advanced_rules()
	check_taxes()
	list_cargo()

# =============================================================================
# PROLOGOT TUTORIAL #5: ADD_FACT - ADD DYNAMIC FACTS
# -----------------------------------------------------------------------------
# add_fact("pred(arg1, arg2)") adds a Prolog fact at runtime.
# Use to_lower() for names for Prolog atom case consistency.
# =============================================================================
func add_alien_facts():
	var alien_name = current_alien.name.to_lower()

	prolog.add_fact("passenger(%s, %s)" % [alien_name, current_alien.species])
	log_message("  Added: passenger(%s, %s)" % [alien_name, current_alien.species])

	if current_alien.has_visa:
		prolog.add_fact("has_visa(%s)" % alien_name)
		log_message("  Added: has_visa(%s)" % alien_name)

	if current_alien.has_tentacles:
		prolog.add_fact("has_tentacles(%s)" % alien_name)
		log_message("  Added: has_tentacles(%s)" % alien_name)

	if current_alien.has_permit:
		prolog.add_fact("has_slime_permit(%s)" % alien_name)
		log_message("  Added: has_slime_permit(%s)" % alien_name)

	prolog.add_fact("origin(%s, %s)" % [alien_name, current_alien.origin])
	log_message("  Added: origin(%s, %s)" % [alien_name, current_alien.origin])

	for cargo_item in current_alien.cargo:
		prolog.add_fact("has_cargo(%s, %s)" % [alien_name, cargo_item])
		log_message("  Added: has_cargo(%s, %s)" % [alien_name, cargo_item])

# =============================================================================
# PROLOGOT TUTORIAL #6: QUERY - BOOLEAN QUERY
# -----------------------------------------------------------------------------
# query("predicate(name)") returns true or false if proven.
# Used here to determine if the alien is dangerous.
# =============================================================================
func check_alien_status():
	var alien_name = current_alien.name.to_lower()
	var is_dangerous = prolog.query("dangerous(%s)" % alien_name)

	if is_dangerous:
		set_alien_color(Color.RED)
		alien_status_bar.text = "⚠ DANGEROUS ENTITY ⚠"
		alien_status_bar.add_theme_color_override("font_color", Color(1, 0.3, 0.3))
		log_message("  [ALERT] Alien is dangerous!")
	else:
		set_alien_color(Color.GREEN)
		alien_status_bar.text = "✓ SAFE TO PROCESS ✓"
		alien_status_bar.add_theme_color_override("font_color", Color(0.3, 1, 0.4))
		log_message("  [OK] Alien is safe")

# =============================================================================
# NEW: Check advanced rules from galactic_customs.pl
# -----------------------------------------------------------------------------
# Demonstrates querying the comprehensive predicates from galactic_customs.pl:
# - suspect/1: banned cargo or outer planet without visa
# - threat_level/2: critical/high/medium/low assessment
# - requires_quarantine/1: europa or gaseous species
# - has_record/1: criminal background check
# =============================================================================
func check_advanced_rules():
	var alien_name = current_alien.name.to_lower()

	# Check if suspect (from galactic_customs.pl)
	if prolog.query("suspect(%s)" % alien_name):
		log_message("  [WARNING] Alien is SUSPECT (banned cargo or outer planet without visa)")

	# Check threat level (from galactic_customs.pl)
	var threat_levels = ["critical", "high", "medium", "low"]
	for level in threat_levels:
		if prolog.query("threat_level(%s, %s)" % [alien_name, level]):
			log_message("  [THREAT] Threat level: %s" % level.to_upper())
			break

	# Check quarantine requirement (from galactic_customs.pl)
	if prolog.query("requires_quarantine(%s)" % alien_name):
		log_message("  [QUARANTINE] Quarantine required (Europa origin or gaseous species)")

	# Check criminal record (from galactic_customs.pl)
	if prolog.query("has_record(%s)" % alien_name):
		log_message("  [CRIMINAL] Has criminal record!")

# =============================================================================
# PROLOGOT TUTORIAL #7: PREDICATE_EXISTS and CALL_FUNCTION with galactic_customs.pl
# -----------------------------------------------------------------------------
# - predicate_exists(name, arity): does a predicate exist?
# - call_function(name, [args]): calls a predicate and returns a value
# Pattern: check existence before calling to avoid errors.
#
# Now uses calculate_total_tax/2 from galactic_customs.pl which sums all taxable cargo.
# =============================================================================
func check_taxes():
	var alien_name = current_alien.name.to_lower()

	# Try the comprehensive tax calculation from galactic_customs.pl
	if prolog.predicate_exists("calculate_total_tax", 2):
		var total_tax = prolog.call_function("calculate_total_tax", [alien_name])
		if total_tax != null and total_tax > 0:
			log_message("  [TAX] Total amount: %s credits" % str(total_tax))

	# Fallback to simple calculate_tax if defined in daily rules
	elif prolog.predicate_exists("calculate_tax", 2):
		var tax = prolog.call_function("calculate_tax", [alien_name])
		if tax != null:
			log_message("  [TAX] Amount: %s credits" % str(tax))

# =============================================================================
# PROLOGOT TUTORIAL #8: QUERY_ALL - ALL SOLUTIONS
# -----------------------------------------------------------------------------
# query_all("has_cargo(name, C)") returns all matching cargo items.
# Each result is a dictionary of variable bindings.
# Used here to count cargo items.
# =============================================================================
func list_cargo():
	var alien_name = current_alien.name.to_lower()
	var all_cargo = prolog.query_all("has_cargo(%s, C)" % alien_name)
	log_message("  [CARGO] Items detected: %d" % all_cargo.size())

# =============================================================================
# PROLOGOT TUTORIAL #9: CALL_PREDICATE - CHECK A PREDICATE
# -----------------------------------------------------------------------------
# call_predicate(name, [args]) is like query() but more flexible.
# Returns true/false depending on Prolog rules.
# Used to verify if the alien is authorized. Result determines
# whether the player's choice is correct.
# =============================================================================
func make_decision(is_approve: bool):
	if processing:
		return
	processing = true

	var alien_name = current_alien.name.to_lower()
	var is_authorized = prolog.call_predicate("authorize", [alien_name])
	var is_dangerous = prolog.query("dangerous(%s)" % alien_name)

	if is_approve:
		handle_approval(is_authorized)
	else:
		handle_arrest(is_authorized, is_dangerous)

	await get_tree().create_timer(1.5).timeout
	processing = false

	# Increment aliens processed counter
	aliens_processed += 1

	# Check if we should advance to next day (after 5 aliens)
	if aliens_processed >= 5:
		advance_to_next_day()
	else:
		load_next_alien()

	update_hud()

# =============================================================================
# Handle approval logic
# -----------------------------------------------------------------------------
# Compares player approve action to Prolog's authorization.
# If authorized & approve: correct (+10); otherwise: error (-20)
# =============================================================================
func handle_approval(is_authorized: bool):
	if is_authorized:
		log_message("[SUCCESS] Correct decision! +10 points")
		score += 10
		set_alien_color(Color.GREEN)
		flash_success()
	else:
		log_message("[ERROR] You let a criminal through! -20 points")
		score -= 20
		set_alien_color(Color.RED)
		flash_error()

# =============================================================================
# Handle arrest logic
# -----------------------------------------------------------------------------
# Arrest is justified if:
# - Not authorized or
# - Alien is dangerous
# Allows arrest of dangerous aliens even if their papers are in order.
# =============================================================================
func handle_arrest(is_authorized: bool, is_dangerous: bool):
	if not is_authorized or is_dangerous:
		log_message("[SUCCESS] Arrest justified! +15 points")
		score += 15
		set_alien_color(Color.ORANGE)
	else:
		log_message("[ERROR] Unjustified arrest! -25 points")
		score -= 25
		set_alien_color(Color.RED)
		flash_error()

# =============================================================================
# Show a red error indicator for 0.5s if the player makes a mistake
# =============================================================================
func flash_error():
	error_indicator.visible = true
	error_indicator.color = Color(1, 0, 0, 0.4) # Red background for errors
	error_text.text = "⚠ ERROR ⚠"
	await get_tree().create_timer(0.5).timeout
	error_indicator.visible = false

# =============================================================================
# Show a green success indicator for 0.5s when the player makes a correct decision
# =============================================================================
func flash_success():
	error_indicator.visible = true
	error_indicator.color = Color(0, 1, 0, 0.4) # Green background for success
	error_text.text = "✓ SUCCESS ✓"
	await get_tree().create_timer(0.5).timeout
	error_indicator.visible = false

# =============================================================================
# Show permanent game over message when score reaches 0
# =============================================================================
func show_game_over():
	error_indicator.visible = true
	error_indicator.color = Color(1, 0, 0, 0.4) # Red background for game over
	error_text.text = "⚠ GAME OVER ⚠\nYOU ARE FIRED!"

# =============================================================================
# Advance to the next day after processing 5 aliens
# =============================================================================
func advance_to_next_day():
	aliens_processed = 0

	# Check if we've completed all days
	if day >= daily_missions.size():
		show_victory()
		get_tree().paused = true
		return

	# Move to next day
	day += 1
	log_message("\n[DAY %d] Starting new day..." % day)
	load_current_day()
	load_next_alien()

# =============================================================================
# Show permanent victory message when all days are completed
# =============================================================================
func show_victory():
	error_indicator.visible = true
	error_indicator.color = Color(0, 1, 0, 0.4) # Green background for victory
	error_text.text = "✓ VICTORY ✓\nALL DAYS COMPLETED!"
	log_message("\n[VICTORY] Congratulations! You completed all missions!")

# =============================================================================
# Add a message to the console log area.
# Automatically scrolls to latest line.
# Note: scroll_to_line() requires scroll_following to be enabled.
# =============================================================================
func log_message(msg: String):
	console_log.text += msg + "\n"
	if console_log.get_line_count() > 0:
		console_log.scroll_to_line(console_log.get_line_count() - 1)

# =============================================================================
# Update HUD display for score and day.
# Ends game and pauses if score <= 0.
# =============================================================================
func update_hud():
	score_label.text = str(score)
	day_label.text = str(day)

	if score <= 0:
		log_message("\n[GAME OVER] You are fired!")
		get_tree().paused = true
		show_game_over()

# =============================================================================
# GUI event handler for Scan button
# -----------------------------------------------------------------------------
# Converts GUI input into a function call.
# =============================================================================
func _on_scan_gui_input(event: InputEvent):
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		scan_alien()

# =============================================================================
# GUI handler for Approve button
# -----------------------------------------------------------------------------
# Converts GUI input into a function call.
# =============================================================================
func _on_approve_gui_input(event: InputEvent):
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		make_decision(true)

# =============================================================================
# GUI handler for Arrest button
# -----------------------------------------------------------------------------
# Converts GUI input into a function call.
# =============================================================================
func _on_arrest_gui_input(event: InputEvent):
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		make_decision(false)
