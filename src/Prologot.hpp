/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file defines the main Prologot class that provides an interface
 * to the SWI-Prolog engine from GDScript.
 */

#pragma once

#include "PrologGoal.hpp"
#include "PrologPredicate.hpp"
#include "PrologSolution.hpp"
#include "PrologTerm.hpp"
#include "PrologVariable.hpp"
#include <SWI-Prolog.h>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <map>
#include <string>
#include <vector>

using namespace godot;

/**
 * @class Prologot
 * @brief Main class providing SWI-Prolog integration for Godot 4.
 *
 * This class wraps the SWI-Prolog C API and exposes it to GDScript,
 * allowing users to execute Prolog queries, assert/retract facts,
 * and consult Prolog files from within Godot.
 */
class Prologot: public RefCounted
{
    GDCLASS(Prologot, RefCounted)

public:

    /**
     * @brief Constructs a new Prologot instance.
     *
     * Initializes the singleton pointer and sets the initialized flag to false.
     * The Prolog engine is not started until initialize() is called.
     */
    Prologot();

    /**
     * @brief Destructs the Prologot instance.
     *
     * Ensures proper cleanup of the Prolog engine and resets the singleton
     * pointer. This prevents memory leaks and ensures clean shutdown.
     */
    ~Prologot();

    /**
     * @brief Gets the singleton instance of Prologot.
     *
     * This static method provides global access to the Prologot instance.
     * Useful for accessing Prologot from C++ code without passing references.
     *
     * @return Pointer to the singleton instance, or nullptr if not created yet.
     */
    static Prologot* get_singleton();

    // =========================================================================
    // Initialization and Cleanup
    // =========================================================================

    /**
     * @brief Initializes the SWI-Prolog engine with optional configuration.
     *
     * This method performs the following steps:
     * 1. Checks if already initialized (idempotent)
     * 2. Parses the options Dictionary for configuration settings
     * 3. Sets up the SWI-Prolog home directory if provided
     * 4. Initializes the Prolog engine with the specified options
     * 5. Bootstraps helper predicates needed for consult_string()
     *
     * The bootstrap predicates enable loading Prolog code from strings by:
     * - Parsing multi-line Prolog code into individual clauses
     * - Handling directives (:-) and queries (?-) appropriately
     * - Asserting regular clauses into the knowledge base
     *
     * @param p_options Dictionary containing initialization options:
     *   Main options:
     *     - "home" (String): Path to SWI-Prolog installation
     *     - "quiet" (bool): Suppress informational messages (default: true)
     *     - "goal" (String/Array): Goal(s) to execute at startup
     *     - "toplevel" (String): Custom toplevel goal
     *     - "init file" (String): User initialization file
     *     - "script file" (String): Script source file to load
     *   Performance options:
     *     - "stack limit" (String): Prolog stack limit (e.g. "1g", "512m")
     *     - "table space" (String): Space for SLG tables (e.g. "128m")
     *     - "shared table space" (String): Space for shared SLG tables
     *     - "optimized" (bool): Enable optimized compilation
     *   Behavior options:
     *     - "traditional" (bool): Traditional mode, disable v7 extensions
     *     - "threads" (bool): Allow threads (default: true)
     *     - "packs" (bool): Attach add-ons/packages (default: true)
     *   Error handling:
     *     - "on error" (String): Error handling style ("print", "halt",
     * "status")
     *     - "on warning" (String): Warning handling style ("print", "halt",
     * "status")
     *   Advanced options:
     *     - "prolog flags" (Dictionary): Define Prolog flags
     *     - "file search paths" (Dictionary): Define file search paths
     *     - "custom args" (Array): Additional custom arguments
     *
     * @return true if initialization succeeded, false otherwise.
     */
    bool initialize(Dictionary const& p_options = Dictionary());

    /**
     * @brief Cleans up and shuts down the Prolog engine.
     *
     * This method is safe to call multiple times. It only performs cleanup
     * if the engine was actually initialized. After cleanup, the engine
     * must be re-initialized before use.
     */
    void cleanup();

    /**
     * @brief Checks if the Prolog engine is currently initialized.
     *
     * @return true if initialized and ready to use, false otherwise.
     */
    bool is_initialized() const;

    // =========================================================================
    // File and Code Consultation
    // =========================================================================

