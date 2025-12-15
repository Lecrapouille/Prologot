/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 * Version 0.1.0
 *
 * This file implements the Prologot class methods.
 */

#include "Prologot.hpp"
#include <cstring>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// =============================================================================
// Static Member Initialization
// =============================================================================

Prologot* Prologot::m_singleton = nullptr;

// =============================================================================
// Godot Method Binding
// =============================================================================

void Prologot::_bind_methods()
{
    // Initialization methods
    ClassDB::bind_method(D_METHOD("initialize", "prolog_home"),
                         &Prologot::initialize,
                         DEFVAL(""));
    ClassDB::bind_method(D_METHOD("cleanup"), &Prologot::cleanup);
    ClassDB::bind_method(D_METHOD("is_initialized"), &Prologot::is_initialized);

    // File/code loading methods
    ClassDB::bind_method(D_METHOD("consult", "filename"), &Prologot::consult);
    ClassDB::bind_method(D_METHOD("consult_string", "prolog_code"),
                         &Prologot::consult_string);

    // Query methods
    ClassDB::bind_method(D_METHOD("query", "goal"), &Prologot::query);
    ClassDB::bind_method(D_METHOD("query_all", "goal"), &Prologot::query_all);
    ClassDB::bind_method(D_METHOD("query_one", "goal"), &Prologot::query_one);

    // Dynamic assertion methods
    ClassDB::bind_method(D_METHOD("assert_fact", "fact"),
                         &Prologot::assert_fact);
    ClassDB::bind_method(D_METHOD("retract_fact", "fact"),
                         &Prologot::retract_fact);
    ClassDB::bind_method(D_METHOD("retract_all", "functor"),
                         &Prologot::retract_all);

    // Predicate methods
    ClassDB::bind_method(D_METHOD("call_predicate", "predicate", "args"),
                         &Prologot::call_predicate);
    ClassDB::bind_method(D_METHOD("call_function", "predicate", "args"),
                         &Prologot::call_function);

    // Introspection methods
    ClassDB::bind_method(D_METHOD("predicate_exists", "predicate", "arity"),
                         &Prologot::predicate_exists);
    ClassDB::bind_method(D_METHOD("list_predicates"),
                         &Prologot::list_predicates);

    // Error handling
    ClassDB::bind_method(D_METHOD("get_last_error"), &Prologot::get_last_error);
}

// =============================================================================
// Constructor and Destructor
// =============================================================================

Prologot::Prologot()
{
    m_initialized = false;
    m_singleton = this;
}

Prologot::~Prologot()
{
    cleanup();
    m_singleton = nullptr;
}

// =============================================================================
// Initialization and Cleanup
// =============================================================================

