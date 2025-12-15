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
#include <vector>
#ifdef _WIN32
#    include <cstdlib> // For _putenv_s on Windows
#else
#    include <unistd.h> // For setenv on Unix
#endif
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
    ClassDB::bind_method(D_METHOD("initialize", "options"),
                         &Prologot::initialize,
                         DEFVAL(Dictionary()));
    ClassDB::bind_method(D_METHOD("cleanup"), &Prologot::cleanup);
    ClassDB::bind_method(D_METHOD("is_initialized"), &Prologot::is_initialized);

    // File/code loading methods
    ClassDB::bind_method(D_METHOD("consult_file", "filename"),
                         &Prologot::consult_file);
    ClassDB::bind_method(D_METHOD("consult_string", "prolog_code"),
                         &Prologot::consult_string);

    // Query methods
    ClassDB::bind_method(D_METHOD("query", "predicate", "args"),
                         &Prologot::query,
                         DEFVAL(Array()));
    ClassDB::bind_method(D_METHOD("query_all", "predicate", "args"),
                         &Prologot::query_all,
                         DEFVAL(Array()));
    ClassDB::bind_method(D_METHOD("query_one", "predicate", "args"),
                         &Prologot::query_one,
                         DEFVAL(Array()));

    // Dynamic assertion methods
    ClassDB::bind_method(D_METHOD("add_fact", "fact"), &Prologot::add_fact);
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
    m_on_error = "print";
    m_on_warning = "print";
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

void Prologot::set_swi_home_dir(String const& p_prolog_home)
{
    if (p_prolog_home.is_empty())
        return;

#ifdef _WIN32
    _putenv_s("SWI_HOME_DIR", p_prolog_home.utf8().get_data());
#else
    setenv("SWI_HOME_DIR", p_prolog_home.utf8().get_data(), 1);
#endif
}

