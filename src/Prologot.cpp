/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file implements the Prologot class methods.
 */

#include "Prologot.hpp"
#include "PrologConversion.hpp"
#include <algorithm>
#include <deque>
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

// =============================================================================
// Static Member Initialization
// =============================================================================

namespace prologot
{

Prologot* Prologot::m_singleton = nullptr;

static int g_attach_count = 0;
static std::vector<Prologot*> g_instances;

static bool pl_engine_is_up()
{
    return PL_is_initialised(nullptr, nullptr) != FALSE;
}

static bool call_prolog_silent(char const* p_goal)
{
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(p_goal, t))
        return false;

    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "system"), t);
    int result = PL_next_solution(qid);
    if (result == PL_S_EXCEPTION)
        PL_clear_exception();
    PL_close_query(qid);
    return result != 0 && result != PL_S_EXCEPTION;
}

// =============================================================================
// Godot Method Binding
// =============================================================================

void Prologot::_bind_methods()
{
    // Initialization methods
    godot::ClassDB::bind_method(godot::D_METHOD("initialize", "options"),
                         &Prologot::initialize,
                         DEFVAL(godot::Dictionary()));
    godot::ClassDB::bind_method(godot::D_METHOD("cleanup"), &Prologot::cleanup);
    godot::ClassDB::bind_method(godot::D_METHOD("is_initialized"), &Prologot::is_initialized);

    // File/code loading methods
    godot::ClassDB::bind_method(godot::D_METHOD("consult_file", "filename"),
                         &Prologot::consult_file);
    godot::ClassDB::bind_method(godot::D_METHOD("consult_string", "prolog_code"),
                         &Prologot::consult_string);

    godot::ClassDB::bind_method(godot::D_METHOD("atom", "name"), &Prologot::atom);
    godot::ClassDB::bind_method(godot::D_METHOD("integer", "value"), &Prologot::integer);
    godot::ClassDB::bind_method(godot::D_METHOD("real", "value"), &Prologot::real);
    godot::ClassDB::bind_method(godot::D_METHOD("string", "value"), &Prologot::string);
    godot::ClassDB::bind_method(godot::D_METHOD("nil"), &Prologot::nil);
    godot::ClassDB::bind_method(godot::D_METHOD("list", "items"), &Prologot::list);
    godot::ClassDB::bind_method(godot::D_METHOD("compound", "functor", "args"),
                         &Prologot::compound);
    godot::ClassDB::bind_method(
        godot::D_METHOD("variable", "name"), &Prologot::variable, DEFVAL(godot::String()));
    godot::ClassDB::bind_method(godot::D_METHOD("anonymous"), &Prologot::anonymous);
    godot::ClassDB::bind_method(godot::D_METHOD("predicate", "name"), &Prologot::predicate);
    godot::ClassDB::bind_method(godot::D_METHOD("object", "value"), &Prologot::object);

    // High-level structured solving
    godot::ClassDB::bind_method(godot::D_METHOD("solve", "goal"), &Prologot::solve);

    // Editor console only (not a game API)
    godot::ClassDB::bind_method(godot::D_METHOD("_editor_query", "goal"),
                         &Prologot::query_text_named);

    // Dynamic assertion methods
    godot::ClassDB::bind_method(godot::D_METHOD("assert_fact", "goal"),
                         &Prologot::assert_fact);
    godot::ClassDB::bind_method(godot::D_METHOD("retract_fact", "goal"),
                         &Prologot::retract_fact);
    godot::ClassDB::bind_method(godot::D_METHOD("retract_all", "goal"),
                         &Prologot::retract_all);

    // Introspection methods
    godot::ClassDB::bind_method(godot::D_METHOD("predicate_exists", "predicate", "arity"),
                         &Prologot::predicate_exists);
    godot::ClassDB::bind_method(godot::D_METHOD("list_predicates"),
                         &Prologot::list_predicates);
    godot::ClassDB::bind_method(
        godot::D_METHOD("expose_property", "class_name", "property", "predicate"),
        &Prologot::expose_property,
        DEFVAL(godot::String()));
    godot::ClassDB::bind_method(
        godot::D_METHOD("expose_method", "class_name", "method", "predicate"),
        &Prologot::expose_method,
        DEFVAL(godot::String()));
    godot::ClassDB::bind_method(godot::D_METHOD("unexpose", "predicate", "arity"),
                         &Prologot::unexpose);
    godot::ClassDB::bind_method(godot::D_METHOD("list_exposed"), &Prologot::list_exposed);

    // Error handling
    godot::ClassDB::bind_method(godot::D_METHOD("get_last_error"), &Prologot::get_last_error);
}