bool Prologot::initialize(String const& p_prolog_home)
{
    // Idempotent: if already initialized, return success immediately
    if (m_initialized)
        return true;

    // Set up Prolog initialization arguments
    // The first argument is the program name (used by Prolog internally)
    const char* argv[] = { "godot", nullptr };

    // Set SWI-Prolog home directory if specified
    // This allows custom SWI-Prolog installations
    if (!p_prolog_home.is_empty())
    {
        setenv("SWI_HOME_DIR", p_prolog_home.utf8().get_data(), 1);
    }

    // Initialize the Prolog engine
    // PL_initialise() starts the Prolog runtime system
    if (!PL_initialise(1, (char**)argv))
    {
        return false;
    }

    // Bootstrap helper predicates for consult_string()
    // These predicates allow loading Prolog code from strings by:
    // 1. Opening a string as a stream
    // 2. Reading terms one by one until end_of_file
    // 3. Processing each term (directive, query, or clause)
    //
    // We assert each clause individually since PL_chars_to_term() works on
    // single terms only, not multi-clause programs.
    const char* predicates[] = {
        // Main entry point: opens string stream and loads clauses
        "load_program_from_string(Code) :- "
        "open_string(Code, Stream), "
        "call_cleanup(prologot_load_clauses(Stream), close(Stream))",

        // Recursively reads terms from stream until end_of_file
        "prologot_load_clauses(Stream) :- "
        "read_term(Stream, Term, []), "
        "(Term == end_of_file -> true ; "
        "prologot_process_clause(Term), prologot_load_clauses(Stream))",

        // Process directive clauses (:- Goal) - execute immediately
        "prologot_process_clause((:- Goal)) :- !, call(Goal)",

        // Process query clauses (?- Goal) - execute immediately
        "prologot_process_clause((?- Goal)) :- !, call(Goal)",

        // Process regular clauses - assert into knowledge base
        "prologot_process_clause(Clause) :- assertz(Clause)",

        nullptr // Sentinel to mark end of array
    };

    // Get the assertz/1 predicate handle for asserting clauses
    predicate_t assert_pred = PL_predicate("assertz", 1, "user");

    // Assert each bootstrap predicate into Prolog
    for (int i = 0; predicates[i] != nullptr; i++)
    {
        // Create a term reference for the clause
        term_t clause = PL_new_term_ref();

        // Parse the Prolog code string into a term
        if (!PL_chars_to_term(predicates[i], clause))
        {
            PL_cleanup(0); // Clean up on failure
            return false;
        }

        // Assert the clause into the Prolog knowledge base
        if (!PL_call_predicate(NULL, PL_Q_NORMAL, assert_pred, clause))
        {
            PL_cleanup(0); // Clean up on failure
            return false;
        }
    }

    // Mark as initialized only after all steps succeed
    m_initialized = true;
    return true;
}

void Prologot::cleanup()
{
    if (m_initialized)
    {
        // PL_cleanup(0) shuts down the Prolog engine
        // The argument (0) means normal cleanup
        PL_cleanup(0);
        m_initialized = false;
    }
}

bool Prologot::is_initialized() const
{
    return m_initialized;
}

// =============================================================================
// File and Code Consultation
// =============================================================================

bool Prologot::consult(String const& p_filename)
{
    if (!m_initialized)
        return false;

    // Convert res:// and user:// paths to absolute filesystem paths
    // SWI-Prolog doesn't understand Godot's virtual filesystem paths
    String filename = p_filename;
    if (filename.begins_with("res://") || filename.begins_with("user://"))
    {
        ProjectSettings* project_settings = ProjectSettings::get_singleton();
        if (project_settings)
        {
            filename = project_settings->globalize_path(p_filename);
        }
    }

    // Get a handle to Prolog's built-in consult/1 predicate
    // "user" module is the default module for user-defined predicates
    predicate_t pred = PL_predicate("consult", 1, "user");

    // Allocate term references for the predicate arguments (1 argument:
    // filename)
    term_t args = PL_new_term_refs(1);

    // Set the filename argument as a Prolog atom
    if (!PL_put_atom_chars(args, filename.utf8().get_data()))
    {
        return false;
    }

    // Call consult/1 with the filename argument
    // PL_Q_NORMAL means normal query execution (not catching exceptions)
    int result = PL_call_predicate(NULL, PL_Q_NORMAL, pred, args);
    return result != 0; // Non-zero means success in SWI-Prolog API
}

bool Prologot::consult_string(String const& p_prolog_code)
{
    if (!m_initialized)
        return false;

    // Create a term reference and store the code as a Prolog string
    term_t t = PL_new_term_ref();
    if (!PL_put_string_chars(t, p_prolog_code.utf8().get_data()))
        return false;

    // Get handle to the bootstrap predicate created during initialization
    predicate_t pred = PL_predicate("load_program_from_string", 1, "user");

    // Prepare arguments for the predicate call
    term_t args = PL_new_term_refs(1);
    if (!PL_put_term(args, t))
        return false;

    // Open query with exception catching
    qid_t qid = PL_open_query(NULL, PL_Q_CATCH_EXCEPTION, pred, args);
    int result = PL_next_solution(qid);

    // Handle exceptions
    if (result == PL_S_EXCEPTION)
    {
        term_t exception = PL_exception(qid);
        if (exception)
        {
            char* exception_str;
            if (PL_get_chars(
                    exception, &exception_str, CVT_WRITE | CVT_EXCEPTION))
            {
                m_last_error = String(exception_str);
                godot::UtilityFunctions::push_error(
                    String("Prologot: Syntax error: ") + m_last_error);
            }
        }
        PL_close_query(qid);
        return false;
    }

    PL_close_query(qid);
    return result != 0;
}

