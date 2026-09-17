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
#include <cstring>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <utility>
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
#include <godot_cpp/classes/os.hpp>
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

std::vector<Prologot::ExposedBinding> Prologot::s_exposed;

// Live handles attached via initialize() (not the number of Prologot objects).
static int g_attach_count = 0;
// Predicates added through consult_* / assert_fact / expose_*; last cleanup().
static std::set<std::pair<std::string, int>> g_added_predicates;

// True after a successful PL_initialise in this process (any handle).
static bool pl_engine_is_up()
{
    return PL_is_initialised(nullptr, nullptr) != FALSE;
}

// Run a ground goal, swallow exceptions. Used when wiping the user KB.
static bool call_prolog_silent(char const* p_goal)
{
    if (PL_exception(0))
        PL_clear_exception();

    fid_t frame = PL_open_foreign_frame();
    term_t t = PL_new_term_ref();
    if (!PL_chars_to_term(p_goal, t))
    {
        PL_discard_foreign_frame(frame);
        return false;
    }

    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "system"), t);
    int result = PL_next_solution(qid);
    if (result == PL_S_EXCEPTION)
        PL_clear_exception();
    PL_close_query(qid);
    PL_discard_foreign_frame(frame);
    return result != 0 && result != PL_S_EXCEPTION;
}

// One-shot PL_open_query / PL_next_solution / PL_close_query. Clears exceptions.
static bool pl_call_pred(predicate_t p_pred, term_t p_args)
{
    qid_t qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, p_pred, p_args);
    int result = PL_next_solution(qid);
    if (result == PL_S_EXCEPTION)
        PL_clear_exception();
    PL_close_query(qid);
    return result != 0 && result != PL_S_EXCEPTION;
}

// Predicates we must never abolish (bootstrap helpers + SWI hooks / lists).
static bool is_protected_predicate(std::string const& p_name, int p_arity)
{
    if ((p_name == "load_program_from_string" && p_arity == 1) ||
        (p_name == "prologot_load_clauses" && p_arity == 1) ||
        (p_name == "prologot_process_clause" && p_arity == 1))
        return true;

    // SWI hooks and library predicates visible in user. Touching them
    // breaks consult/1, member/2, or exception autoload.
    if (p_name == "exception" || p_name == "message_hook" ||
        p_name == "thread_message_hook" || p_name == "portray" ||
        p_name == "prolog_load_file" || p_name == "prolog_list_goal" ||
        p_name == "message_property" || p_name == "file_search_path" ||
        p_name == "expand_query" || p_name == "expand_answer" ||
        p_name == "term_expansion" || p_name == "goal_expansion" ||
        p_name == "member" || p_name == "memberchk" || p_name == "append" ||
        p_name == "reverse" ||
        p_name == "consult" || p_name == "load_files" ||
        p_name == "ensure_loaded" || p_name == "use_module" ||
        p_name == "absolute_file_name" || p_name == "access_file" ||
        p_name == "exists_file" || p_name == "exists_source" ||
        p_name == "open" || p_name == "close" || p_name == "read_term" ||
        p_name == "open_string" || p_name == "call" ||
        p_name == "call_cleanup" || p_name == "assert" ||
        p_name == "assertz" || p_name == "asserta" || p_name == "retract" ||
        p_name == "retractall" || p_name == "abolish")
        return true;

    return false;
}

// Writes user:Name/Arity into p_out (for predicate_property/2).
static bool put_user_predicate_indicator(term_t p_out,
                                         char const* p_name,
                                         int p_arity)
{
    term_t name = PL_new_term_ref();
    term_t arity = PL_new_term_ref();
    term_t pi = PL_new_term_ref();
    term_t user = PL_new_term_ref();
    if (!PL_put_atom_chars(name, p_name) || !PL_put_integer(arity, p_arity))
        return false;
    if (!PL_cons_functor(pi, PL_new_functor(PL_new_atom("/"), 2), name, arity))
        return false;
    if (!PL_put_atom_chars(user, "user"))
        return false;
    return PL_cons_functor(
               p_out, PL_new_functor(PL_new_atom(":"), 2), user, pi) != FALSE;
}