bool Prologot::initialize(Dictionary const& p_options)
{
    // Idempotent: if already initialized, return success immediately
    if (m_initialized)
        return true;

    // Extract all options with defaults
    String home = p_options.get("home", "");
    bool quiet = p_options.get("quiet", true);
    bool optimised = p_options.get("optimised", false);
    bool traditional = p_options.get("traditional", false);
    bool threads = p_options.get("threads", true);
    bool packs = p_options.get("packs", true);

    String on_error = p_options.get("on error", "print");
    String on_warning = p_options.get("on warning", "print");

    // Store error handling options
    m_on_error = on_error;
    m_on_warning = on_warning;

    String stack_limit = p_options.get("stack limit", "");
    String table_space = p_options.get("table space", "");
    String shared_table_space = p_options.get("shared table space", "");

    String init_file = p_options.get("init file", "");
    String script_file = p_options.get("script file", "");
    String toplevel = p_options.get("toplevel", "");
    Variant goal_var = p_options.get("goal", Variant());

    // Set SWI-Prolog home directory if specified
    // This allows custom SWI-Prolog installations
    set_swi_home_dir(home);

    // Build argv dynamically with string_storage for lifecycle management
    std::vector<String> string_storage;
    std::vector<const char*> argv_list;
    argv_list.push_back("godot");

    // Boolean options
    if (quiet)
        argv_list.push_back("--quiet");
    if (optimised)
        argv_list.push_back("-O");
    if (traditional)
        argv_list.push_back("--traditional");
    if (!threads)
        argv_list.push_back("--no-threads");
    if (!packs)
        argv_list.push_back("--no-packs");

    // Options with values (format --option=value)
    if (!on_error.is_empty() && on_error != "print")
    {
        string_storage.push_back("--on-error=" + on_error);
        argv_list.push_back(string_storage.back().utf8().get_data());
    }
    if (!on_warning.is_empty() && on_warning != "print")
    {
        string_storage.push_back("--on-warning=" + on_warning);
        argv_list.push_back(string_storage.back().utf8().get_data());
    }
    if (!stack_limit.is_empty())
    {
        string_storage.push_back("--stack-limit=" + stack_limit);
        argv_list.push_back(string_storage.back().utf8().get_data());
    }
    if (!table_space.is_empty())
    {
        string_storage.push_back("--table-space=" + table_space);
        argv_list.push_back(string_storage.back().utf8().get_data());
    }
    if (!shared_table_space.is_empty())
    {
        string_storage.push_back("--shared-table-space=" + shared_table_space);
        argv_list.push_back(string_storage.back().utf8().get_data());
    }

    // Files and toplevel
    if (!init_file.is_empty())
    {
        argv_list.push_back("-f");
        string_storage.push_back(init_file);
        argv_list.push_back(string_storage.back().utf8().get_data());
    }
    if (!script_file.is_empty())
    {
        argv_list.push_back("-l");
        string_storage.push_back(script_file);
        argv_list.push_back(string_storage.back().utf8().get_data());
    }
    if (!toplevel.is_empty())
    {
        argv_list.push_back("-t");
        string_storage.push_back(toplevel);
        argv_list.push_back(string_storage.back().utf8().get_data());
    }

    // Goals (-g can be repeated)
    if (goal_var.get_type() == Variant::STRING)
    {
        String goal = goal_var;
        if (!goal.is_empty())
        {
            argv_list.push_back("-g");
            string_storage.push_back(goal);
            argv_list.push_back(string_storage.back().utf8().get_data());
        }
    }
    else if (goal_var.get_type() == Variant::ARRAY)
    {
        Array goals = goal_var;
        for (int i = 0; i < goals.size(); i++)
        {
            String goal = goals[i];
            argv_list.push_back("-g");
            string_storage.push_back(goal);
            argv_list.push_back(string_storage.back().utf8().get_data());
        }
    }

    // Prolog flags (-D name=value)
    if (p_options.has("prolog flags"))
    {
        Dictionary flags = p_options.get("prolog flags", Dictionary());
        Array keys = flags.keys();
        for (int i = 0; i < keys.size(); i++)
        {
            String key = keys[i];
            String value = flags[key];
            argv_list.push_back("-D");
            string_storage.push_back(key + "=" + value);
            argv_list.push_back(string_storage.back().utf8().get_data());
        }
    }

    // File search paths (-p alias=path)
    if (p_options.has("file search paths"))
    {
        Dictionary paths = p_options.get("file search paths", Dictionary());
        Array keys = paths.keys();
        for (int i = 0; i < keys.size(); i++)
        {
            String alias = keys[i];
            String path = paths[alias];
            argv_list.push_back("-p");
            string_storage.push_back(alias + "=" + path);
            argv_list.push_back(string_storage.back().utf8().get_data());
        }
    }

    // Custom arguments
    if (p_options.has("custom args"))
    {
        Array custom_args = p_options.get("custom args", Array());
        for (int i = 0; i < custom_args.size(); i++)
        {
            string_storage.push_back(String(custom_args[i]));
            argv_list.push_back(string_storage.back().utf8().get_data());
        }
    }

    argv_list.push_back(nullptr);

    // Initialize the Prolog engine
    // PL_initialise() starts the Prolog runtime system
    if (!PL_initialise(argv_list.size() - 1, (char**)argv_list.data()))
    {
        if (!handle_prolog_exception(0, "PL_initialise"))
        {
            m_last_error = "PL_initialise() failed (no details available)";
        }

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
            m_last_error = String("Failed to parse bootstrap predicate: ") + String(predicates[i]);
            PL_cleanup(0); // Clean up on failure
            return false;
        }

        // Assert the clause into the Prolog knowledge base
        // Use exception catching to avoid interactive mode even during
        // bootstrap
        qid_t qid =
            PL_open_query(NULL, PL_Q_CATCH_EXCEPTION, assert_pred, clause);
        int result = PL_next_solution(qid);

        if (result == PL_S_EXCEPTION || !result)
        {
            // Try to get exception details
            term_t ex = PL_exception(qid);
            if (ex)
            {
                char* msg;
                if (PL_get_chars(ex, &msg, CVT_WRITE | BUF_DISCARDABLE | REP_UTF8))
                {
                    m_last_error = String("Failed to assert bootstrap predicate: ") + String(msg);
                }
                else
                {
                    m_last_error = String("Failed to assert bootstrap predicate: ") + String(predicates[i]);
                }
            }
            else
            {
                m_last_error = String("Failed to assert bootstrap predicate: ") + String(predicates[i]);
            }
            PL_close_query(qid);
            PL_cleanup(0); // Clean up on failure
            return false;
        }

        PL_close_query(qid);
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

bool Prologot::consult_file(String const& p_filename)
{
    if (!m_initialized)
        return false;

    // Validate input
    if (p_filename.is_empty())
    {
        m_last_error = "Empty filename";
        return false;
    }

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
        m_last_error = "Failed to convert filename to Prolog atom";
        return false;
    }

    // Call consult/1 with exception catching to avoid interactive mode
    qid_t qid = PL_open_query(NULL, PL_Q_CATCH_EXCEPTION, pred, args);
    int result = PL_next_solution(qid);

    // Handle exceptions
    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Consult");
        PL_close_query(qid);
        return false;
    }

    PL_close_query(qid);
    return result != 0; // Non-zero means success in SWI-Prolog API
}