// =============================================================================
// Query Execution
// =============================================================================

bool Prologot::query(String const& p_goal)
{
    if (!m_initialized)
        return false;

    // Parse the goal string into a Prolog term
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(p_goal.utf8().get_data(), t))
    {
        return false;
    }

    // Open a query using call/1 to execute the goal
    // PL_Q_NORMAL means normal execution (exceptions propagate)
    qid_t qid =
        PL_open_query(NULL, PL_Q_NORMAL, PL_predicate("call", 1, "user"), t);

    // Get the first solution (if any)
    int result = PL_next_solution(qid);

    // Always close the query to free resources
    PL_close_query(qid);

    return result != 0; // Non-zero means solution found
}

Array Prologot::query_all(String const& p_goal)
{
    Array results;
    if (!m_initialized)
        return results;

    // Use findall/3 to collect all solutions
    String findall_goal =
        "findall(" + p_goal + ", " + p_goal + ", PrologotResults__)";

    // Parse the findall query into a Prolog term
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(findall_goal.utf8().get_data(), t))
    {
        return results;
    }

    // Execute the findall query with exception handling
    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), t);

    int solution_result = PL_next_solution(qid);

    // Handle Prolog exceptions
    if (solution_result == PL_S_EXCEPTION)
    {
        term_t exception = PL_exception(qid);
        if (exception)
        {
            char* exception_str;
            if (PL_get_chars(
                    exception, &exception_str, CVT_WRITE | CVT_EXCEPTION))
            {
                m_last_error = String(exception_str);
                godot::UtilityFunctions::push_error(
                    String("Prologot: Query error: ") + m_last_error);
            }
        }
        PL_close_query(qid);
        return results;
    }

    // Extract results list
    if (solution_result)
    {
        term_t findall_term = PL_new_term_ref();
        if (PL_get_arg(3, t, findall_term))
        {
            term_t head = PL_new_term_ref();
            term_t tail = PL_copy_term_ref(findall_term);

            // Each solution is converted directly to a Variant
            while (PL_get_list(tail, head, tail))
            {
                results.push_back(term_to_variant(head));
            }
        }
    }

    PL_close_query(qid);
    return results;
}

Variant Prologot::query_one(String const& goal)
{
    if (!m_initialized)
        return Variant();

    // Parse the goal string into a Prolog term
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(goal.utf8().get_data(), t))
    {
        return Variant();
    }

    // Open a query with exception handling
    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), t);

    int result = PL_next_solution(qid);

    // Handle exceptions
    if (result == PL_S_EXCEPTION)
    {
        term_t exception = PL_exception(qid);
        if (exception)
        {
            char* exception_str;
            if (PL_get_chars(
                    exception, &exception_str, CVT_WRITE | CVT_EXCEPTION))
            {
                m_last_error = String(exception_str);
                godot::UtilityFunctions::push_error(
                    String("Prologot: Query error: ") + m_last_error);
            }
        }
        PL_close_query(qid);
        return Variant();
    }

    Variant var;
    if (result)
    {
        // Solution found: convert to Variant
        var = term_to_variant(t);
    }

    PL_close_query(qid);
    return var; // Returns null Variant if no solution
}

// =============================================================================
// Dynamic Assertions
// =============================================================================

bool Prologot::assert_fact(String const& p_fact)
{
    if (!m_initialized)
        return false;

    // Parse the fact string into a Prolog term
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(p_fact.utf8().get_data(), t))
    {
        return false;
    }

    // Assert the fact using Prolog's built-in assert/1 predicate
    // assert/1 adds the clause at the end of the predicate definition
    predicate_t pred = PL_predicate("assert", 1, "user");
    return PL_call_predicate(NULL, PL_Q_NORMAL, pred, t) != 0;
}