// =============================================================================
// Constructor and Destructor
// =============================================================================

Prologot::Prologot()
{
    m_initialized = false;
    m_on_error = "print";
    m_on_warning = "print";
    g_instances.push_back(this);
    if (m_singleton == nullptr)
        m_singleton = this;
}

Prologot::~Prologot()
{
    cleanup();
    auto it = std::find(g_instances.begin(), g_instances.end(), this);
    if (it != g_instances.end())
        g_instances.erase(it);
    if (m_singleton == this)
    {
        m_singleton = nullptr;
        for (Prologot* instance : g_instances)
        {
            if (instance->m_initialized)
            {
                m_singleton = instance;
                break;
            }
        }
        if (m_singleton == nullptr && !g_instances.empty())
            m_singleton = g_instances.front();
    }
}

// =============================================================================
// Initialization and Cleanup
// =============================================================================

godot::String Prologot::resolve_godot_path(godot::String const& p_path)
{
    if (p_path.begins_with("res://") || p_path.begins_with("user://"))
    {
        return godot::ProjectSettings::get_singleton()->globalize_path(p_path);
    }
    return p_path;
}

std::pair<godot::String, godot::String>
Prologot::set_swi_home_dir(godot::String const& p_home_option)
{
    if (p_home_option.is_empty())
        return std::make_pair("", ""); // Use system default SWI-Prolog

    godot::String path = resolve_godot_path(p_home_option);

    if (!godot::FileAccess::file_exists(path + "/boot.prc"))
    {
        godot::String out_error = "boot.prc not found in: " + path;
        return std::make_pair("", out_error);
    }

    return std::make_pair(path, "");
}