bool Prologot::consult_string(String const& p_prolog_code)
{
    if (!m_initialized)
        return false;

    // Validate input
    if (p_prolog_code.is_empty())
    {
        m_last_error = "Empty Prolog code";
        return false;
    }

    // Create a term reference and store the code as a Prolog string
    term_t t = PL_new_term_ref();
    if (!PL_put_string_chars(t, p_prolog_code.utf8().get_data()))
    {
        m_last_error = "Failed to convert code to Prolog string";
        return false;
    }

    // Get handle to the bootstrap predicate created during initialization
    predicate_t pred = PL_predicate("load_program_from_string", 1, "user");

    // Prepare arguments for the predicate call
    term_t args = PL_new_term_refs(1);
    if (!PL_put_term(args, t))
    {
        m_last_error = "Failed to prepare arguments";
        return false;
    }

    // Open query with exception catching
    qid_t qid = PL_open_query(NULL, PL_Q_CATCH_EXCEPTION, pred, args);
    int result = PL_next_solution(qid);

    // Handle exceptions
    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Consult string");
        PL_close_query(qid);
        return false;
    }

    PL_close_query(qid);
    return result != 0;
}

// =============================================================================
// Query Execution
// =============================================================================

String Prologot::build_query(String const& p_predicate, Array const& p_args)
{
    String predicate = p_predicate;

    // Remove trailing period if present (users might include it by mistake)
    if (predicate.length() > 0 && predicate[predicate.length() - 1] == '.')
    {
        predicate = predicate.substr(0, predicate.length() - 1);
    }

    if (p_args.is_empty())
    {
        // If no args, assume predicate is already a full goal
        return predicate;
    }

    // Build query: predicate(arg1, arg2, ...)
    String query = predicate + String("(");
    for (int i = 0; i < p_args.size(); i++)
    {
        if (i > 0)
            query += String(", ");

        Variant arg = p_args[i];
        if (arg.get_type() == Variant::STRING)
        {
            // Assume it's a variable name (starts with uppercase or _)
            String var_name = arg;
            if (var_name.length() > 0 &&
                    (var_name[0] >= 'A' && var_name[0] <= 'Z') ||
                var_name[0] == '_')
            {
                query += var_name;
            }
            else
            {
                // It's a value, quote it if needed
                query += var_name;
            }
        }
        else
        {
            // Convert to string representation
            query += String(arg);
        }
    }
    query += String(")");
    return query;
}