    /**
     * @brief Consults a Prolog file into the knowledge base.
     *
     * This method uses Prolog's built-in consult/1 predicate to load a .pl
     * file. The file is parsed and all clauses are added to the knowledge base.
     *
     * Multiple calls to consult_file() and consult_string() accumulate clauses
     * in the knowledge base. Each new file or code string adds its clauses to
     * the existing knowledge base without removing previous ones. If you need
     * to replace the knowledge base, use retract_all() to remove specific
     * predicates first, or reinitialize the engine with cleanup() and
     * initialize().
     *
     * @param p_filename Path to the Prolog file (.pl) to load.
     * @return true if the file was loaded successfully, false otherwise.
     *
     * @example
     * # Load a Prolog file containing facts and rules:
     * # File: family.pl
     * #   parent(tom, bob).
     * #   parent(bob, ann).
     * #   grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
     * prolog.consult_file("res://rules/family.pl")
     *
     * # Subsequent calls add to the knowledge base:
     * prolog.consult_file("res://rules/game_rules.pl")  # Adds more clauses
     * prolog.consult_string("enemy(goblin, 10).")  # Adds even more clauses
     * # All clauses from both files and the code string are now available
     */
    bool consult_file(String const& p_filename);

    /**
     * @brief Consults Prolog code from a string into the knowledge base.
     *
     * This method uses the bootstrap predicate load_program_from_string/1
     * (created during initialization) to parse and load multi-line Prolog code.
     * The code can contain multiple clauses, directives, and queries.
     *
     * Multiple calls to consult_string() and consult_file() accumulate clauses
     * in the knowledge base. Each new code string adds its clauses to the
     * existing knowledge base without removing previous ones. If you need to
     * replace the knowledge base, use retract_all() to remove specific
     * predicates first, or reinitialize the engine with cleanup() and
     * initialize().
     *
     * @param p_prolog_code The Prolog code to load (can be multi-line).
     * @return true if the code was loaded successfully, false otherwise.
     *
     * @example
     * prolog.consult_string("""
     *     parent(tom, bob).
     *     parent(bob, ann).
     *     grandparent(X, Z) :- parent(X, Y), parent(Y, Z).
     * """)
     *
     * # Subsequent calls add to the knowledge base:
     * prolog.consult_string("enemy(goblin, 10).")
     * prolog.consult_file("res://rules/game_rules.pl")
     * # All clauses from both code strings and the file are now available
     */
    bool consult_string(String const& p_prolog_code);

    // =========================================================================
    // Structured term factories
    // =========================================================================

    Ref<PrologTerm> atom(String const& p_name);
    Ref<PrologTerm> integer(int64_t p_value);
    Ref<PrologTerm> real(double p_value);
    Ref<PrologTerm> string(String const& p_value);
    Ref<PrologTerm> nil();
    Ref<PrologTerm> list(Array const& p_items);
    Ref<PrologTerm> compound(String const& p_functor, Array const& p_args);
    Ref<PrologVariable> variable(String const& p_name = String());
    Ref<PrologVariable> anonymous();
    Ref<PrologPredicate> predicate(String const& p_name, int p_arity);

    // =========================================================================
    // High-level solving (structured terms, no Prolog source parsing)
    // =========================================================================

    /**
     * @brief Solves a structured Prolog goal and returns whether it succeeds.
     *
     * This is the high-level API: the goal is built from Godot values and
     * sent to SWI-Prolog as a term. It does not parse Prolog source text.
     * Strings that start with an uppercase letter or '_' are variables.
     *
     * @param p_goal Functor name (String), or a compound term Dictionary
     * {"functor": name, "args": [...]}.
     * @param p_args Arguments when p_goal is a functor name. Ignored when
     * p_goal is a Dictionary.
     * @return true if at least one solution exists, false otherwise.
     *
     * @example
     * prolog.solve("parent", ["tom", "child"])
     * prolog.solve({"functor": "parent", "args": ["tom", "child"]})
     */
    bool solve(Variant const& p_goal, Array const& p_args = Array());

    /**
     * @brief Solves a structured Prolog goal and returns all solutions.
     *
     * Variable arguments (uppercase or '_') are collected into Dictionaries
     * mapping variable names to bound values. Anonymous "_" is omitted.
     * If the goal has no named variables, each solution is the whole term.
     *
     * @param p_goal Functor name (String), or a compound term Dictionary.
     * @param p_args Arguments when p_goal is a functor name.
     * @return Array of solutions. Empty array if none.
     *
     * @example
     * prolog.solve_all("parent", ["tom", "X"])
     * # Returns: [{"X": "bob"}, {"X": "liz"}]
     */
    Array solve_all(Variant const& p_goal, Array const& p_args = Array());

