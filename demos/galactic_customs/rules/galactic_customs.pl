# =============================================================================
% PROLOGOT TUTORIAL: COMPREHENSIVE PROLOG RULES FILE
# =============================================================================
% This file contains the complete Prolog knowledge base for the Galactic Customs
% game. It demonstrates various Prolog concepts and patterns used in rule-based
% systems.
%
% FILE OVERVIEW:
% --------------
% This rules file is referenced by main.gd and contains:
% - Base facts (species, planets, substances)
% - Analysis rules (suspect detection, tax calculation)
% - Security rules (threat levels, quarantine requirements)
% - Decision logic (authorization, conditional entry, refusals)
% - Temporal rules (day-based restrictions)
% - Background checks (criminal records)
%
% HOW THESE RULES ARE USED:
% --------------------------------------------------------------------------------
% This file is automatically loaded by main.gd in the load_current_day() function using:
%   prolog.consult_file("res://demos/galactic_customs/rules/rules.pl")
%
% It is loaded FIRST, providing the base knowledge base. Then daily mission rules
% are loaded on top using prolog.consult_string(), which can add or override predicates.
%
% The game queries these predicates (suspect/1, threat_level/2, etc.) to make
% decisions about alien entries, taxes, threats, and authorization.
%
% PROLOG SYNTAX QUICK REFERENCE:
% -------------------------------------------------------------------------------------
% Facts:      predicate(argument).
% Rules:      predicate(X) :- condition1, condition2.
% Negation:   \+ condition        (means "NOT condition")
% Conjunction: condition1, condition2  (means "AND")
% Disjunction: multiple rules with same head  (means "OR")
% Variables:  Capitalized identifiers (X, Y, C, etc.)
% Anonymous:  _  (matches anything, value is ignored)
%
% For more examples and usage, see the tutorial comments throughout this file.
# =============================================================================

# =============================================================================
% PROLOG TUTORIAL: PERMANENT RULES - GALACTIC BASE
# =============================================================================
% This section defines the foundational facts and rules of the game world.
%
% PROLOG BASICS - FACTS:
% ---------------------
% Facts are statements that are always true. They are written as:
%   predicate_name(argument1, argument2, ...).
% The period (.) at the end is required and indicates the end of the fact.
%
% Examples:
%   valid_species(tentaculien).  % States that 'tentaculien' is a valid species
%   planet(mars, inner).         % States that 'mars' is an 'inner' planet
%
% Facts can be queried later to check if they are true. For example:
%   ?- valid_species(tentaculien).  % Returns: true
%   ?- valid_species(human).        % Returns: false (not in database)
# =============================================================================

% Definition of valid species
% These are facts that list all species types recognized by the customs system.
% Each fact has the form: valid_species(SpeciesName).
valid_species(tentaculien).
valid_species(robotic).
valid_species(gooey).
valid_species(crystalline).
valid_species(gaseous).

% Planets of the system
% These facts define planets and their classification (inner or outer zone).
% Format: planet(PlanetName, ZoneClassification).
% Outer planets typically require stricter entry conditions (see suspect/1 rule below).
planet(mars, inner).
planet(europa, outer).
planet(titan, outer).
planet(ganymede, outer).
planet(io, outer).

% Restricted substances
% These are illegal items that automatically make an alien a suspect.
% Any alien carrying these substances should be refused entry.
% Format: banned_substance(SubstanceName).
banned_substance(plutonium).
banned_substance(spice_melange).
banned_substance(antimatter).
banned_substance(nanobots).

% Taxable substances
% These facts define items that require tax payment with their tax amounts.
% Format: taxable_substance(SubstanceName, TaxAmount).
% Used by calculate_total_tax/2 to sum up taxes for all cargo items.
taxable_substance(ore, 50).
taxable_substance(water, 10).
taxable_substance(electronic_components, 30).
taxable_substance(medications, 20).