bool Prologot::retract_fact(String const& p_fact)
{
    if (!m_initialized)
        return false;

    // Parse the fact string into a Prolog term
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(p_fact.utf8().get_data(), t))
    {
        return false;
    }

    // Retract the fact using Prolog's built-in retract/1 predicate
    // retract/1 removes the first clause that unifies with the given term
    predicate_t pred = PL_predicate("retract", 1, "user");
    return PL_call_predicate(NULL, PL_Q_NORMAL, pred, t) != 0;
}

bool Prologot::retract_all(String const& p_functor)
{
    if (!m_initialized)
        return false;

    // Build and execute a retractall/1 goal
    // retractall/1 removes all clauses that unify with the given term
    String goal = "retractall(" + p_functor + ")";
    return query(goal);
}

// =============================================================================
// Predicate Manipulation
// =============================================================================

bool Prologot::call_predicate(String const& p_predicate, Array const& p_args)
{
    if (!m_initialized)
        return false;

    // Allocate term references for all arguments
    // PL_new_term_refs() allocates a contiguous array of term references
    term_t t = PL_new_term_refs(p_args.size());

    // Convert each Godot Variant argument to a Prolog term
    for (int i = 0; i < p_args.size(); i++)
    {
        term_t arg = variant_to_term(p_args[i]);
        // PL_put_term() copies the term into the argument slot
        // t + i is pointer arithmetic to access the i-th term reference
        if (!PL_put_term(t + i, arg))
            return false;
    }

    // Create the functor (predicate name + arity)
    // The functor represents the predicate signature
    functor_t f = PL_new_functor(PL_new_atom(p_predicate.utf8().get_data()),
                                 p_args.size());

    // Create the goal term by combining functor with arguments
    term_t goal = PL_new_term_ref();
    if (!PL_cons_functor_v(goal, f, t))
        return false;

    // Execute the goal
    // PL_call() executes a goal directly (simpler than
    // open_query/next_solution)
    return PL_call(goal, NULL) != 0;
}

Variant Prologot::call_function(String const& p_predicate, Array const& p_args)
{
    if (!m_initialized)
        return Variant();

    // Allocate term references for arguments plus one extra for the result
    // The result will be stored in the last term reference
    term_t t = PL_new_term_refs(p_args.size() + 1);

    // Convert each input argument to a Prolog term
    for (int i = 0; i < p_args.size(); i++)
    {
        term_t arg = variant_to_term(p_args[i]);
        if (!PL_put_term(t + i, arg))
            return Variant();
    }
    // Note: t + args.size() is left unbound - Prolog will bind it

    // Create the functor with arity = args.size() + 1 (includes result)
    functor_t f = PL_new_functor(PL_new_atom(p_predicate.utf8().get_data()),
                                 p_args.size() + 1);
    term_t goal = PL_new_term_ref();
    if (!PL_cons_functor_v(goal, f, t))
        return Variant();

    // Execute the goal
    // If successful, the last argument (t + args.size()) will be bound to the
    // result
    if (PL_call(goal, NULL))
    {
        // Extract and return the result (last argument)
        return term_to_variant(t + p_args.size());
    }

    return Variant(); // Call failed
}

// =============================================================================
// Introspection
// =============================================================================

bool Prologot::predicate_exists(String const& p_predicate, int p_arity)
{
    if (!m_initialized)
        return false;

    // PL_predicate() looks up a predicate by name and arity
    // Returns 0 (NULL) if the predicate doesn't exist
    // NULL as module means search in all modules
    predicate_t pred =
        PL_predicate(p_predicate.utf8().get_data(), p_arity, NULL);
    return pred != 0;
}

Array Prologot::list_predicates()
{
    Array predicates;
    if (!m_initialized)
        return predicates;

    // Query for all current predicates using Prolog's built-in
    // current_predicate/1 This returns all predicates in the form Name/Arity
    String goal = "current_predicate(Name/Arity)";
    Array results = query_all(goal);

    return results;
}