bool Prologot::query(String const& p_predicate, Array const& p_args)
{
    if (!m_initialized)
        return false;

    // Build the query string (build_query automatically removes trailing
    // periods)
    String goal = build_query(p_predicate, p_args);

    // Validate input
    if (goal.is_empty())
    {
        m_last_error = "Empty query";
        return false;
    }

    // Parse the goal string into a Prolog term
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(goal.utf8().get_data(), t))
    {
        m_last_error = "Failed to parse query: " + goal;
        return false;
    }

    // Open a query using call/1 to execute the goal
    // Use PL_Q_CATCH_EXCEPTION to capture exceptions and avoid interactive mode
    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), t);

    // Get the first solution (if any)
    int result = PL_next_solution(qid);

    // Handle exceptions
    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Query");
        PL_close_query(qid);
        return false;
    }

    // Always close the query to free resources
    PL_close_query(qid);

    return result != 0; // Non-zero means solution found
}

Dictionary Prologot::extract_variables(term_t p_term, Array const& p_variables)
{
    Dictionary result;

    // Check if p_term is a compound term
    atom_t name;
    size_t arity;
    if (!PL_get_name_arity(p_term, &name, &arity))
        return result;

    // Extract each variable from the term arguments
    for (int i = 0; i < p_variables.size() && i < (int)arity; i++)
    {
        String var_name = p_variables[i];
        term_t arg = PL_new_term_ref();
        if (PL_get_arg(i + 1, p_term, arg))
        {
            Variant value = term_to_variant(arg);
            result[var_name] = value;
        }
    }

    return result;
}

Array Prologot::query_all(String const& p_predicate, Array const& p_args)
{
    Array results;
    if (!m_initialized)
        return results;

    // Build the query string (build_query automatically removes trailing
    // periods)
    String goal = build_query(p_predicate, p_args);

    // Validate input
    if (goal.is_empty())
    {
        m_last_error = "Empty query";
        return results;
    }

    // Check if we need to extract variables
    bool extract_vars = !p_args.is_empty();
    // Check if all args are strings (variable names)
    for (int i = 0; i < p_args.size(); i++)
    {
        if (p_args[i].get_type() != Variant::STRING)
        {
            extract_vars = false;
            break;
        }
        String var_name = p_args[i];
        // Variable names in Prolog start with uppercase or underscore
        if (var_name.length() == 0 ||
            ((var_name[0] < 'A' || var_name[0] > 'Z') && var_name[0] != '_'))
        {
            extract_vars = false;
            break;
        }
    }

    // Use findall/3 to collect all solutions
    String findall_goal =
        "findall(" + goal + ", " + goal + ", PrologotResults__)";

    // Parse the findall query into a Prolog term
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(findall_goal.utf8().get_data(), t))
    {
        m_last_error = "Failed to parse query: " + goal;
        return results;
    }

    // Execute the findall query with exception handling
    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), t);

    int solution_result = PL_next_solution(qid);

    // Handle Prolog exceptions
    if (solution_result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Query all");
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

            // Each solution is converted to a Variant or Dictionary
            while (PL_get_list(tail, head, tail))
            {
                if (extract_vars)
                {
                    // Extract variables into a Dictionary
                    Dictionary var_dict = extract_variables(head, p_args);
                    results.push_back(var_dict);
                }
                else
                {
                    // Convert directly to Variant
                    results.push_back(term_to_variant(head));
                }
            }
        }
    }

    PL_close_query(qid);
    return results;
}