# =============================================================================
% PROLOG TUTORIAL: DIPLOMATIC RULES
# =============================================================================
% PROLOG BASICS - RULES:
% -----------------------------------------------------------------------------
% Rules are conditional statements. They define when a predicate is true.
% Format: Head :- Body.
%   - Head: The conclusion (what we're proving)
%   - :-   : Means "if" or "is true when"
%   - Body: The conditions (what must be true)
%
% Example:
%   diplomatic_immunity(X) :- rank(X, ambassador).
%   Reads as: "X has diplomatic immunity IF X has rank ambassador"
%
% Multiple rules with the same head mean "OR" - the predicate is true if
% ANY of the rules' bodies are satisfied. Here, diplomatic immunity applies
% if the alien is an ambassador OR a consul.
# =============================================================================

% Diplomatic immunity (for 'federation_rules' knowledge base)
% Rule 1: Ambassadors have diplomatic immunity
diplomatic_immunity(X) :-
	rank(X, ambassador).

% Rule 2: Consuls also have diplomatic immunity
diplomatic_immunity(X) :-
	rank(X, consul).

# =============================================================================
% PROLOG TUTORIAL: ANALYSIS RULES
# =============================================================================
% PROLOG BASICS - CONJUNCTION AND NEGATION:
% -----------------------------------------------------------------------------
% - Comma (,): Means "AND" - all conditions must be true
% - \+ : Means "NOT" - the condition must be false
%
% Example:
%   suspect(X) :- has_cargo(X, C), banned_substance(C).
%   Reads as: "X is suspect IF X has cargo C AND C is a banned substance"
%
% Variables (capitalized like X, C, P) are placeholders that Prolog will
% try to match. They can represent any value that makes the rule true.
# =============================================================================

% An alien is suspect if carrying banned substances
% Rule: X is suspect IF X has cargo C AND C is a banned substance
% The variable C binds to the cargo item, then we check if it's banned.
suspect(X) :-
    has_cargo(X, C),
    banned_substance(C).

% An alien is suspect if from an outer planet without a visa
% Rule: X is suspect IF X is from planet P AND P is outer AND X has NO visa
% This demonstrates:
%   - Variable binding (P gets the planet name)
%   - Chaining conditions (origin -> planet -> outer)
%   - Negation (\+ means "does not have")
suspect(X) :-
    origin(X, P),
    planet(P, outer),
    \+ has_visa(X).

% Calculate total tax
% PROLOG ADVANCED - COLLECTING SOLUTIONS:
% -----------------------------------------------------------------------------
% findall(Template, Goal, List) collects all solutions to Goal where Template
% specifies what values to collect into List.
%
% This rule:
%   1. Finds all cargo items C that X has
%   2. For each taxable cargo, collects its tax Amount
%   3. Sums all amounts into Total
%
% Example: If X has cargo [ore, water], it collects [50, 10] and sums to 60.
calculate_total_tax(X, Total) :-
    findall(Amount, (
        has_cargo(X, C),
        taxable_substance(C, Amount)
    ), Amounts),
    sum_list(Amounts, Total).

% Complete documents verification
% PROLOG BASICS - ANONYMOUS VARIABLES:
% ----------------------------------------------------------------------------------
% The underscore (_) is an anonymous variable meaning "any value, I don't care".
% Here, we check that X has an origin (any origin), but we don't need to know which.
% Rule: X has complete documents IF X has a visa AND X has an origin (any origin)
has_complete_documents(X) :-
    has_visa(X),
    origin(X, _).

# =============================================================================
% PROLOG TUTORIAL: ADVANCED SECURITY RULES
# =============================================================================
% PROLOG BASICS - MULTIPLE RULES WITH DIFFERENT VALUES:
% ----------------------------------------------------------------------------------
% These rules define threat_level(X, Level) with different Level values.
% Prolog tries rules in order and returns the first matching one.
% The rules form a priority system: critical > high > medium > low
# =============================================================================

% Threat level - CRITICAL (highest priority)
% Rule: X has critical threat IF X has tentacles AND X has banned cargo
% This is the most dangerous combination.
threat_level(X, critical) :-
    has_tentacles(X),
    has_cargo(X, C),
    banned_substance(C).

% Threat level - HIGH
% Rule: X has high threat IF X has tentacles AND X lacks a slime permit
threat_level(X, high) :-
    has_tentacles(X),
    \+ has_slime_permit(X).

% Threat level - MEDIUM
% Rule: X has medium threat IF X is suspect (carries banned substances or
%       from outer planet without visa - see suspect/1 rules above)
threat_level(X, medium) :-
    suspect(X).

% Threat level - LOW
% Rule: X has low threat IF X is NOT suspect AND X has a visa
% This is the safest category.
threat_level(X, low) :-
    \+ suspect(X),
    has_visa(X).

% Requires quarantine
% PROLOG BASICS - MULTIPLE DEFINITIONS (OR LOGIC):
% -----------------------------------------------------------------------------
% Multiple rules with the same head mean "OR" - the predicate is true if
% ANY of the conditions are met.
% X requires quarantine IF X is from europa OR X is a gaseous species.
requires_quarantine(X) :-
    origin(X, europa).

requires_quarantine(X) :-
    species(X, gaseous).

# =============================================================================
% PROLOG TUTORIAL: TEMPORAL RULES (Day of the week)
# =============================================================================
% These rules demonstrate conditional logic based on alien characteristics.
% The second rule uses anonymous variable (_) meaning "any day" for non-3-eyed aliens.
# =============================================================================

% Aliens with 3 eyes can only enter on Tuesday
% Rule: X is allowed on tuesday IF X has 3 eyes
allowed_by_day(X, tuesday) :-
    eyes_count(X, 3).

% All other aliens (not 3-eyed) are allowed on any day
% Rule: X is allowed on any day IF X does NOT have 3 eyes
% The underscore (_) means "any day value is acceptable"
allowed_by_day(X, _) :-
    \+ eyes_count(X, 3).

# =============================================================================
% PROLOG TUTORIAL: BACKGROUND CHECK
# =============================================================================
% This section shows how to store and query multiple records per entity.
% An alien can have multiple criminal_record facts (see Zorglub has 2 records).
# =============================================================================

% Example criminal records
% Facts storing criminal history. An alien can have multiple records.
% Format: criminal_record(AlienName, CrimeType).
criminal_record(zorglub, smuggling).
criminal_record(zorglub, document_fraud).
criminal_record(glorp, illegal_substance_import).

% An alien has a criminal record
% PROLOG BASICS - EXISTENCE CHECK:
% ------------------------------------------------------------------------------
% This rule checks if ANY criminal_record fact exists for X.
% The anonymous variable (_) means "any crime type, I don't care which".
% Rule: X has a record IF there exists a criminal_record(X, _) fact
has_record(X) :-
    criminal_record(X, _).

# =============================================================================
% PROLOG TUTORIAL: COMPLEX DECISION RULES
# =============================================================================
% These rules demonstrate complex logical conditions including:
% - Nested negation: \+ (condition1, condition2)
% - Calling other predicates: calculate_total_tax(X, T)
% - Comparison operators: T > 0
% - Lists as values: [quarantine], [tax_payment]
# =============================================================================

% Standard authorization
% PROLOG BASICS - NESTED NEGATION:
% ------------------------------------------------------------------------------
% \+ (has_cargo(X, C), banned_substance(C)) means "NOT (has cargo C AND C is banned)"
% This is equivalent to "does NOT have any banned cargo"
% Rule: X is authorized IF:
%   - X has a visa AND
%   - X does NOT have banned cargo AND
%   - X does NOT have a criminal record
decision_authorized(X) :-
    has_visa(X),
    \+ (has_cargo(X, C), banned_substance(C)),
    \+ has_record(X).

% Authorization with conditions - Quarantine required
% Rule: X is authorized with quarantine condition IF:
%   - X has a visa AND
%   - X requires quarantine (from europa or is gaseous)
% Returns a list [quarantine] indicating the condition.
decision_authorized_conditional(X, [quarantine]) :-
    has_visa(X),
    requires_quarantine(X).

% Authorization with conditions - Tax payment required
% PROLOG ADVANCED - USING CALCULATED VALUES:
% ---------------------------------------------------------------------------------
% This rule calls calculate_total_tax(X, T) which computes the total tax.
% Then it checks if T > 0 (comparison operator).
% Rule: X is authorized with tax_payment condition IF:
%   - X has a visa AND
%   - X's total tax T is greater than 0
decision_authorized_conditional(X, [tax_payment]) :-
	has_visa(X),
	calculate_total_tax(X, T),
	T > 0.

% Entry refusal rules
% PROLOG BASICS - MULTIPLE REFUSAL CONDITIONS:
% -----------------------------------------------------------------------------------
% Multiple rules mean "OR" - X is refused if ANY condition is true.
% These rules define automatic refusal scenarios.

% Refusal rule 1: No visa
% Rule: X is refused IF X does NOT have a visa
decision_refused(X) :-
	\+ has_visa(X).

% Refusal rule 2: Critical threat level
% Rule: X is refused IF X has critical threat level
decision_refused(X) :-
	threat_level(X, critical).

% Refusal rule 3: Carrying banned substances
% Rule: X is refused IF X has cargo C AND C is a banned substance
decision_refused(X) :-
	has_cargo(X, C),
	banned_substance(C).