bool Prologot::initialize(godot::Dictionary const& p_options)
{
    // Idempotent: if this handle is already attached, return success
    if (m_initialized)
        return true;

    m_on_error = p_options.get("on error", "print");
    m_on_warning = p_options.get("on warning", "print");

    // SWI-Prolog is process-global: attach to an engine started earlier
    // in this process (tests, editor dock, another Prologot.new()).
    if (pl_engine_is_up())
        return attach_handle();

    // Extract other options
    bool quiet = p_options.get("quiet", true);
    bool optimized = p_options.get("optimized", false);
    bool traditional = p_options.get("traditional", false);
    bool threads = p_options.get("threads", true);
    bool packs = p_options.get("packs", true);
    godot::String stack_limit = p_options.get("stack limit", "");
    godot::String table_space = p_options.get("table space", "");
    godot::String shared_table_space = p_options.get("shared table space", "");
    godot::String init_file = p_options.get("init file", "");
    godot::String script_file = p_options.get("script file", "");
    godot::String toplevel = p_options.get("toplevel", "");
    godot::Variant goal_var = p_options.get("goal", godot::Variant());

    // Extract and resolve home directory
    auto [home, error] = set_swi_home_dir(p_options.get("home", ""));
    if (!error.is_empty())
    {
        m_last_error = error;
        push_error("Invalid SWI-Prolog home directory: " + error +
                   ". I will try to use the default one.");
    }

    // Build argv for PL_initialise.
    // deque (not vector): push_back must not reallocate/move existing strings,
    // otherwise the c_str() pointers already stored in argv_list dangle (SSO).
    std::deque<std::string> string_storage;
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
    if (goal_var.get_type() == godot::Variant::STRING)
    {
        godot::String goal = goal_var;
        if (!goal.is_empty())
        {
            argv_list.push_back("-g");
            string_storage.push_back(goal.utf8().get_data());
            argv_list.push_back(string_storage.back().c_str());
        }
    }
    else if (goal_var.get_type() == godot::Variant::ARRAY)
    {
        godot::Array goals = goal_var;
        for (int i = 0; i < goals.size(); i++)
        {
            godot::String goal = goals[i];
            argv_list.push_back("-g");
            string_storage.push_back(goal.utf8().get_data());
            argv_list.push_back(string_storage.back().c_str());
        }
    }

    // Prolog flags (-D name=value)
    if (p_options.has("prolog flags"))
    {
        godot::Dictionary flags = p_options.get("prolog flags", godot::Dictionary());
        godot::Array keys = flags.keys();
        for (int i = 0; i < keys.size(); i++)
        {
            godot::String key = keys[i];
            godot::String value = flags[key];
            argv_list.push_back("-D");
            string_storage.push_back((key + "=" + value).utf8().get_data());
            argv_list.push_back(string_storage.back().c_str());
        }
    }

    // File search paths (-p alias=path)
    if (p_options.has("file search paths"))
    {
        godot::Dictionary paths = p_options.get("file search paths", godot::Dictionary());
        godot::Array keys = paths.keys();
        for (int i = 0; i < keys.size(); i++)
        {
            godot::String alias = keys[i];
            godot::String path = paths[alias];
            argv_list.push_back("-p");
            string_storage.push_back((alias + "=" + path).utf8().get_data());
            argv_list.push_back(string_storage.back().c_str());
        }
    }

    // Custom arguments
    if (p_options.has("custom args"))
    {
        godot::Array custom_args = p_options.get("custom args", godot::Array());
        for (int i = 0; i < custom_args.size(); i++)
        {
            string_storage.push_back(godot::String(custom_args[i]).utf8().get_data());
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
            godot::UtilityFunctions::print("[Prologot] SWI-Prolog HOME: ", home_path);
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
            m_last_error = godot::String("Failed to parse bootstrap predicate: ") +
                           godot::String(predicates[i]);
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
                        godot::String("Failed to assert bootstrap predicate: ") +
                        godot::String(msg);
                }
                else
                {
                    m_last_error =
                        godot::String("Failed to assert bootstrap predicate: ") +
                        godot::String(predicates[i]);
                }
            }
            else
            {
                m_last_error =
                    godot::String("Failed to assert bootstrap predicate: ") +
                    godot::String(predicates[i]);
            }
            PL_close_query(qid);
            PL_cleanup(0); // Clean up on failure
            return false;
        }

        PL_close_query(qid);
    }

    // Mark as initialized only after all steps succeed
    return attach_handle();
}

bool Prologot::attach_handle()
{
    m_initialized = true;
    ++g_attach_count;
    if (m_singleton == nullptr || !m_singleton->m_initialized)
        m_singleton = this;
    return true;
}

void Prologot::uninstall_exposed()
{
    if (!pl_engine_is_up() || !m_initialized)
    {
        m_exposed.clear();
        return;
    }

    std::vector<ExposedBinding> exposed = m_exposed;
    for (ExposedBinding const& binding : exposed)
        unexpose(binding.predicate, binding.arity);
    m_exposed.clear();
}

void Prologot::reset_user_knowledge()
{
    if (!pl_engine_is_up())
        return;

    // Keep bootstrap helpers and foreign predicates; drop user clauses.
    // Do not call PL_cleanup(): restarting SWI in-process is not robust.
    call_prolog_silent(
        "catch(abolish_all_tables, _, true), "
        "forall((current_predicate(user:PI), "
        "        \\+ predicate_property(user:PI, built_in), "
        "        \\+ predicate_property(user:PI, foreign), "
        "        \\+ predicate_property(user:PI, imported_from(_)), "
        "        PI \\= load_program_from_string/1, "
        "        PI \\= prologot_load_clauses/1, "
        "        PI \\= prologot_process_clause/1), "
        "       catch(abolish(user:PI), _, true))");
}

void Prologot::cleanup()
{
    if (!m_initialized)
        return;

    if (g_attach_count > 0)
        --g_attach_count;

    if (pl_engine_is_up())
    {
        if (g_attach_count == 0)
            reset_user_knowledge();
        else
            uninstall_exposed();
    }

    m_exposed.clear();
    m_initialized = false;
}

void Prologot::shutdown_engine()
{
    if (!pl_engine_is_up())
        return;

    PL_cleanup(0);
    g_attach_count = 0;
}

