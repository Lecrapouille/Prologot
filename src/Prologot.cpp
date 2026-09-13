/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file implements the Prologot class methods.
 */

#include "Prologot.hpp"
#include <cstring>
#include <map>
#include <string>
#include <vector>
#ifdef _WIN32
#    include <cstdlib>   // For _putenv_s on Windows
#    include <windows.h> // For GetModuleFileName
#else
#    include <dlfcn.h>  // For dladdr to find library path
#    include <unistd.h> // For setenv on Unix
#endif
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector3i.hpp>

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

    ClassDB::bind_method(D_METHOD("atom", "name"), &Prologot::atom);
    ClassDB::bind_method(D_METHOD("integer", "value"), &Prologot::integer);
    ClassDB::bind_method(D_METHOD("real", "value"), &Prologot::real);
    ClassDB::bind_method(D_METHOD("string", "value"), &Prologot::string);
    ClassDB::bind_method(D_METHOD("nil"), &Prologot::nil);
    ClassDB::bind_method(D_METHOD("list", "items"), &Prologot::list);
    ClassDB::bind_method(D_METHOD("compound", "functor", "args"),
                         &Prologot::compound);
    ClassDB::bind_method(
        D_METHOD("variable", "name"), &Prologot::variable, DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("anonymous"), &Prologot::anonymous);
    ClassDB::bind_method(D_METHOD("predicate", "name"), &Prologot::predicate);
    ClassDB::bind_method(D_METHOD("object", "value"), &Prologot::object);

    // High-level structured solving
    ClassDB::bind_method(D_METHOD("solve", "goal"), &Prologot::solve);

    // Editor console only (not a game API)
    ClassDB::bind_method(D_METHOD("_editor_query", "goal"),
                         &Prologot::query_text_named);

    // Dynamic assertion methods
    ClassDB::bind_method(D_METHOD("assert_fact", "goal"),
                         &Prologot::assert_fact);
    ClassDB::bind_method(D_METHOD("retract_fact", "goal"),
                         &Prologot::retract_fact);
    ClassDB::bind_method(D_METHOD("retract_all", "goal"),
                         &Prologot::retract_all);

    // Introspection methods
    ClassDB::bind_method(D_METHOD("predicate_exists", "predicate", "arity"),
                         &Prologot::predicate_exists);
    ClassDB::bind_method(D_METHOD("list_predicates"),
                         &Prologot::list_predicates);
    ClassDB::bind_method(
        D_METHOD("expose_property", "class_name", "property", "predicate"),
        &Prologot::expose_property,
        DEFVAL(String()));
    ClassDB::bind_method(
        D_METHOD("expose_method", "class_name", "method", "predicate"),
        &Prologot::expose_method,
        DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("unexpose", "predicate", "arity"),
                         &Prologot::unexpose);
    ClassDB::bind_method(D_METHOD("list_exposed"), &Prologot::list_exposed);

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

String Prologot::resolve_godot_path(String const& p_path)
{
    if (p_path.begins_with("res://") || p_path.begins_with("user://"))
    {
        return godot::ProjectSettings::get_singleton()->globalize_path(p_path);
    }
    return p_path;
}

std::pair<String, String>
Prologot::set_swi_home_dir(String const& p_home_option)
{
    if (p_home_option.is_empty())
        return std::make_pair("", ""); // Use system default SWI-Prolog

    String path = resolve_godot_path(p_home_option);

    if (!godot::FileAccess::file_exists(path + "/boot.prc"))
    {
        String out_error = "boot.prc not found in: " + path;
        return std::make_pair("", out_error);
    }

    return std::make_pair(path, "");
}