Variant Prologot::query_one(String const& p_predicate, Array const& p_args)
{
    if (!m_initialized)
        return Variant();

    // Build the query string (build_query automatically removes trailing
    // periods)
    String goal = build_query(p_predicate, p_args);

    // Validate input
    if (goal.is_empty())
    {
        m_last_error = "Empty query";
        return Variant();
    }

    // Check if we need to extract variables
    bool extract_vars = !p_args.is_empty();
    for (int i = 0; i < p_args.size(); i++)
    {
        if (p_args[i].get_type() != Variant::STRING)
        {
            extract_vars = false;
            break;
        }
        String var_name = p_args[i];
        if (var_name.length() == 0 ||
            ((var_name[0] < 'A' || var_name[0] > 'Z') && var_name[0] != '_'))
        {
            extract_vars = false;
            break;
        }
    }

    // Parse the goal string into a Prolog term
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(goal.utf8().get_data(), t))
    {
        m_last_error = "Failed to parse query: " + goal;
        return Variant();
    }

    // Open a query with exception handling
    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), t);

    int result = PL_next_solution(qid);

    // Handle exceptions
    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Query one");
        PL_close_query(qid);
        return Variant();
    }

    Variant var;
    if (result)
    {
        // Solution found: convert to Variant or Dictionary
        if (extract_vars)
        {
            var = extract_variables(t, p_args);
        }
        else
        {
            var = term_to_variant(t);
        }
    }

    PL_close_query(qid);
    return var; // Returns null Variant if no solution
}

// =============================================================================
// Dynamic Assertions
// =============================================================================

bool Prologot::add_fact(String const& p_fact)
{
    if (!m_initialized)
        return false;

    // Validate input
    if (p_fact.is_empty())
    {
        m_last_error = "Empty fact";
        return false;
    }

    // Remove trailing period if present (users might include it by mistake)
    String fact = p_fact;
    if (fact.length() > 0 && fact[fact.length() - 1] == '.')
    {
        fact = fact.substr(0, fact.length() - 1);
    }

    // Parse the fact string into a Prolog term
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(fact.utf8().get_data(), t))
    {
        m_last_error = "Failed to parse fact: " + fact;
        return false;
    }

    // Assert the fact using Prolog's built-in assert/1 predicate
    // assert/1 adds the clause at the end of the predicate definition
    predicate_t pred = PL_predicate("assert", 1, "user");

    // Use exception catching to avoid interactive mode on syntax errors
    qid_t qid = PL_open_query(NULL, PL_Q_CATCH_EXCEPTION, pred, t);
    int result = PL_next_solution(qid);

    // Handle exceptions
    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Assert fact");
        PL_close_query(qid);
        return false;
    }

    PL_close_query(qid);
    return result != 0;
}

bool Prologot::retract_fact(String const& p_fact)
{
    if (!m_initialized)
        return false;

    // Validate input
    if (p_fact.is_empty())
    {
        m_last_error = "Empty fact";
        return false;
    }

    // Remove trailing period if present (users might include it by mistake)
    String fact = p_fact;
    if (fact.length() > 0 && fact[fact.length() - 1] == '.')
    {
        fact = fact.substr(0, fact.length() - 1);
    }

    // Parse the fact string into a Prolog term
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(fact.utf8().get_data(), t))
    {
        m_last_error = "Failed to parse fact: " + fact;
        return false;
    }

    // Retract the fact using Prolog's built-in retract/1 predicate
    // retract/1 removes the first clause that unifies with the given term
    predicate_t pred = PL_predicate("retract", 1, "user");

    // Use exception catching to avoid interactive mode on syntax errors
    qid_t qid = PL_open_query(NULL, PL_Q_CATCH_EXCEPTION, pred, t);
    int result = PL_next_solution(qid);

    // Handle exceptions
    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Retract fact");
        PL_close_query(qid);
        return false;
    }

    PL_close_query(qid);
    return result != 0;
}

bool Prologot::retract_all(String const& p_functor)
{
    if (!m_initialized)
        return false;

    // Remove trailing period if present (users might include it by mistake)
    String functor = p_functor;
    if (functor.length() > 0 && functor[functor.length() - 1] == '.')
    {
        functor = functor.substr(0, functor.length() - 1);
    }

    // Build and execute a retractall/1 goal
    // retractall/1 removes all clauses that unify with the given term
    String goal = String("retractall(") + functor + String(")");
    return query(goal);
}