bool Prologot::is_initialized() const
{
    return m_initialized;
}

// =============================================================================
// File and Code Consultation
// =============================================================================

bool Prologot::consult_file(godot::String const& p_filename)
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
    godot::String filename = resolve_godot_path(p_filename);

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

bool Prologot::consult_string(godot::String const& p_prolog_code)
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

godot::Ref<PrologTerm> Prologot::atom(godot::String const& p_name)
{
    return PrologTerm::make_atom(p_name);
}

godot::Ref<PrologTerm> Prologot::integer(int64_t p_value)
{
    return PrologTerm::make_integer(p_value);
}

godot::Ref<PrologTerm> Prologot::real(double p_value)
{
    return PrologTerm::make_real(p_value);
}

godot::Ref<PrologTerm> Prologot::string(godot::String const& p_value)
{
    return PrologTerm::make_string(p_value);
}

godot::Ref<PrologTerm> Prologot::nil()
{
    return PrologTerm::make_nil();
}

godot::Ref<PrologTerm> Prologot::list(godot::Array const& p_items)
{
    return PrologTerm::make_list(p_items);
}

godot::Ref<PrologTerm> Prologot::compound(godot::String const& p_functor, godot::Array const& p_args)
{
    return PrologTerm::make_compound(p_functor, p_args);
}

godot::Ref<PrologVariable> Prologot::variable(godot::String const& p_name)
{
    return PrologVariable::create(p_name);
}

godot::Ref<PrologVariable> Prologot::anonymous()
{
    return PrologVariable::create_anonymous();
}

godot::Ref<PrologPredicate> Prologot::predicate(godot::String const& p_name)
{
    return PrologPredicate::create(p_name);
}

godot::Ref<PrologObject> Prologot::object(godot::Object* p_object)
{
    return PrologObject::create(p_object);
}

// =============================================================================
// High-level solving (structured terms)
// =============================================================================

godot::Array Prologot::collect_goal_solutions(godot::Ref<PrologGoal> const& p_goal)
{
    godot::Array results;
    if (!m_initialized || p_goal.is_null())
        return results;

    if (PL_exception(0))
        PL_clear_exception();

    fid_t frame = PL_open_foreign_frame();
    std::map<int64_t, term_t> vars;
    std::vector<godot::Ref<PrologVariable>> order;
    term_t goal = PL_new_term_ref();
    godot::String error;
    if (!compile_goal(p_goal, goal, vars, order, &error))
    {
        if (!error.is_empty())
            m_last_error = error;
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

        godot::Ref<PrologSolution> solution = PrologSolution::create();
        for (godot::Ref<PrologVariable> const& variable : order)
        {
            auto it = vars.find(variable->get_id());
            if (it != vars.end())
                solution->put(
                    variable, term_to_variant(it->second));
        }
        results.push_back(solution);
    }

    PL_close_query(qid);
    PL_close_foreign_frame(frame);
    return results;
}