    /**
     * @brief Solves a structured Prolog goal and returns the first solution.
     *
     * @param p_goal Functor name (String), or a compound term Dictionary.
     * @param p_args Arguments when p_goal is a functor name.
     * @return Bindings Dictionary if the goal has named variables, otherwise
     * the whole term. Null Variant if no solution.
     *
     * @example
     * prolog.solve_one("parent", ["tom", "X"])
     * # Returns: {"X": "bob"} or null
     */
    Variant solve_one(Variant const& p_goal, Array const& p_args = Array());

    /**
     * @brief Returns true if a PrologGoal has at least one solution.
     */
    bool succeeds(Ref<PrologGoal> const& p_goal);

    // =========================================================================
    // Low-level queries (Prolog source text)
    // =========================================================================

    /**
     * @brief Executes a Prolog source goal and returns whether it succeeds.
     *
     * This is the low-level API: the string is parsed by SWI-Prolog
     * (`PL_chars_to_term`). Use it for conjunctions, operators, or any
     * goal that is already written as Prolog text.
     *
     * A trailing period ('.') is ignored if present.
     *
     * @param p_goal Prolog goal source (e.g., "parent(tom, X)").
     * @return true if at least one solution exists, false otherwise.
     *
     * @example
     * prolog.query_text("parent(tom, bob)")
     * prolog.query_text("parent(tom, X), parent(X, Y)")
     */
    bool query_text(String const& p_goal);

    /**
     * @brief Executes a Prolog source goal and returns all solutions.
     *
     * Solutions are converted as Prolog terms (atoms, lists, or compound
     * Dictionaries {"functor": name, "args": [...]}). Variable names in the
     * source string are not extracted; use solve_all() for named bindings.
     *
     * @param p_goal Prolog goal source (e.g., "parent(tom, X)").
     * @return Array of solutions. Empty array if none.
     *
     * @example
     * prolog.query_text_all("parent(X, Y)")
     * # Returns: [{"functor": "parent", "args": ["tom", "bob"]}, ...]
     */
    Array query_text_all(String const& p_goal);

    /**
     * @brief Executes a Prolog source goal and returns the first solution.
     *
     * @param p_goal Prolog goal source (e.g., "parent(tom, X)").
     * @return The solution as a Variant, or null if none.
     *
     * @example
     * prolog.query_text_one("parent(tom, X)")
     * # Returns: {"functor": "parent", "args": ["tom", "bob"]}
     */
    Variant query_text_one(String const& p_goal);

    /**
     * @brief Gets the last error message from Prolog.
     *
     * This method returns the last error message stored in m_last_error.
     * Note: This method does not call push_error(). Errors are automatically
     * handled according to the "on error" and "on warning" options set during
     * initialization. Use this method to retrieve error messages for custom
     * error handling or logging.
     *
     * @return The last error message, or empty string if no error.
     */
    String get_last_error() const;

    // =========================================================================
    // Dynamic Assertions
    // =========================================================================

    /**
     * @brief Adds a fact into the Prolog knowledge base.
     *
     * This method adds a new clause to the knowledge base. The fact will be
     * available for subsequent queries. Uses Prolog's assert/1 predicate.
     *
     * Note: Do not include a trailing period ('.') in the fact string. If a
     * period is present, it will be automatically removed. For example,
     * "parent(tom, bob)." will be treated as "parent(tom, bob)".
     *
     * @param p_fact The Prolog fact to add (e.g., "likes(john, pizza)").
     * @return true if the fact was added successfully, false otherwise.
     *
     * @example
     * prolog.add_fact("parent(tom, bob)")
     * prolog.add_fact("game_state(level, 5)")
     * # Note: "parent(tom, bob)." also works (period is removed automatically)
     */
    bool add_fact(String const& p_fact);

    /**
     * @brief Removes a fact from the Prolog knowledge base.
     *
     * This method removes a clause from the knowledge base. Uses Prolog's
     * retract/1 predicate, which removes the first matching clause.
     *
     * Note: Do not include a trailing period ('.') in the fact string. If a
     * period is present, it will be automatically removed.
     *
     * @param p_fact The Prolog fact to remove (must match exactly).
     * @return true if a matching fact was found and removed, false otherwise.
     *
     * @example
     * prolog.retract_fact("parent(tom, bob)")
     * prolog.retract_fact("game_state(level, 5)")
     */
    bool retract_fact(String const& p_fact);