bool Prologot::initialize(Dictionary const& p_options)
{
    // Idempotent: if already initialized, return success immediately
    if (m_initialized)
        return true;

    // Extract other options
    bool quiet = p_options.get("quiet", true);
    bool optimized = p_options.get("optimized", false);
    bool traditional = p_options.get("traditional", false);
    bool threads = p_options.get("threads", true);
    bool packs = p_options.get("packs", true);
    String stack_limit = p_options.get("stack limit", "");
    String table_space = p_options.get("table space", "");
    String shared_table_space = p_options.get("shared table space", "");
    String init_file = p_options.get("init file", "");
    String script_file = p_options.get("script file", "");
    String toplevel = p_options.get("toplevel", "");
    Variant goal_var = p_options.get("goal", Variant());
    m_on_error = p_options.get("on error", "print");
    m_on_warning = p_options.get("on warning", "print");

    // Extract and resolve home directory
    auto [home, error] = set_swi_home_dir(p_options.get("home", ""));
    if (!error.is_empty())
    {
        m_last_error = error;
        push_error("Invalid SWI-Prolog home directory: " + error +
                   ". I will try to use the default one.");
    }

    // Build argv for PL_initialise
    // Note: Use std::string storage to keep char* pointers valid
    std::vector<std::string> string_storage;
    std::vector<const char*> argv_list;
    argv_list.push_back("godot");

    if (!home.is_empty())
    {
        string_storage.push_back(("--home=" + home).utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }

    // Boolean options
    if (quiet)
        argv_list.push_back("--quiet");
    if (optimized)
        argv_list.push_back("-O");
    if (traditional)
        argv_list.push_back("--traditional");
    if (!threads)
        argv_list.push_back("--no-threads");
    if (!packs)
        argv_list.push_back("--no-packs");

    // Options with values (format --option=value)
    if (!m_on_error.is_empty() && m_on_error != "print")
    {
        string_storage.push_back(
            ("--on-error=" + m_on_error).utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }
    if (!m_on_warning.is_empty() && m_on_warning != "print")
    {
        string_storage.push_back(
            ("--on-warning=" + m_on_warning).utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }
    if (!stack_limit.is_empty())
    {
        string_storage.push_back(
            ("--stack-limit=" + stack_limit).utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }
    if (!table_space.is_empty())
    {
        string_storage.push_back(
            ("--table-space=" + table_space).utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }
    if (!shared_table_space.is_empty())
    {
        string_storage.push_back(
            ("--shared-table-space=" + shared_table_space).utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }

    // Files and toplevel
    if (!init_file.is_empty())
    {
        argv_list.push_back("-f");
        string_storage.push_back(init_file.utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }
    if (!script_file.is_empty())
    {
        argv_list.push_back("-l");
        string_storage.push_back(script_file.utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }
    if (!toplevel.is_empty())
    {
        argv_list.push_back("-t");
        string_storage.push_back(toplevel.utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }

    // Goals (-g can be repeated)
    if (goal_var.get_type() == Variant::STRING)
    {
        String goal = goal_var;
        if (!goal.is_empty())
        {
            argv_list.push_back("-g");
            string_storage.push_back(goal.utf8().get_data());
            argv_list.push_back(string_storage.back().c_str());
        }
    }
    else if (goal_var.get_type() == Variant::ARRAY)
    {
        Array goals = goal_var;
        for (int i = 0; i < goals.size(); i++)
        {
            String goal = goals[i];
            argv_list.push_back("-g");
            string_storage.push_back(goal.utf8().get_data());
            argv_list.push_back(string_storage.back().c_str());
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
            string_storage.push_back((key + "=" + value).utf8().get_data());
            argv_list.push_back(string_storage.back().c_str());
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
            string_storage.push_back((alias + "=" + path).utf8().get_data());
            argv_list.push_back(string_storage.back().c_str());
        }
    }

    // Custom arguments
    if (p_options.has("custom args"))
    {
        Array custom_args = p_options.get("custom args", Array());
        for (int i = 0; i < custom_args.size(); i++)
        {
            string_storage.push_back(String(custom_args[i]).utf8().get_data());
            argv_list.push_back(string_storage.back().c_str());
        }
    }

    argv_list.push_back(nullptr);

    // Initialize Prolog engine
    if (!PL_initialise(argv_list.size() - 1, (char**)argv_list.data()))
    {
        if (!handle_prolog_exception(0, "PL_initialise"))
        {
            m_last_error = "PL_initialise() failed (no details available)";
        }

        return false;
    }

    PrologObject::register_blob_type();
    if (!register_foreign_predicates())
    {
        m_last_error = "Failed to register Prologot foreign predicates";
        PL_cleanup(0);
        return false;
    }

    // Log which SWI_HOME_DIR is being used by Prolog
    term_t home_term = PL_new_term_ref();
    predicate_t flag_pred = PL_predicate("current_prolog_flag", 2, "system");
    term_t flag_args = PL_new_term_refs(2);
    PL_put_atom_chars(flag_args + 0, "home");
    if (PL_call_predicate(NULL, PL_Q_NORMAL, flag_pred, flag_args))
    {
        char* home_path;
        if (PL_get_chars(flag_args + 1, &home_path, CVT_ATOM | CVT_STRING))
        {
            UtilityFunctions::print("[Prologot] SWI-Prolog HOME: ", home_path);
        }
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
            m_last_error = String("Failed to parse bootstrap predicate: ") +
                           String(predicates[i]);
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
                if (PL_get_chars(
                        ex, &msg, CVT_WRITE | BUF_DISCARDABLE | REP_UTF8))
                {
                    m_last_error =
                        String("Failed to assert bootstrap predicate: ") +
                        String(msg);
                }
                else
                {
                    m_last_error =
                        String("Failed to assert bootstrap predicate: ") +
                        String(predicates[i]);
                }
            }
            else
            {
                m_last_error =
                    String("Failed to assert bootstrap predicate: ") +
                    String(predicates[i]);
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
        m_exposed.clear();
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
    String filename = resolve_godot_path(p_filename);

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
    if (result == 0 && m_last_error.is_empty())
        m_last_error = "Failed to consult file: " + filename;
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
// Structured term factories
// =============================================================================

Ref<PrologTerm> Prologot::atom(String const& p_name)
{
    return PrologTerm::make_atom(p_name);
}

Ref<PrologTerm> Prologot::integer(int64_t p_value)
{
    return PrologTerm::make_integer(p_value);
}

Ref<PrologTerm> Prologot::real(double p_value)
{
    return PrologTerm::make_real(p_value);
}

Ref<PrologTerm> Prologot::string(String const& p_value)
{
    return PrologTerm::make_string(p_value);
}

Ref<PrologTerm> Prologot::nil()
{
    return PrologTerm::make_nil();
}

Ref<PrologTerm> Prologot::list(Array const& p_items)
{
    return PrologTerm::make_list(p_items);
}

Ref<PrologTerm> Prologot::compound(String const& p_functor, Array const& p_args)
{
    return PrologTerm::make_compound(p_functor, p_args);
}

Ref<PrologVariable> Prologot::variable(String const& p_name)
{
    return PrologVariable::create(p_name);
}

Ref<PrologVariable> Prologot::anonymous()
{
    return PrologVariable::create_anonymous();
}

Ref<PrologPredicate> Prologot::predicate(String const& p_name)
{
    return PrologPredicate::create(p_name);
}

Ref<PrologObject> Prologot::object(Object* p_object)
{
    return PrologObject::create(p_object);
}

// =============================================================================
// High-level solving (structured terms)
// =============================================================================

term_t Prologot::object_arg_to_term(Variant const& p_arg,
                                    std::map<int64_t, term_t>& p_vars,
                                    std::vector<Ref<PrologVariable>>& p_order)
{
    if (p_arg.get_type() == Variant::OBJECT)
    {
        Ref<PrologVariable> variable = p_arg;
        if (variable.is_valid())
        {
            if (variable->is_anonymous())
            {
                term_t fresh = PL_new_term_ref();
                if (!PL_put_variable(fresh))
                    return (term_t)0;
                return fresh;
            }
            auto it = p_vars.find(variable->get_id());
            if (it != p_vars.end())
                return it->second;
            term_t var = PL_new_term_ref();
            if (!PL_put_variable(var))
                return (term_t)0;
            p_vars[variable->get_id()] = var;
            p_order.push_back(variable);
            return var;
        }

        Ref<PrologGoal> nested = p_arg;
        if (nested.is_valid())
        {
            term_t goal = PL_new_term_ref();
            if (!compile_goal(nested, goal, p_vars, p_order))
                return (term_t)0;
            return goal;
        }

        Ref<PrologTerm> term = p_arg;
        if (term.is_valid())
            return prolog_term_to_swi(term, p_vars, p_order);

        Ref<PrologObject> handle = p_arg;
        if (handle.is_valid())
            return handle->to_swi_term();

        Object* native = p_arg;
        if (native)
            return godot_object_to_term(native);
    }

    if (p_arg.get_type() == Variant::ARRAY)
    {
        Array arr = p_arg;
        term_t list = PL_new_term_ref();
        if (!PL_put_nil(list))
            return (term_t)0;
        for (int i = arr.size() - 1; i >= 0; i--)
        {
            term_t elem = object_arg_to_term(arr[i], p_vars, p_order);
            if (!elem)
                return (term_t)0;
            term_t new_list = PL_new_term_ref();
            if (!PL_cons_list(new_list, elem, list))
                return (term_t)0;
            list = new_list;
        }
        return list;
    }

    return variant_to_term(p_arg);
}

term_t Prologot::prolog_term_to_swi(Ref<PrologTerm> const& p_term,
                                    std::map<int64_t, term_t>& p_vars,
                                    std::vector<Ref<PrologVariable>>& p_order)
{
    if (p_term.is_null())
        return (term_t)0;

    Ref<PrologVariable> variable = p_term;
    if (variable.is_valid())
        return object_arg_to_term(variable, p_vars, p_order);

    switch (p_term->get_kind_enum())
    {
        case PrologTerm::KIND_ATOM:
            return variant_to_term(p_term->get_atom());
        case PrologTerm::KIND_INTEGER:
            return variant_to_term(p_term->get_integer());
        case PrologTerm::KIND_FLOAT:
            return variant_to_term(p_term->get_real());
        case PrologTerm::KIND_STRING:
        {
            term_t t = PL_new_term_ref();
            if (!PL_put_string_chars(
                    t, p_term->get_string_value().utf8().get_data()))
                return (term_t)0;
            return t;
        }
        case PrologTerm::KIND_NIL:
        {
            term_t t = PL_new_term_ref();
            if (!PL_put_nil(t))
                return (term_t)0;
            return t;
        }
        case PrologTerm::KIND_LIST:
            return object_arg_to_term(p_term->get_args(), p_vars, p_order);
        case PrologTerm::KIND_COMPOUND:
        {
            Ref<PrologGoal> goal = PrologGoal::from_compound(
                p_term->get_functor(), p_term->get_args());
            term_t t = PL_new_term_ref();
            if (!compile_goal(goal, t, p_vars, p_order))
                return (term_t)0;
            return t;
        }
        case PrologTerm::KIND_VARIABLE:
            return (term_t)0;
    }
    return (term_t)0;
}

bool Prologot::compile_goal(Ref<PrologGoal> const& p_goal,
                            term_t p_out_goal,
                            std::map<int64_t, term_t>& p_vars,
                            std::vector<Ref<PrologVariable>>& p_order)
{
    if (p_goal.is_null())
        return false;

    Array args = p_goal->get_args();
    int arity = args.size();
    term_t arg_refs = arity > 0 ? PL_new_term_refs(arity) : (term_t)0;
    for (int i = 0; i < arity; i++)
    {
        term_t arg = object_arg_to_term(args[i], p_vars, p_order);
        if (!arg || !PL_put_term(arg_refs + i, arg))
        {
            m_last_error =
                "Failed to convert goal argument " + String::num_int64(i);
            return false;
        }
    }

    functor_t f = PL_new_functor(
        PL_new_atom(p_goal->get_functor().utf8().get_data()), arity);
    if (!PL_cons_functor_v(p_out_goal, f, arg_refs))
    {
        m_last_error = "Failed to construct goal term";
        return false;
    }
    return true;
}

Array Prologot::collect_goal_solutions(Ref<PrologGoal> const& p_goal)
{
    Array results;
    if (!m_initialized || p_goal.is_null())
        return results;

    if (PL_exception(0))
        PL_clear_exception();

    fid_t frame = PL_open_foreign_frame();
    std::map<int64_t, term_t> vars;
    std::vector<Ref<PrologVariable>> order;
    term_t goal = PL_new_term_ref();
    if (!compile_goal(p_goal, goal, vars, order))
    {
        PL_discard_foreign_frame(frame);
        return results;
    }

    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), goal);

    while (true)
    {
        int result = PL_next_solution(qid);
        if (result == PL_S_EXCEPTION)
        {
            handle_prolog_exception(qid, "Solve goal");
            PL_close_query(qid);
            PL_discard_foreign_frame(frame);
            return results;
        }
        if (!result)
            break;

        Ref<PrologSolution> solution = PrologSolution::create();
        for (Ref<PrologVariable> const& variable : order)
        {
            auto it = vars.find(variable->get_id());
            if (it != vars.end())
                solution->put(variable, term_to_variant(it->second));
        }
        results.push_back(solution);
    }

    PL_close_query(qid);
    PL_close_foreign_frame(frame);
    return results;
}

term_t Prologot::godot_object_to_term(Object* p_object)
{
    Ref<PrologObject> handle = PrologObject::create(p_object);
    if (handle.is_null())
        return (term_t)0;
    return handle->to_swi_term();
}

bool Prologot::apply_clause_predicate(char const* p_name,
                                      Ref<PrologGoal> const& p_goal,
                                      String const& p_context)
{
    if (!m_initialized)
        return false;
    if (p_goal.is_null())
    {
        m_last_error = p_context + String(": missing PrologGoal");
        return false;
    }

    std::map<int64_t, term_t> vars;
    std::vector<Ref<PrologVariable>> order;
    term_t term = PL_new_term_ref();
    if (!compile_goal(p_goal, term, vars, order))
    {
        if (m_last_error.is_empty())
            m_last_error = p_context + String(": failed to compile goal");
        return false;
    }

    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate(p_name, 1, "user"), term);
    int result = PL_next_solution(qid);
    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, p_context);
        PL_close_query(qid);
        return false;
    }
    PL_close_query(qid);
    return result != 0;
}

Ref<PrologQuery> Prologot::solve(Ref<PrologGoal> const& p_goal)
{
    return PrologQuery::create(collect_goal_solutions(p_goal));
}

// =============================================================================
// Low-level queries (Prolog source text, editor/REPL)
// =============================================================================

static String strip_trailing_period(String text)
{
    if (text.length() > 0 && text[text.length() - 1] == '.')
        text = text.substr(0, text.length() - 1);
    return text;
}

bool Prologot::query_text(String const& p_goal)
{
    if (!m_initialized)
        return false;

    String goal = strip_trailing_period(p_goal);
    if (goal.is_empty())
    {
        m_last_error = "Empty query";
        return false;
    }

    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(goal.utf8().get_data(), t))
    {
        m_last_error = "Failed to parse query: " + goal;
        return false;
    }

    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), t);
    int result = PL_next_solution(qid);

    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Query");
        PL_close_query(qid);
        return false;
    }

    PL_close_query(qid);
    return result != 0;
}

Array Prologot::query_text_all(String const& p_goal)
{
    Array results;
    if (!m_initialized)
        return results;

    String goal = strip_trailing_period(p_goal);
    if (goal.is_empty())
    {
        m_last_error = "Empty query";
        return results;
    }

    String findall_goal =
        "findall(" + goal + ", " + goal + ", PrologotResults__)";

    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(findall_goal.utf8().get_data(), t))
    {
        m_last_error = "Failed to parse query: " + goal;
        return results;
    }

    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), t);
    int solution_result = PL_next_solution(qid);

    if (solution_result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Query all");
        PL_close_query(qid);
        return results;
    }

    if (solution_result)
    {
        term_t findall_term = PL_new_term_ref();
        if (PL_get_arg(3, t, findall_term))
        {
            term_t head = PL_new_term_ref();
            term_t tail = PL_copy_term_ref(findall_term);
            while (PL_get_list(tail, head, tail))
                results.push_back(term_to_variant(head));
        }
    }

    PL_close_query(qid);
    return results;
}

