/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 * Version 0.1.0
 *
 * This file defines the main Prologot class that provides an interface
 * to the SWI-Prolog engine from GDScript.
 */

#pragma once

#include <SWI-Prolog.h>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

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
     *     - "optimised" (bool): Enable optimised compilation
     *   Behavior options:
     *     - "traditional" (bool): Traditional mode, disable v7 extensions
     *     - "threads" (bool): Allow threads (default: true)
     *     - "signals" (bool): Modify signal handling (default: true)
     *     - "packs" (bool): Attach add-ons/packages (default: true)
     *     - "debug" (bool): Generate debug info
     *     - "debug on interrupt" (bool): Trigger debugger on interrupt
     *     - "tty" (bool): Allow terminal control
     *   Error handling:
     *     - "on error" (String): Error handling style ("print", "halt",
     * "status")
     *     - "on warning" (String): Warning handling style ("print", "halt",
     * "status") Advanced options:
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
     * @brief Loads a Prolog file into the knowledge base.
     *
     * This method uses Prolog's built-in consult/1 predicate to load a .pl
     * file. The file is parsed and all clauses are added to the knowledge base.
     *
     * @param p_filename Path to the Prolog file (.pl) to load.
     * @return true if the file was loaded successfully, false otherwise.
     */
    bool consult(String const& p_filename);

    /**
     * @brief Loads Prolog code from a string into the knowledge base.
     *
     * This method uses the bootstrap predicate load_program_from_string/1
     * (created during initialization) to parse and load multi-line Prolog code.
     * The code can contain multiple clauses, directives, and queries.
     *
     * @param p_prolog_code The Prolog code to load (can be multi-line).
     * @return true if the code was loaded successfully, false otherwise.
     */
    bool consult_string(String const& p_prolog_code);

    // =========================================================================
    // Query Execution
    // =========================================================================

    /**
     * @brief Executes a Prolog query and checks if it succeeds.
     *
     * This method executes a query and returns true if at least one solution
     * exists. It does not collect or return the solutions themselves.
     *
     * @param p_goal The Prolog goal to execute (e.g., "member(X, [1,2,3])").
     * @return true if the query succeeds (has at least one solution), false
     * otherwise.
     */
    bool query(String const& p_goal);

    /**
     * @brief Executes a Prolog query and returns all solutions.
     *
     * This method uses Prolog's findall/3 to collect all solutions.
     * Each solution is converted to a Variant (atom becomes String,
     * compound term becomes Array [functor, arg1, arg2, ...]).
     *
     * @param p_goal The Prolog goal to execute.
     * @return Array of Variants, one per solution. Empty array if no solutions.
     */
    Array query_all(String const& p_goal);

    /**
     * @brief Executes a Prolog query and returns the first solution.
     *
     * This method executes a query and returns only the first solution found.
     * Returns a null Variant if no solution is found.
     *
     * @param p_goal The Prolog goal to execute.
     * @return The solution as Variant, or null Variant if no solution.
     */
    Variant query_one(String const& p_goal);

    /**
     * @brief Gets the last error message from Prolog.
     *
     * @return The last error message, or empty string if no error.
     */
    String get_last_error() const;

    // =========================================================================
    // Dynamic Assertions
    // =========================================================================

    /**
     * @brief Asserts a fact into the Prolog knowledge base.
     *
     * This method adds a new clause to the knowledge base. The fact will be
     * available for subsequent queries. Uses Prolog's assert/1 predicate.
     *
     * @param p_fact The Prolog fact to assert (e.g., "likes(john, pizza)").
     * @return true if the assertion succeeded, false otherwise.
     */
    bool assert_fact(String const& p_fact);

    /**
     * @brief Retracts a fact from the Prolog knowledge base.
     *
     * This method removes a clause from the knowledge base. Uses Prolog's
     * retract/1 predicate, which removes the first matching clause.
     *
     * @param p_fact The Prolog fact to retract (must match exactly).
     * @return true if a matching fact was found and retracted, false otherwise.
     */
    bool retract_fact(String const& p_fact);

    /**
     * @brief Retracts all facts matching a functor pattern.
     *
     * This method removes all clauses that match the given functor pattern.
     * Uses Prolog's retractall/1 predicate, which removes all matching clauses.
     *
     * Example: retract_all("likes(_, _)") removes all likes/2 facts.
     *
     * @param p_functor The functor pattern to match (can contain variables).
     * @return true if any matching facts were retracted, false otherwise.
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
     */
    bool call_predicate(String const& p_predicate, Array const& p_args);

    /**
     * @brief Calls a Prolog predicate and returns a result value.
     *
     * This method is similar to call_predicate(), but treats the predicate
     * as a function that returns a value. The result is expected to be the
     * last argument of the predicate.
     *
     * Example: call_function("length", [list]) where length(List, N) returns N.
     *
     * @param p_predicate Name of the predicate to call.
     * @param p_args Array of input arguments (result is the last argument).
     * @return The result value as a Variant, or Variant() if the call failed.
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
     * query_all().
     *
     * @return Array of Dictionary objects describing each predicate.
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
     * @brief Sets the SWI-Prolog home directory environment variable.
     *
     * This static helper function sets the SWI_HOME_DIR environment variable
     * in a platform-independent way (uses _putenv_s on Windows, setenv on
     * Unix).
     *
     * @param p_prolog_home Path to SWI-Prolog installation directory.
     */
    static void set_swi_home_dir(String const& p_prolog_home);

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
     * @brief Helper to handle Prolog exceptions.
     *
     * This method extracts exception information from a failed query and
     * stores it in m_last_error, then displays it as an error message.
     *
     * @param p_qid The query ID that raised the exception.
     * @param p_context Context string for error messages (e.g., "Query",
     * "Consult").
     * @return true if exception was handled, false if no exception occurred.
     */
    bool handle_prolog_exception(qid_t p_qid, String const& p_context);

private:

    /** Whether the Prolog engine has been initialized. */
    bool m_initialized;

    /** Last error message from Prolog. */
    String m_last_error;

    /**
     * @brief Singleton instance pointer for global access.
     *
     * This static member allows access to the Prologot instance from anywhere
     * in the codebase without needing to pass references around.
     */
    static Prologot* m_singleton;
};