// =============================================================================
// Predicate Manipulation
// =============================================================================

bool Prologot::call_predicate(String const& p_predicate, Array const& p_args)
{
    if (!m_initialized)
        return false;

    // Validate input
    if (p_predicate.is_empty())
    {
        m_last_error = "Empty predicate name";
        return false;
    }

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
        {
            m_last_error = "Failed to convert argument " + String::num_int64(i);
            return false;
        }
    }

    // Create the functor (predicate name + arity)
    // The functor represents the predicate signature
    functor_t f = PL_new_functor(PL_new_atom(p_predicate.utf8().get_data()),
                                 p_args.size());

    // Create the goal term by combining functor with arguments
    term_t goal = PL_new_term_ref();
    if (!PL_cons_functor_v(goal, f, t))
    {
        m_last_error = "Failed to construct predicate term";
        return false;
    }

    // Execute the goal with exception catching to avoid interactive mode
    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), goal);
    int result = PL_next_solution(qid);

    // Handle exceptions
    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Call predicate");
        PL_close_query(qid);
        return false;
    }

    PL_close_query(qid);
    return result != 0;
}

Variant Prologot::call_function(String const& p_predicate, Array const& p_args)
{
    if (!m_initialized)
        return Variant();

    // Validate input
    if (p_predicate.is_empty())
    {
        m_last_error = "Empty predicate name";
        return Variant();
    }

    // Allocate term references for arguments plus one extra for the result
    // The result will be stored in the last term reference
    term_t t = PL_new_term_refs(p_args.size() + 1);

    // Convert each input argument to a Prolog term
    for (int i = 0; i < p_args.size(); i++)
    {
        term_t arg = variant_to_term(p_args[i]);
        if (!PL_put_term(t + i, arg))
        {
            m_last_error = "Failed to convert argument " + String::num_int64(i);
            return Variant();
        }
    }
    // Note: t + args.size() is left unbound - Prolog will bind it

    // Create the functor with arity = args.size() + 1 (includes result)
    functor_t f = PL_new_functor(PL_new_atom(p_predicate.utf8().get_data()),
                                 p_args.size() + 1);
    term_t goal = PL_new_term_ref();
    if (!PL_cons_functor_v(goal, f, t))
    {
        m_last_error = "Failed to construct predicate term";
        return Variant();
    }

    // Execute the goal with exception catching to avoid interactive mode
    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), goal);
    int result = PL_next_solution(qid);

    // Handle exceptions
    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Call function");
        PL_close_query(qid);
        return Variant();
    }

    Variant var;
    if (result)
    {
        // Extract and return the result (last argument)
        var = term_to_variant(t + p_args.size());
    }

    PL_close_query(qid);
    return var;
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
// Exception Handling
// =============================================================================

void Prologot::push_error(String const& p_message, String const& p_type)
{
    // Store error in m_last_error
    m_last_error = p_message;

    // Determine which option to check
    String option = (p_type == "warning") ? m_on_warning : m_on_error;

    // Handle according to option
    if (option == "print")
    {
        godot::UtilityFunctions::push_error("Prologot: " + p_message);
    }
    else if (option == "halt")
    {
        godot::UtilityFunctions::push_error("Prologot: " + p_message);
        // Note: We can't actually halt execution in Godot, but we log the error
    }
    // "status" option: only store in m_last_error, don't print
}

bool Prologot::handle_prolog_exception(qid_t p_qid, String const& p_context)
{
    term_t exception = PL_exception(p_qid);
    if (exception)
    {
        char* exception_str;
        if (PL_get_chars(exception, &exception_str, CVT_WRITE | CVT_EXCEPTION | BUF_DISCARDABLE))
        {
            String error_msg =
                p_context + String(" error: ") + String(exception_str);
            push_error(error_msg, "error");
            return true;
        }
        return false;
    }
    return false;
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