Variant Prologot::query_text_one(String const& p_goal)
{
    if (!m_initialized)
        return Variant();

    String goal = strip_trailing_period(p_goal);
    if (goal.is_empty())
    {
        m_last_error = "Empty query";
        return Variant();
    }

    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(goal.utf8().get_data(), t))
    {
        m_last_error = "Failed to parse query: " + goal;
        return Variant();
    }

    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), t);
    int result = PL_next_solution(qid);

    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Query one");
        PL_close_query(qid);
        return Variant();
    }

    Variant var;
    if (result)
        var = term_to_variant(t);

    PL_close_query(qid);
    return var;
}

Array Prologot::query_text_named(String const& p_goal)
{
    Array results;
    if (!m_initialized)
        return results;

    String goal = strip_trailing_period(p_goal);
    if (goal.begins_with("?-"))
        goal = goal.substr(2).strip_edges();
    if (goal.is_empty())
    {
        m_last_error = "Empty query";
        return results;
    }

    term_t args = PL_new_term_refs(3);
    if (!PL_put_atom_chars(args + 0, goal.utf8().get_data()))
    {
        m_last_error = "Failed to build query atom: " + goal;
        return results;
    }
    if (!PL_put_variable(args + 1) || !PL_put_variable(args + 2))
        return results;

    qid_t parse_qid = PL_open_query(NULL,
                                    PL_Q_CATCH_EXCEPTION,
                                    PL_predicate("atom_to_term", 3, NULL),
                                    args);
    int parsed = PL_next_solution(parse_qid);
    if (parsed == PL_S_EXCEPTION)
    {
        handle_prolog_exception(parse_qid, "Parse query");
        PL_close_query(parse_qid);
        return results;
    }
    if (!parsed)
    {
        m_last_error = "Failed to parse query: " + goal;
        PL_close_query(parse_qid);
        return results;
    }
    // Keep Term and Bindings. PL_close_query would undo atom_to_term/3.
    PL_cut_query(parse_qid);

    term_t term = args + 1;
    term_t bindings = args + 2;

    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), term);

    while (true)
    {
        int result = PL_next_solution(qid);
        if (result == PL_S_EXCEPTION)
        {
            handle_prolog_exception(qid, "Query");
            PL_close_query(qid);
            return results;
        }
        if (!result)
            break;

        Dictionary dict;
        term_t head = PL_new_term_ref();
        term_t tail = PL_copy_term_ref(bindings);
        while (PL_get_list(tail, head, tail))
        {
            term_t name_term = PL_new_term_ref();
            term_t value_term = PL_new_term_ref();
            if (!PL_get_arg(1, head, name_term) ||
                !PL_get_arg(2, head, value_term))
                continue;
            char* name_chars = nullptr;
            if (!PL_get_atom_chars(name_term, &name_chars) || !name_chars)
                continue;
            dict[String(name_chars)] = term_to_variant(value_term);
        }
        results.push_back(dict);
    }

    PL_close_query(qid);
    return results;
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