// =============================================================================
// Term Conversion
// =============================================================================

Variant Prologot::term_to_variant(term_t p_term)
{
    int type = PL_term_type(p_term);

    switch (type)
    {
        case PL_VARIABLE:
            // Unbound variables cannot be converted to a concrete value
            return Variant();

        case PL_ATOM:
        {
            // Convert Prolog atom to Godot String
            // Atoms are like symbols in other languages (e.g., 'foo', 'bar')
            char* s;
            if (!PL_get_atom_chars(p_term, &s))
                return Variant();
            return String(s);
        }

        case PL_INTEGER:
        {
            // Convert Prolog integer to int64_t
            int64_t i;
            if (!PL_get_int64(p_term, &i))
                return Variant();
            return i;
        }

        case PL_FLOAT:
        {
            // Convert Prolog float to double
            double d;
            if (!PL_get_float(p_term, &d))
                return Variant();
            return d;
        }

        case PL_STRING:
        {
            // Convert Prolog string to Godot String
            // Note: Prolog strings are different from atoms
            // Strings are "text" while atoms are 'symbols'
            char* s;
            size_t len;
            if (!PL_get_string_chars(p_term, &s, &len))
                return Variant();
            return String(s);
        }

        case PL_NIL:
        {
            // Empty list [] - return empty Godot Array
            return Array();
        }

        case PL_LIST_PAIR:
        {
            // Non-empty list [H|T] - convert to Godot Array
            // In modern SWI-Prolog, lists have their own type PL_LIST_PAIR
            Array list_array;
            term_t head = PL_new_term_ref();
            term_t tail = PL_copy_term_ref(p_term);

            while (PL_get_list(tail, head, tail))
            {
                list_array.push_back(term_to_variant(head));
            }

            return list_array;
        }

        case PL_TERM:
        {
            // Compound terms can be lists or structured terms
            // First, try to deconstruct as a list [Head|Tail]
            term_t list_copy = PL_copy_term_ref(p_term);
            term_t head = PL_new_term_ref();
            term_t tail = PL_new_term_ref();

            // Try to deconstruct as a list
            if (PL_get_list(list_copy, head, tail))
            {
                // It's a list! Convert to Godot Array
                Array list_array;

                // Add first element (head)
                list_array.push_back(term_to_variant(head));

                // Iterate through the rest of the list (tail)
                // PL_get_list() modifies tail to point to the next element
                while (PL_get_list(tail, head, tail))
                {
                    list_array.push_back(term_to_variant(head));
                }

                return list_array;
            }

            // Not a list, try as compound term (e.g., functor(arg1, arg2))
            atom_t name;
            size_t arity;
            if (PL_get_name_arity(p_term, &name, &arity))
            {
                // Check for empty list atom (special case)
                // Empty list can be represented as the atom []
                const char* atom_name = PL_atom_chars(name);
                if (arity == 0 && strcmp(atom_name, "[]") == 0)
                {
                    // Empty list represented as atom
                    return Array();
                }

                // Convert compound term to Dictionary format:
                // {"functor": "name", "args": [arg1, arg2, ...]}
                // This distinguishes compound terms from lists (which are
                // Arrays)
                Dictionary compound;
                compound["functor"] = String(atom_name);

                Array args;
                // Recursively convert each argument
                for (size_t i = 1; i <= arity; i++)
                {
                    term_t arg = PL_new_term_ref();
                    if (!PL_get_arg(i, p_term, arg))
                    {
                        // Failed to get argument, return invalid Variant
                        return Variant();
                    }
                    args.push_back(term_to_variant(arg));
                }
                compound["args"] = args;
                return compound;
            }
        }
    }

    // Unknown or unsupported type
    return Variant();
}