// True if user:Name/Arity has the given predicate_property/2 atom (built_in, …).
static bool user_predicate_has_atom_property(char const* p_name,
                                             int p_arity,
                                             char const* p_property)
{
    fid_t frame = PL_open_foreign_frame();
    term_t args = PL_new_term_refs(2);
    bool ok = put_user_predicate_indicator(args, p_name, p_arity) &&
              PL_put_atom_chars(args + 1, p_property) &&
              pl_call_pred(PL_predicate("predicate_property", 2, "system"), args);
    PL_discard_foreign_frame(frame);
    return ok;
}

// retractall + abolish one tracked user predicate. Skips protected / foreign.
static void wipe_user_predicate(char const* p_name, int p_arity)
{
    if (is_protected_predicate(p_name, p_arity))
        return;
    if (user_predicate_has_atom_property(p_name, p_arity, "built_in"))
        return;
    if (user_predicate_has_atom_property(p_name, p_arity, "foreign"))
        return;
    if (user_predicate_has_atom_property(p_name, p_arity, "autoload"))
        return;

    std::string retract = "catch(system:retractall(user:";
    retract += p_name;
    if (p_arity > 0)
    {
        retract += "(";
        for (int i = 0; i < p_arity; ++i)
        {
            if (i)
                retract += ",";
            retract += "_";
        }
        retract += ")";
    }
    retract += "), _, true)";
    call_prolog_silent(retract.c_str());

    std::string abolish = "catch(system:abolish(user:";
    abolish += p_name;
    abolish += "/";
    abolish += std::to_string(p_arity);
    abolish += "), _, true)";
    call_prolog_silent(abolish.c_str());
}

// Snapshot of current_predicate(user:PI). Used to detect predicates we added.
static std::vector<std::pair<std::string, int>> list_user_predicate_indicators()
{
    std::vector<std::pair<std::string, int>> predicates;
    if (!pl_engine_is_up())
        return predicates;

    StringBuffers strings;
    fid_t frame = PL_open_foreign_frame();
    term_t findall_goal = PL_new_term_ref();
    if (!PL_chars_to_term(
            "findall(PI, system:current_predicate(user:PI), PIs)",
            findall_goal))
    {
        PL_discard_foreign_frame(frame);
        return predicates;
    }

    qid_t qid = PL_open_query(
        NULL,
        PL_Q_CATCH_EXCEPTION,
        PL_predicate("call", 1, "system"),
        findall_goal);
    int found = PL_next_solution(qid);
    if (found == PL_S_EXCEPTION)
        PL_clear_exception();

    if (found && found != PL_S_EXCEPTION)
    {
        term_t list = PL_new_term_ref();
        if (PL_get_arg(3, findall_goal, list))
        {
            term_t head = PL_new_term_ref();
            term_t tail = PL_copy_term_ref(list);
            while (PL_get_list(tail, head, tail))
            {
                atom_t functor = 0;
                size_t arity = 0;
                if (!PL_get_name_arity(head, &functor, &arity) || arity != 2)
                    continue;
                if (std::strcmp(PL_atom_chars(functor), "/") != 0)
                    continue;

                term_t name_t = PL_new_term_ref();
                term_t arity_t = PL_new_term_ref();
                char* name_chars = nullptr;
                int arity_i = 0;
                if (!PL_get_arg(1, head, name_t) || !PL_get_arg(2, head, arity_t))
                    continue;
                if (!PL_get_chars(
                        name_t, &name_chars, CVT_ATOM | REP_UTF8 | BUF_DISCARDABLE) ||
                    !PL_get_integer(arity_t, &arity_i))
                    continue;
                predicates.emplace_back(name_chars, arity_i);
            }
        }
    }
    PL_close_query(qid);
    PL_discard_foreign_frame(frame);
    return predicates;
}

// SWI is process-global and not re-entrant from Godot worker threads.
bool Prologot::require_main_thread(char const* p_where)
{
    godot::OS* os = godot::OS::get_singleton();
    if (os == nullptr || os->get_thread_caller_id() == os->get_main_thread_id())
        return true;

    godot::String msg =
        godot::String("Prologot: ") + p_where +
        " must run on the Godot main thread (one SWI engine per process; "
        "Prologot instances are handles, not isolated engines).";
    godot::UtilityFunctions::push_error(msg);
    return false;
}