bool Prologot::assert_fact(Ref<PrologGoal> const& p_goal)
{
    return apply_clause_predicate("assertz", p_goal, "Assert fact");
}

bool Prologot::retract_fact(Ref<PrologGoal> const& p_goal)
{
    return apply_clause_predicate("retract", p_goal, "Retract fact");
}

bool Prologot::retract_all(Ref<PrologGoal> const& p_goal)
{
    return apply_clause_predicate("retractall", p_goal, "Retract all");
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
    Array results = query_text_all(goal);

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
        case PL_BLOB:
        {
            PL_blob_t* blob_type = nullptr;
            if (PL_is_blob(p_term, &blob_type) &&
                PrologObject::is_blob_type(blob_type))
            {
                Ref<PrologObject> handle = PrologObject::from_swi_term(p_term);
                if (handle.is_valid())
                    return handle;
            }

            if (type == PL_BLOB)
                return Variant();

            // Atom → Godot String. Distinct from a Prolog string.
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
            // Prolog string → PrologTerm, so it is not confused with an atom.
            char* s;
            size_t len;
            if (!PL_get_string_chars(p_term, &s, &len))
                return Variant();
            (void)len;
            return PrologTerm::make_string(String(s));
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
        case Variant::STRING_NAME:
            // A GDScript String / StringName is always a Prolog atom.
            // Use prolog.string() for a Prolog string ("text").
            if (!PL_put_atom_chars(t, String(p_var).utf8().get_data()))
            {
                return (term_t)0;
            }
            break;

        case Variant::OBJECT:
        {
            Ref<PrologTerm> term = p_var;
            if (term.is_valid())
            {
                std::map<int64_t, term_t> vars;
                std::vector<Ref<PrologVariable>> order;
                return prolog_term_to_swi(term, vars, order);
            }
            Ref<PrologObject> handle = p_var;
            if (handle.is_valid())
                return handle->to_swi_term();
            Object* native = p_var;
            if (native)
                return godot_object_to_term(native);
            return (term_t)0;
        }

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

        case Variant::VECTOR2:
        {
            Vector2 v = p_var;
            Array arr;
            arr.push_back(v.x);
            arr.push_back(v.y);
            return variant_to_term(arr);
        }

        case Variant::VECTOR2I:
        {
            Vector2i v = p_var;
            Array arr;
            arr.push_back(v.x);
            arr.push_back(v.y);
            return variant_to_term(arr);
        }

        case Variant::VECTOR3:
        {
            Vector3 v = p_var;
            Array arr;
            arr.push_back(v.x);
            arr.push_back(v.y);
            arr.push_back(v.z);
            return variant_to_term(arr);
        }

        case Variant::VECTOR3I:
        {
            Vector3i v = p_var;
            Array arr;
            arr.push_back(v.x);
            arr.push_back(v.y);
            arr.push_back(v.z);
            return variant_to_term(arr);
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

        // Quit the application
        Engine* engine = Engine::get_singleton();
        if (engine)
        {
            SceneTree* scene_tree =
                Object::cast_to<SceneTree>(engine->get_main_loop());
            if (scene_tree)
            {
                scene_tree->quit(1);
            }
        }
    }
    // "status" option: only store in m_last_error, don't print
}

bool Prologot::handle_prolog_exception(qid_t p_qid, String const& p_context)
{
    term_t exception = PL_exception(p_qid);
    if (exception)
    {
        char* exception_str;
        if (PL_get_chars(exception,
                         &exception_str,
                         CVT_WRITE | CVT_EXCEPTION | BUF_DISCARDABLE))
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