bool Prologot::apply_clause_predicate(char const* p_name,
                                      godot::Ref<PrologGoal> const& p_goal,
                                      godot::String const& p_context)
{
    if (!m_initialized)
        return false;
    if (p_goal.is_null())
    {
        m_last_error = p_context + godot::String(": missing PrologGoal");
        return false;
    }

    std::map<int64_t, term_t> vars;
    std::vector<godot::Ref<PrologVariable>> order;
    term_t term = PL_new_term_ref();
    godot::String error;
    if (!compile_goal(p_goal, term, vars, order, &error))
    {
        m_last_error = error.is_empty()
                           ? p_context + godot::String(": failed to compile goal")
                           : error;
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

godot::Ref<PrologQuery> Prologot::solve(godot::Ref<PrologGoal> const& p_goal)
{
    return PrologQuery::create(collect_goal_solutions(p_goal));
}

// =============================================================================
// Low-level queries (Prolog source text, editor/REPL)
// =============================================================================

static godot::String strip_trailing_period(godot::String text)
{
    if (text.length() > 0 && text[text.length() - 1] == '.')
        text = text.substr(0, text.length() - 1);
    return text;
}

bool Prologot::query_text(godot::String const& p_goal)
{
    if (!m_initialized)
        return false;

    godot::String goal = strip_trailing_period(p_goal);
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

godot::Array Prologot::query_text_all(godot::String const& p_goal)
{
    godot::Array results;
    if (!m_initialized)
        return results;

    godot::String goal = strip_trailing_period(p_goal);
    if (goal.is_empty())
    {
        m_last_error = "Empty query";
        return results;
    }

    godot::String findall_goal =
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

godot::Variant Prologot::query_text_one(godot::String const& p_goal)
{
    if (!m_initialized)
        return godot::Variant();

    godot::String goal = strip_trailing_period(p_goal);
    if (goal.is_empty())
    {
        m_last_error = "Empty query";
        return godot::Variant();
    }

    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(goal.utf8().get_data(), t))
    {
        m_last_error = "Failed to parse query: " + goal;
        return godot::Variant();
    }

    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "user"), t);
    int result = PL_next_solution(qid);

    if (result == PL_S_EXCEPTION)
    {
        handle_prolog_exception(qid, "Query one");
        PL_close_query(qid);
        return godot::Variant();
    }

    godot::Variant var;
    if (result)
        var = term_to_variant(t);

    PL_close_query(qid);
    return var;
}

godot::Array Prologot::query_text_named(godot::String const& p_goal)
{
    godot::Array results;
    if (!m_initialized)
        return results;

    godot::String goal = strip_trailing_period(p_goal);
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

        godot::Dictionary dict;
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
            dict[godot::String(name_chars)] =
                term_to_variant(value_term);
        }
        results.push_back(dict);
    }

    PL_close_query(qid);
    return results;
}

// =============================================================================
// Dynamic Assertions
// =============================================================================

bool Prologot::add_fact(godot::String const& p_fact)
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
    godot::String fact = p_fact;
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

bool Prologot::assert_fact(godot::Ref<PrologGoal> const& p_goal)
{
    return apply_clause_predicate("assertz", p_goal, "Assert fact");
}

bool Prologot::retract_fact(godot::Ref<PrologGoal> const& p_goal)
{
    return apply_clause_predicate("retract", p_goal, "Retract fact");
}

bool Prologot::retract_all(godot::Ref<PrologGoal> const& p_goal)
{
    return apply_clause_predicate("retractall", p_goal, "Retract all");
}

// =============================================================================
// Introspection
// =============================================================================

bool Prologot::predicate_exists(godot::String const& p_predicate, int p_arity)
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

godot::Array Prologot::list_predicates()
{
    godot::Array predicates;
    if (!m_initialized)
        return predicates;

    // Query for all current predicates using Prolog's built-in
    // current_predicate/1 This returns all predicates in the form Name/Arity
    godot::String goal = "current_predicate(Name/Arity)";
    godot::Array results = query_text_all(goal);

    return results;
}


// =============================================================================
// Exception Handling
// =============================================================================

void Prologot::push_error(godot::String const& p_message, godot::String const& p_type)
{
    // Store error in m_last_error
    m_last_error = p_message;

    // Determine which option to check
    godot::String option = (p_type == "warning") ? m_on_warning : m_on_error;

    // Handle according to option
    if (option == "print")
    {
        godot::UtilityFunctions::push_error("Prologot: " + p_message);
    }
    else if (option == "halt")
    {
        godot::UtilityFunctions::push_error("Prologot: " + p_message);

        // Quit the application
        godot::Engine* engine = godot::Engine::get_singleton();
        if (engine)
        {
            godot::SceneTree* scene_tree =
                godot::Object::cast_to<godot::SceneTree>(engine->get_main_loop());
            if (scene_tree)
            {
                scene_tree->quit(1);
            }
        }
    }
    // "status" option: only store in m_last_error, don't print
}

bool Prologot::handle_prolog_exception(qid_t p_qid, godot::String const& p_context)
{
    term_t exception = PL_exception(p_qid);
    if (exception)
    {
        char* exception_str;
        if (PL_get_chars(exception,
                         &exception_str,
                         CVT_WRITE | CVT_EXCEPTION | BUF_DISCARDABLE))
        {
            godot::String error_msg =
                p_context + godot::String(" error: ") + godot::String(exception_str);
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

godot::String Prologot::get_last_error() const
{
    return m_last_error;
}

} // namespace prologot