// After consult/assert, record PIs that were not in p_before (last-handle wipe).
static void remember_new_predicates(
    std::vector<std::pair<std::string, int>> const& p_before)
{
    auto const after = list_user_predicate_indicators();
    for (auto const& pred : after)
    {
        if (std::find(p_before.begin(), p_before.end(), pred) != p_before.end())
            continue;
        if (is_protected_predicate(pred.first, pred.second))
            continue;
        g_added_predicates.insert(pred);
    }
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
    godot::ClassDB::bind_method(godot::D_METHOD("clear_knowledge"),
                         &Prologot::clear_knowledge);

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
    godot::ClassDB::bind_method(godot::D_METHOD("solve", "goal", "max_solutions"),
                         &Prologot::solve,
                         DEFVAL(int64_t(0)));

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
}

Prologot::~Prologot()
{
    cleanup();
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
    {
#if defined(_WIN32)
        char const* platform = "windows";
#elif defined(__APPLE__)
        char const* platform = "macos";
#else
        char const* platform = "linux";
#endif
        godot::String bundled =
            resolve_godot_path(godot::String("res://bin/") + platform + "/swipl");
        if (godot::FileAccess::file_exists(bundled + "/boot.prc"))
            return std::make_pair(bundled, "");
        return std::make_pair("", "");
    }

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
    if (!require_main_thread("initialize()"))
        return false;

    // Idempotent: if this handle is already attached, return success
    if (m_initialized)
        return true;

    m_on_error = p_options.get("on error", "print");
    m_on_warning = p_options.get("on warning", "print");

    // SWI-Prolog is process-global: attach to an engine started earlier
    // in this process (tests, editor dock, another Prologot.new()).
    if (pl_engine_is_up())
        return attach_handle();

    // Embed options (typical Godot use).
    bool quiet = p_options.get("quiet", true);
    bool threads = p_options.get("threads", true);
    godot::String stack_limit = p_options.get("stack limit", "");
    godot::String table_space = p_options.get("table space", "");
    godot::String shared_table_space = p_options.get("shared table space", "");

    // Same keys as `swipl` on the command line. Prefer consult_* / solve()
    // after initialize(); these only apply to the first PL_initialise.
    bool optimized = p_options.get("optimized", false);
    bool traditional = p_options.get("traditional", false);
    bool packs = p_options.get("packs", true);
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

    if (quiet)
        argv_list.push_back("--quiet");
    if (!threads)
        argv_list.push_back("--no-threads");
    if (optimized)
        argv_list.push_back("-O");
    if (traditional)
        argv_list.push_back("--traditional");
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

    // "init file" → `swipl -f FILE`: user init instead of ~/.swiplrc
    // (`"none"` skips that file). Not the same as consult_file().
    if (!init_file.is_empty())
    {
        argv_list.push_back("-f");
        string_storage.push_back(init_file.utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }
    // "script file" → `swipl -l FILE`: consult a .pl once at boot.
    // After start, use consult_file() instead.
    if (!script_file.is_empty())
    {
        argv_list.push_back("-l");
        string_storage.push_back(script_file.utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }
    // "toplevel" → `swipl -t GOAL`: replaces the interactive Prolog prompt.
    // Godot has no SWI REPL; leave empty.
    if (!toplevel.is_empty())
    {
        argv_list.push_back("-t");
        string_storage.push_back(toplevel.utf8().get_data());
        argv_list.push_back(string_storage.back().c_str());
    }

    // "goal" → `swipl -g GOAL` (repeatable): run at startup, before toplevel.
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
        "prologot_process_clause(Clause) :- assertz(user:Clause)",

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

// Mark this object as attached. Does not call PL_initialise (engine already up).
bool Prologot::attach_handle()
{
    m_initialized = true;
    ++g_attach_count;
    return true;
}

// Retract every process-global expose_* wrapper (shared SWI KB).
void Prologot::uninstall_exposed()
{
    if (!pl_engine_is_up())
    {
        s_exposed.clear();
        return;
    }

    std::vector<ExposedBinding> exposed = s_exposed;
    for (ExposedBinding const& binding : exposed)
        unexpose(binding.predicate, binding.arity);
    s_exposed.clear();
}

bool Prologot::clear_knowledge()
{
    if (!require_main_thread("clear_knowledge()"))
        return false;
    if (!m_initialized)
        return false;
    PrologQuery::abandon_all_open();
    reset_user_knowledge();
    return true;
}

// Abolish predicates recorded in g_added_predicates. Leaves SWI running.
void Prologot::reset_user_knowledge()
{
    if (!pl_engine_is_up())
        return;

    if (PL_exception(0))
        PL_clear_exception();

    call_prolog_silent("catch(system:abolish_all_tables, _, true)");

    for (auto const& pred : g_added_predicates)
        wipe_user_predicate(pred.first.c_str(), pred.second);
    g_added_predicates.clear();
    uninstall_exposed();
}

void Prologot::cleanup()
{
    if (!require_main_thread("cleanup()"))
        return;
    if (!m_initialized)
        return;

    if (g_attach_count > 0)
        --g_attach_count;

    if (pl_engine_is_up())
    {
        PrologQuery::abandon_all_open();
        if (g_attach_count == 0)
            reset_user_knowledge();
    }

    m_initialized = false;
}

void Prologot::shutdown_engine()
{
    if (!pl_engine_is_up())
        return;

    PL_cleanup(0);
    g_attach_count = 0;
    g_added_predicates.clear();
    s_exposed.clear();
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
    if (!require_main_thread("consult_file()"))
        return false;
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

    auto const before = list_user_predicate_indicators();

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
    if (result != 0)
        remember_new_predicates(before);
    return result != 0; // Non-zero means success in SWI-Prolog API
}

bool Prologot::consult_string(godot::String const& p_prolog_code)
{
    if (!require_main_thread("consult_string()"))
        return false;
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

    auto const before = list_user_predicate_indicators();

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
    if (result != 0)
        remember_new_predicates(before);
    return result != 0;
}

// =============================================================================
// Structured term factories
//
// Thin forwards to PrologTerm / PrologVariable / PrologPredicate /
// PrologObject. No SWI call. Bound on Prologot so GDScript has one facade.
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

// assertz / asserta / retract / retractall on a compiled PrologGoal.
bool Prologot::apply_clause_predicate(char const* p_name,
                                      godot::Ref<PrologGoal> const& p_goal,
                                      godot::String const& p_context)
{
    if (!require_main_thread(p_context.utf8().get_data()))
        return false;
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

    bool const tracks_new_predicates =
        std::strcmp(p_name, "assertz") == 0 || std::strcmp(p_name, "asserta") == 0;

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
    if (result != 0 && tracks_new_predicates && p_goal.is_valid())
    {
        std::string const name = p_goal->get_functor().utf8().get_data();
        int const arity = p_goal->get_arity();
        if (!is_protected_predicate(name, arity))
            g_added_predicates.insert({name, arity});
    }
    return result != 0;
}

godot::Ref<PrologQuery> Prologot::solve(godot::Ref<PrologGoal> const& p_goal,
                                        int64_t p_max_solutions)
{
    if (!require_main_thread("solve()"))
        return PrologQuery::create(godot::Array());
    return PrologQuery::open(this, p_goal, p_max_solutions);
}

// =============================================================================
// Low-level queries (Prolog source text, editor/REPL)
// =============================================================================

// Editor/REPL helpers accept "parent(tom, X)." — SWI term parse does not.
static godot::String strip_trailing_period(godot::String text)
{
    if (text.length() > 0 && text[text.length() - 1] == '.')
        text = text.substr(0, text.length() - 1);
    return text;
}

bool Prologot::query_text(godot::String const& p_goal)
{
    if (!require_main_thread("query_text()"))
        return false;
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
    if (!require_main_thread("query_text_all()"))
        return results;
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
    if (!require_main_thread("query_text_one()"))
        return godot::Variant();
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
    if (!require_main_thread("_editor_query()"))
        return results;
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
        StringBuffers strings;
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
    if (!require_main_thread("add_fact()"))
        return false;
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

    auto const before = list_user_predicate_indicators();

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
    if (result != 0)
        remember_new_predicates(before);
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
    if (!require_main_thread("predicate_exists()"))
        return false;
    if (!m_initialized)
        return false;

    // PL_predicate() / PL_pred() create the predicate if it is missing.
    std::string const name = p_predicate.utf8().get_data();
    int const arity = static_cast<int>(p_arity);
    for (auto const& pred : list_user_predicate_indicators())
    {
        if (pred.first == name && pred.second == arity)
            return true;
    }
    return false;
}

godot::Array Prologot::list_predicates()
{
    godot::Array predicates;
    if (!require_main_thread("list_predicates()"))
        return predicates;
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
        StringBuffers strings;
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

godot::String Prologot::get_last_error() const
{
    return m_last_error;
}

} // namespace prologot