term_t Prologot::variant_to_term(Variant const& p_var)
{
    term_t t = PL_new_term_ref();

    switch (p_var.get_type())
    {
        case Variant::NIL:
            // Null becomes empty list atom
            if (!PL_put_atom_chars(t, "[]"))
            {
                return (term_t)0; // Return invalid term on failure
            }
            break;

        case Variant::BOOL:
            // Boolean to Prolog atom (true or false)
            // Prolog has built-in atoms for boolean values
            if (!PL_put_atom_chars(t, (bool)p_var ? "true" : "false"))
            {
                return (term_t)0; // Return invalid term on failure
            }
            break;

        case Variant::INT:
            // Convert integer to Prolog integer
            if (!PL_put_int64(t, (int64_t)p_var))
            {
                return (term_t)0; // Return invalid term on failure
            }
            break;

        case Variant::FLOAT:
            // Convert float to Prolog float
            if (!PL_put_float(t, (double)p_var))
            {
                return (term_t)0; // Return invalid term on failure
            }
            break;

        case Variant::STRING:
            // Convert GDScript strings to Prolog atoms (not strings)
            // This is important because Prolog atoms (foo) differ from strings
            // ("foo") Atoms are more commonly used in Prolog, so we use them by
            // default
            if (!PL_put_atom_chars(t, ((String)p_var).utf8().get_data()))
            {
                return (term_t)0; // Return invalid term on failure
            }
            break;

        case Variant::ARRAY:
        {
            // Array becomes Prolog list [elem1, elem2, ...]
            Array arr = p_var;
            if (arr.size() == 0)
            {
                // Empty array becomes empty list []
                if (!PL_put_nil(t))
                {
                    return (term_t)0;
                }
            }
            else
            {
                // Build list from end to start
                // Start with empty list, then prepend elements
                term_t list = PL_new_term_ref();
                if (!PL_put_nil(list))
                {
                    return (term_t)0;
                }

                // Iterate backwards to build the list correctly
                for (int i = arr.size() - 1; i >= 0; i--)
                {
                    term_t elem = variant_to_term(arr[i]);
                    if (!elem)
                    {
                        return (term_t)0;
                    }
                    term_t new_list = PL_new_term_ref();
                    if (!PL_cons_list(new_list, elem, list))
                    {
                        return (term_t)0;
                    }
                    list = new_list;
                }

                if (!PL_put_term(t, list))
                {
                    return (term_t)0;
                }
            }
            break;
        }

        case Variant::DICTIONARY:
        {
            // Dictionary with "functor" and "args" becomes compound term
            // Format: {"functor": "name", "args": [arg1, arg2, ...]}
            //      -> name(arg1, arg2, ...)
            Dictionary dict = p_var;
            if (dict.has("functor") && dict.has("args"))
            {
                String functor = dict["functor"];
                Array args_arr = dict["args"];
                int arity = args_arr.size();

                // Create the functor (predicate signature)
                functor_t f = PL_new_functor(
                    PL_new_atom(functor.utf8().get_data()), arity);

                // Allocate term references for arguments
                term_t args = PL_new_term_refs(arity);

                // Convert each argument recursively
                for (int i = 0; i < arity; i++)
                {
                    term_t arg = variant_to_term(args_arr[i]);
                    if (!arg || !PL_put_term(args + i, arg))
                    {
                        return (term_t)0;
                    }
                }

                // Construct the compound term from functor and arguments
                if (!PL_cons_functor_v(t, f, args))
                {
                    return (term_t)0;
                }
            }
            else
            {
                // Dictionary without proper structure becomes empty list
                if (!PL_put_atom_chars(t, "[]"))
                {
                    return (term_t)0;
                }
            }
            break;
        }

        default:
            // Unknown or unsupported types become empty list atom
            // This provides a safe fallback for unexpected types
            if (!PL_put_atom_chars(t, "[]"))
            {
                return (term_t)0; // Return invalid term on failure
            }
    }

    return t;
}

// =============================================================================
// Singleton Access
// =============================================================================

Prologot* Prologot::get_singleton()
{
    return m_singleton;
}

String Prologot::get_last_error() const
{
    return m_last_error;
}