    /**
     * @brief Retracts all facts matching a functor pattern.
     *
     * This method removes all clauses that match the given functor pattern.
     * Uses Prolog's retractall/1 predicate, which removes all matching clauses.
     *
     * Note: Do not include a trailing period ('.') in the functor pattern. If a
     * period is present, it will be automatically removed.
     *
     * @param p_functor The functor pattern to match (can contain variables).
     * @return true if any matching facts were retracted, false otherwise.
     *
     * @example
     * # Remove all likes/2 facts
     * prolog.retract_all("likes(_, _)")
     *
     * # Remove all game_state facts
     * prolog.retract_all("game_state(_, _)")
     *
     * # Remove all facts with a specific first argument
     * prolog.retract_all("parent(tom, _)")
     */
    bool retract_all(String const& p_functor);

    // =========================================================================
    // Predicate Manipulation
    // =========================================================================

    /**
     * @brief Calls a Prolog predicate with arguments.
     *
     * This method constructs a Prolog goal from a predicate name and arguments,
     * then executes it. The arguments are converted from Godot Variants to
     * Prolog terms automatically.
     *
     * @param p_predicate Name of the predicate to call (e.g., "member").
     * @param p_args Array of arguments to pass to the predicate.
     * @return true if the predicate call succeeded, false otherwise.
     *
     * @example
     * # Call member/2: member(3, [1, 2, 3, 4])
     * prolog.call_predicate("member", [3, [1, 2, 3, 4]])  // Returns true
     *
     * # Call traditional Prolog predicates
     * prolog.call_predicate("assertz", ["parent(tom, bob)"])
     * prolog.call_predicate("retract", ["parent(tom, bob)"])
     */
    bool call_predicate(String const& p_predicate, Array const& p_args);

    /**
     * @brief Calls a Prolog predicate and returns a result value.
     *
     * This method is similar to call_predicate(), but treats the predicate
     * as a function that returns a value. The result is expected to be the
     * last argument of the predicate.
     *
     * @param p_predicate Name of the predicate to call.
     * @param p_args Array of input arguments (result is the last argument).
     * @return The result value as a Variant, or Variant() if the call failed.
     *
     * @example
     * # Call length/2: length([1, 2, 3], N) returns N
     * var len = prolog.call_function("length", [[1, 2, 3]])
     * # Returns: 3
     *
     * # Call arithmetic: plus(5, 3, Result) returns Result
     * var sum = prolog.call_function("plus", [5, 3])
     * # Returns: 8
     */
    Variant call_function(String const& p_predicate, Array const& p_args);

    // =========================================================================
    // Introspection
    // =========================================================================

    /**
     * @brief Checks if a predicate exists with the given arity.
     *
     * This method uses PL_predicate() to look up a predicate. If the predicate
     * doesn't exist, PL_predicate() returns 0 (NULL).
     *
     * @param p_predicate Name of the predicate to check.
     * @param p_arity Number of arguments the predicate should have.
     * @return true if the predicate exists, false otherwise.
     */
    bool predicate_exists(String const& p_predicate, int p_arity);

    /**
     * @brief Lists all currently defined predicates.
     *
     * This method uses Prolog's current_predicate/1 to query for all predicates
     * currently in the knowledge base. Returns an Array of results from
     * query_text_all().
     *
     * @return Array of Dictionary objects describing each predicate.
     *
     * @example
     * # List all predicates in the knowledge base
     * var predicates = prolog.list_predicates()
     * # Returns: [{"functor": "parent", "args": ["/", 2]}, ...]
     * # Format: Name/Arity
     */
    Array list_predicates();

protected:

    /**
     * @brief Binds all C++ methods to GDScript.
     *
     * This method is called by Godot's class system to register all methods
     * that should be accessible from GDScript. Methods are organized by
     * category for better code organization.
     */
    static void _bind_methods();

private:

    /**
     * @brief Resolves the SWI-Prolog home directory from the "home" option.
     *
     * Priority: 1) User-specified "home" (resolved from Godot paths), 2)
     * Extension directory, 3) Empty (system default). Verifies boot.prc exists.
     * Sets error message if user explicitly provided an invalid path.
     *
     * @param p_home_option Value of the "home" option (may be empty, res://,
     * user://, or absolute).
     * @return Resolved absolute path with boot.prc, or empty for system
     * default, and error message if validation fails.
     */
    std::pair<String, String> set_swi_home_dir(String const& p_home_option);

    /**
     * @brief Resolves Godot virtual paths to absolute filesystem paths.
     *
     * Converts res:// and user:// paths to absolute paths using
     * ProjectSettings. Returns the path unchanged if it's already an absolute
     * path.
     *
     * @param p_path The path to resolve (may be res://, user://, or absolute).
     * @return The absolute filesystem path.
     */
    static String resolve_godot_path(String const& p_path);

    /**
     * @brief Converts a Prolog term to a Godot Variant.
     *
     * Internal helper method for converting Prolog data types to Godot types.
     * Handles atoms, integers, floats, strings, lists, and compound terms.
     *
     * @param p_term The Prolog term to convert.
     * @return The converted Variant.
     */
    Variant term_to_variant(term_t p_term);

    /**
     * @brief Converts a Godot Variant to a Prolog term.
     *
     * Internal helper method for converting Godot types to Prolog terms.
     * Handles NIL, bool, int, float, String, and Array types.
     *
     * @param p_var The Variant to convert.
     * @return The created Prolog term (0 if conversion failed).
     */
    term_t variant_to_term(Variant const& p_var);

    /**
     * @brief Helper to create Prolog lists from Godot Arrays.
     *
     * Note: Currently not implemented. Lists are handled in variant_to_term()
     * by converting Arrays to compound terms.
     *
     * @param p_arr The Array to convert.
     * @return The created Prolog list term.
     */
    term_t array_to_prolog_list(Array const& p_arr);

    /**
     * @brief Returns true if p_name is a Prolog variable name.
     *
     * Variable names start with an uppercase letter or '_'.
     */
    static bool is_variable_name(String const& p_name);

    /**
     * @brief Converts a solve() argument to a Prolog term.
     *
     * Uppercase / '_' strings become (shared) variables. Compound
     * Dictionaries and Arrays are converted recursively so nested
     * variables stay variables.
     */
    term_t solve_arg_to_term(Variant const& p_arg,
                             std::map<std::string, term_t>& p_named_vars,
                             Array& p_var_names,
                             std::vector<term_t>& p_var_terms);

    /**
     * @brief Builds a Prolog goal term from a structured solve() call.
     *
     * Rejects Prolog source strings (use query_text() instead).
     */
    bool build_solve_goal(Variant const& p_goal,
                          Array const& p_args,
                          term_t p_out_goal,
                          Array& p_var_names,
                          std::vector<term_t>& p_var_terms);

    /**
     * @brief Reads named variable terms into a Dictionary of bindings.
     */
    Dictionary bindings_from_vars(Array const& p_var_names,
                                  std::vector<term_t> const& p_var_terms);

    term_t object_arg_to_term(Variant const& p_arg,
                              std::map<int64_t, term_t>& p_vars,
                              std::vector<Ref<PrologVariable>>& p_order);

    term_t prolog_term_to_swi(Ref<PrologTerm> const& p_term,
                              std::map<int64_t, term_t>& p_vars,
                              std::vector<Ref<PrologVariable>>& p_order);

    bool compile_goal(Ref<PrologGoal> const& p_goal,
                      term_t p_out_goal,
                      std::map<int64_t, term_t>& p_vars,
                      std::vector<Ref<PrologVariable>>& p_order);

    Array collect_goal_solutions(Ref<PrologGoal> const& p_goal);

    /**
     * @brief Helper to push error messages respecting error handling options.
     *
     * This method checks the error handling options ("on error", "on warning")
     * and either prints the error, halts, or only stores it in m_last_error.
     *
     * @param p_message The error message to handle.
     * @param p_type The error type ("error" or "warning").
     */
    void push_error(String const& p_message, String const& p_type = "error");

    /**
     * @brief Helper to handle Prolog exceptions.
     *
     * This method extracts exception information from a failed query and
     * stores it in m_last_error, then displays it as an error message.
     *
     * @param p_qid The query ID that raised the exception.
     * @param p_context Context string for error messages (e.g., "Query",
     * "Load file").
     * @return true if exception was handled, false if no exception occurred.
     */
    bool handle_prolog_exception(qid_t p_qid, String const& p_context);

private:

    /** Whether the Prolog engine has been initialized. */
    bool m_initialized;

    /** Last error message from Prolog. */
    String m_last_error;

    /** Error handling option: "print", "halt", or "status". */
    String m_on_error;

    /** Warning handling option: "print", "halt", or "status". */
    String m_on_warning;

    /**
     * @brief Singleton instance pointer for global access.
     *
     * This static member allows access to the Prologot instance from anywhere
     * in the codebase without needing to pass references around.
     */
    static Prologot* m_singleton;
};
