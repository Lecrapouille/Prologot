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
#include "PrologObject.hpp"
#include "PrologPredicate.hpp"
#include "PrologQuery.hpp"
#include "PrologSolution.hpp"
#include "PrologTerm.hpp"
#include "PrologVariable.hpp"
#include <SWI-Prolog.h>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <utility>
#include <vector>

namespace prologot
{

/**
 * @class Prologot
 * @brief Main class providing SWI-Prolog integration for Godot 4.
 *
 * SWI-Prolog is process-global: PL_initialise / PL_cleanup run once per
 * process. Each Prologot is a handle to that shared engine. Typical flow:
 * initialize(), consult_file() / consult_string(), build a PrologGoal with
 * predicate().call(), then solve(). solve() is lazy: answers are pulled
 * on demand (has_solution / first / for / all). A PrologQuery is always
 * truthy in GDScript — use has_solution() for a yes/no test. Full
 * reading API: PrologQuery and solve() below.
 *
 * @example
 * var p = Prologot.new()
 * if not p.initialize():
 *     push_error(p.get_last_error())
 *     return
 * p.consult_file("res://rules.pl")
 *
 * var parent = p.predicate("parent")
 * var child = p.variable("Child")
 * if p.solve(parent.call("tom", "bob")).has_solution():
 *     print("true")
 * for solution in p.solve(parent.call("tom", child)):
 *     print(solution.get(child))
 *     if solution.get(child) == "bob":
 *         break
 * p.cleanup()
 */
class Prologot: public godot::RefCounted
{
    GDCLASS(Prologot, godot::RefCounted)

public:

    /**
     * @brief Constructs a new Prologot handle.
     *
     * Does not start SWI-Prolog. The first live instance becomes the C++
     * singleton; later constructors do not overwrite it.
     */
    Prologot();

    /**
     * @brief Destructs the Prologot handle.
     *
     * Detaches this handle (see cleanup()). Clears the C++ singleton only
     * if it still points at this instance. Does not call PL_cleanup().
     */
    ~Prologot();

    /**
     * @brief Gets the C++ singleton handle used by foreign predicates.
     *
     * The first live instance is recorded; a second Prologot.new() does
     * not steal it. initialize() claims it only if it is null or detached.
     *
     * @return Pointer to the singleton instance, or nullptr if none.
     */
    static Prologot* get_singleton();

    /**
     * @brief Shuts down the process-global SWI-Prolog engine.
     *
     * Call only when the GDExtension is unloaded. Safe if Prolog was never
     * started. Do not use this to isolate tests or scene changes.
     */
    static void shutdown_engine();

    // =========================================================================
    // Initialization and Cleanup
    // =========================================================================

    /**
     * @brief Initializes the SWI-Prolog engine with optional configuration.
     *
     * This method performs the following steps:
     * 1. Checks if this handle is already attached (idempotent)
     * 2. If SWI-Prolog is already running in this process, attaches to it
     *    (startup options such as "home" are ignored after the first start)
     * 3. Otherwise parses options, calls PL_initialise once, and bootstraps
     *    helper predicates needed for consult_string()
     *
     * The bootstrap predicates enable loading Prolog code from strings by:
     * - Parsing multi-line Prolog code into individual clauses
     * - Handling directives (:-) and queries (?-) appropriately
     * - Asserting regular clauses into the knowledge base
     *
     * @param p_options Passed to PL_initialise on the first start only.
     *        Later initialize() calls attach and ignore these keys except
     *        "on error" / "on warning" (stored on this handle).
     *
     *        Embed (what a game / editor actually needs):
     *          "home", "quiet" (default true), "stack limit",
     *          "table space", "shared table space", "threads" (default true),
     *          "on error", "on warning"
     *
     *        SWI command-line passthrough (rarely useful in Godot):
     *          "init file"   (`swipl -f`): user init instead of ~/.swiplrc
     *          "script file" (`swipl -l`): consult a .pl at boot
     *          "toplevel"    (`swipl -t`): replaces the interactive prompt
     *          "goal"        (`swipl -g`): run a goal at startup
     *          "optimized", "traditional", "packs",
     *          "prolog flags", "file search paths", "custom args"
     *
     * @return true if initialization succeeded, false otherwise.
     *
     * @example
     * var prolog = Prologot.new()
     * if not prolog.initialize({"home": "res://bin/linux/swipl"}):
     *     push_error(prolog.get_last_error())
     *     return
     * print(prolog.is_initialized())  # true
     */
    bool initialize(godot::Dictionary const& p_options = godot::Dictionary());

    /**
     * @brief Abolishes user predicates added via consult_* / assert_fact.
     *
     * Leaves SWI-Prolog running and this handle attached. Bootstrap helpers
     * and expose_* wrappers installed through add_fact stay unless they
     * were tracked as added predicates. Open queries are cut first.
     *
     * @return false if this handle is not attached or the call is off-thread.
     */
    bool clear_knowledge();

    /**
     * @brief Detaches this handle from the process-global Prolog engine.
     *
     * Safe to call multiple times. Does not call PL_cleanup() (that happens
     * once when the GDExtension unloads). If this was the last attached
     * handle, user predicates are abolished so the next initialize() sees
     * a fresh knowledge base. After cleanup this handle must initialize()
     * again before consult / solve.
     *
     * @example
     * func _exit_tree():
     *     if prolog:
     *         prolog.cleanup()
     */
    void cleanup();

    /**
     * @brief Checks if the Prolog engine is currently initialized.
     *
     * @return true if initialize() succeeded and cleanup() was not called.
     *
     * @example
     * if not prolog.is_initialized():
     *     prolog.initialize()
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
     * predicates first, or call cleanup() then initialize() (the last
     * attached handle resets user predicates without restarting SWI).
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
    bool consult_file(godot::String const& p_filename);

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
     * predicates first, or call cleanup() then initialize() (the last
     * attached handle resets user predicates without restarting SWI).
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
    bool consult_string(godot::String const& p_prolog_code);

    // =========================================================================
    // Structured term factories (GDScript facade)
    //
    // These do not talk to SWI. They only build Godot objects; the same
    // constructors live on PrologTerm / PrologVariable / PrologPredicate /
    // PrologObject. They sit on Prologot so game code uses one handle
    // (prolog.atom(), prolog.predicate()) instead of four static APIs.
    // object() stores an instance id; putting the blob in a term needs
    // initialize() so the SWI blob type is registered.
    // =========================================================================

    /**
     * @brief Creates a Prolog atom term.
     *
     * A GDScript String passed to call() is already an atom. Use this
     * factory when you need an explicit PrologTerm on the way in
     * (inspection, as_text). Bound atoms from solution.get() are String,
     * not this wrapper.
     *
     * @param p_name Atom name (e.g. "tom").
     * @return A PrologTerm whose kind is atom.
     *
     * @example
     * var tom = prolog.atom("tom")
     * print(tom.as_text())  # tom
     * prolog.solve(prolog.predicate("parent").call(tom, "bob")).has_solution()
     */
    godot::Ref<PrologTerm> atom(godot::String const& p_name);

    /**
     * @brief Creates a Prolog integer term.
     *
     * A GDScript int passed to call() is already an integer.
     *
     * @param p_value Signed 64-bit integer.
     * @return A PrologTerm whose kind is integer.
     *
     * @example
     * var hp = prolog.integer(42)
     * print(hp.get_integer())  # 42
     */
    godot::Ref<PrologTerm> integer(int64_t p_value);

    /**
     * @brief Creates a Prolog floating-point term.
     *
     * A GDScript float passed to call() is already a real.
     *
     * @param p_value IEEE-754 double.
     * @return A PrologTerm whose kind is float.
     *
     * @example
     * var dist = prolog.real(3.14)
     * print(dist.get_real())  # 3.14
     */
    godot::Ref<PrologTerm> real(double p_value);

    /**
     * @brief Creates a Prolog string term (quoted, distinct from an atom).
     *
     * A GDScript String sent to call() is an atom, not a Prolog string.
     * Use this factory only when the clause stores a Prolog string.
     *
     * @param p_value String contents.
     * @return A PrologTerm whose kind is string.
     *
     * @example
     * var s = prolog.string("hello")
     * print(s.as_text())  # "hello"
     * prolog.assert_fact(prolog.predicate("title").call(s))
     */
    godot::Ref<PrologTerm> string(godot::String const& p_value);

    /**
     * @brief Creates the empty Prolog list [].
     *
     * @return A PrologTerm whose kind is nil.
     *
     * @example
     * print(prolog.nil().as_text())  # []
     * prolog.solve(prolog.predicate("empty").call(prolog.nil())).has_solution()
     */
    godot::Ref<PrologTerm> nil();

    /**
     * @brief Creates a Prolog list from a Godot Array.
     *
     * An empty Array becomes []. A Godot Array passed to call() is
     * already a list; use this factory for an explicit PrologTerm.
     *
     * @param p_items List elements (Variants or PrologTerm).
     * @return A PrologTerm whose kind is list, or nil if empty.
     *
     * @example
     * var items = prolog.list(["sword", "shield"])
     * print(items.as_text())  # [sword, shield]
     * if prolog.solve(prolog.predicate("member").call("sword", items)).has_solution():
     *     print("found")
     */
    godot::Ref<PrologTerm> list(godot::Array const& p_items);

    /**
     * @brief Creates a compound term functor(args...).
     *
     * For a query, prefer predicate(functor).call(...) which returns a
     * PrologGoal. Use compound() when you need a nested term as an argument.
     *
     * @param p_functor Functor name (e.g. "point").
     * @param p_args Arguments in order.
     * @return A PrologTerm whose kind is compound.
     *
     * @example
     * var point = prolog.compound("point", [10, 20])
     * print(point.as_text())  # point(10, 20)
     * prolog.assert_fact(prolog.predicate("at").call("hero", point))
     */
    godot::Ref<PrologTerm> compound(godot::String const& p_functor, godot::Array const& p_args);

    /**
     * @brief Creates a logical variable object.
     *
     * The optional name is for debug (as_text(), editor console). Solutions
     * are keyed by this object, not by the name. Two variable("X") calls
     * create two distinct variables. A String is never a variable.
     *
     * @param p_name Optional debug name (e.g. "Child"). Empty still has an id.
     * @return A PrologVariable to pass to call() and solution.get().
     *
     * @example
     * var child = prolog.variable("Child")
     * var parent = prolog.predicate("parent")
     * for solution in prolog.solve(parent.call("tom", child)):
     *     print(solution.get(child))  # bob, then liz
     *
     * var a1 = prolog.variable("A")
     * var a2 = prolog.variable("A")
     * print(a1.get_id() != a2.get_id())  # true
     */
    godot::Ref<PrologVariable> variable(godot::String const& p_name = godot::String());

    /**
     * @brief Creates a fresh anonymous variable (_).
     *
     * Each occurrence compiled into a goal is a new Prolog variable.
     * Anonymous variables do not appear in PrologSolution bindings.
     *
     * @return An anonymous PrologVariable.
     *
     * @example
     * var parent = prolog.predicate("parent")
     * # parent(tom, _) : succeed if tom has any child
     * if prolog.solve(parent.call("tom", prolog.anonymous())).has_solution():
     *     print("tom has at least one child")
     * prolog.retract_all(parent.call(prolog.anonymous(), prolog.anonymous()))
     */
    godot::Ref<PrologVariable> anonymous();

    /**
     * @brief Creates a reusable predicate (functor only, no stored arity).
     *
     * Arity comes from call(...) / callv([...]). The same object can build
     * parent/1 and parent/2. GDScript cannot write parent("tom", child);
     * call() is the goal constructor.
     *
     * @param p_name Functor name (e.g. "parent").
     * @return A PrologPredicate.
     *
     * @example
     * var parent = prolog.predicate("parent")
     * var child = prolog.variable("Child")
     * var unary: PrologGoal = parent.call("tom")          # parent/1
     * var binary: PrologGoal = parent.call("tom", child)  # parent/2
     */
    godot::Ref<PrologPredicate> predicate(godot::String const& p_name);

    /**
     * @brief Wraps a Godot Object (Node, Resource, ...) as a Prolog handle.
     *
     * A String is never an object handle. Pass the Node / Resource itself.
     * If the object is freed later, the handle stays but is_valid() is false.
     *
     * @param p_object Node, Resource, or any Object. Null returns null.
     * @return A PrologObject handle, or null.
     *
     * @example
     * var player = prolog.object($Player)
     * var at = prolog.predicate("at")
     * prolog.assert_fact(at.call(player, "zone_1"))
     * # Passing the Node to call() wraps it the same way:
     * prolog.solve(at.call($Player, "zone_1")).has_solution()
     */
    godot::Ref<PrologObject> object(godot::Object* p_object);

    // =========================================================================
    // High-level solving (structured terms, no Prolog source parsing)
    // =========================================================================

    /**
     * @brief Opens a lazy query for p_goal and returns a PrologQuery.
     *
     * Does **not** collect every answer up front. The SWI query
     * (`PL_open_query`) stays open; each `has_solution()` / `first()` /
     * `for` step pulls at most one more `PL_next_solution`. Destroying
     * the PrologQuery (end of a `for`, or a temporary going out of
     * scope) cuts leftover choice points.
     *
     * Strings passed to PrologPredicate.call() are atoms; only
     * PrologVariable objects are variables. A PrologQuery is always
     * truthy in GDScript — use has_solution() for a yes/no test.
     *
     * Reading API (see PrologQuery for the full write-up):
     *
     * - has_solution() — yes / no; at most one pull, then cut.
     * - first() — one PrologSolution or null; at most one pull, then cut.
     * - `for sol in query:` — one PrologSolution per turn; `break` cuts
     *   Prolog. GDScript does not unpack `for via, child in query`.
     * - all() — Array of every PrologSolution (drains what remains).
     * - There is no query.values(). Use all() + sol.get(var). A matrix
     *   (one row per solution) is a GDScript wrapper, not this method.
     *
     * Call from the same thread that called initialize(). Do not
     * cleanup() this handle while the returned query is still open.
     *
     * @param p_goal Goal from predicate.call() or conjunction /
     * disjunction / negated().
     * @param p_max_solutions 0 = unlimited. Otherwise stop after this
     * many answers (also applies to all() / a `for` without break).
     * Even when lazy, all() or an unbounded `for` on an infinite goal
     * (`between(1, inf, N)`) will not return.
     * @return A PrologQuery (never null; check has_solution()).
     *
     * @example
     * var parent = prolog.predicate("parent")
     * var child = prolog.variable("Child")
     * var via = prolog.variable("Via")
     * if prolog.solve(parent.call("tom", "bob")).has_solution():
     *     print("true")
     * var sol = prolog.solve(parent.call("tom", child)).first()
     * if sol != null:
     *     print(sol.get(child))   # bob
     * for sol in prolog.solve(parent.call("tom", child)):
     *     print(sol.get(child))   # bob, then liz
     *     if sol.get(child) == "bob":
     *         break
     * for sol in prolog.solve(parent.call("tom", via).conjunction(
     *         parent.call(via, child))):
     *     print(sol.get(via), "->", sol.get(child))
     * print(prolog.solve(parent.call("tom", child)).all().size())  # 2
     * prolog.solve(between.call(1, 1000000, n), 5).all()
     */
    godot::Ref<PrologQuery> solve(godot::Ref<PrologGoal> const& p_goal,
                                  int64_t p_max_solutions = 0);

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
     *
     * @example
     * if not prolog.consult_file("res://missing.pl"):
     *     push_error(prolog.get_last_error())
     */
    godot::String get_last_error() const;

    // =========================================================================
    // Dynamic Assertions
    // =========================================================================

    /**
     * @brief Asserts a PrologGoal as a fact (assertz/1).
     *
     * @param p_goal Ground or open goal to add as a clause.
     * @return true if assertz succeeded.
     *
     * @example
     * var parent = prolog.predicate("parent")
     * prolog.assert_fact(parent.call("tom", "bob"))
     * prolog.solve(parent.call("tom", "bob")).has_solution()  # true
     */
    bool assert_fact(godot::Ref<PrologGoal> const& p_goal);

    /**
     * @brief Removes the first clause that unifies with p_goal (retract/1).
     *
     * @param p_goal Goal used as the retract pattern.
     * @return true if a clause was removed.
     *
     * @example
     * var parent = prolog.predicate("parent")
     * prolog.retract_fact(parent.call("tom", "bob"))
     */
    bool retract_fact(godot::Ref<PrologGoal> const& p_goal);

    /**
     * @brief Removes every clause that unifies with p_goal (retractall/1).
     *
     * Use anonymous() for “don’t care” arguments. String patterns are
     * not accepted — pass a PrologGoal.
     *
     * @param p_goal Goal used as the retractall pattern.
     * @return true if retractall succeeded (also when nothing matched).
     *
     * @example
     * var parent = prolog.predicate("parent")
     * prolog.retract_all(parent.call("tom", prolog.anonymous()))
     * prolog.retract_all(parent.call(prolog.anonymous(), prolog.anonymous()))
     */
    bool retract_all(godot::Ref<PrologGoal> const& p_goal);

    // =========================================================================
    // Explicit Godot → Prolog exposure (properties and methods)
    // =========================================================================

    /**
     * @brief Exposes a Godot property as a relational Prolog predicate.
     *
     * Creates `predicate(Object, Value)`. If Value is unbound, it is unified
     * with the current property. If Value is ground, the goal succeeds only
     * when it unifies with the current value (no implicit setter).
     *
     * The class name filters with Object.is_class(). Empty class accepts any
     * live object. Property `name` defaults to the predicate `node_name` to
     * avoid clashing with SWI-Prolog name/2.
     *
     * @param p_class Godot class filter (e.g. "Node", "Node2D"), or empty.
     * @param p_property Property name (e.g. "health", "position").
     * @param p_predicate Prolog functor. Empty → property name (or node_name).
     * @return true if the wrapper clause was installed.
     *
     * @example
     * prolog.expose_property("Node", "name", "node_name")
     * prolog.expose_property("Node2D", "position")
     * var name = prolog.variable("Name")
     * var sol = prolog.solve(prolog.predicate("node_name").call($Player, name)).first()
     * print(sol.get(name))  # Player
     */
    bool expose_property(godot::String const& p_class,
                         godot::String const& p_property,
                         godot::String const& p_predicate = godot::String());

    /**
     * @brief Exposes a Godot method as a Prolog predicate.
     *
     * Arity is 1 (the object) + required arguments + 1 if the method returns
     * a non-NIL value. A void method has no result argument.
     *
     * @param p_class Godot class filter, or empty.
     * @param p_method Method name (e.g. "get_class").
     * @param p_predicate Prolog functor. Empty → method name.
     * @return true if the wrapper clause was installed.
     *
     * @example
     * prolog.expose_method("Object", "get_class", "godot_class")
     * var cls = prolog.variable("Class")
     * var sol = prolog.solve(prolog.predicate("godot_class").call($Player, cls)).first()
     * print(sol.get(cls))  # Node
     */
    bool expose_method(godot::String const& p_class,
                       godot::String const& p_method,
                       godot::String const& p_predicate = godot::String());

    /**
     * @brief Removes an exposed wrapper predicate (retractall + bookkeeping).
     *
     * @param p_predicate Functor installed by expose_property / expose_method.
     * @param p_arity Arity of that wrapper (usually 2 for a property).
     * @return true if the wrapper was removed from the bookkeeping list.
     *
     * @example
     * prolog.unexpose("node_name", 2)
     * prolog.solve(prolog.predicate("node_name").call($Player, "Hero")).has_solution()
     * # false — the wrapper is gone
     */
    bool unexpose(godot::String const& p_predicate, int p_arity);

    /**
     * @brief Returns the list of currently exposed members.
     *
     * Each item is a Dictionary: kind, class, member, predicate, arity.
     *
     * @return Array of Dictionaries (empty if nothing is exposed).
     *
     * @example
     * for item in prolog.list_exposed():
     *     print(item["predicate"], "/", item["arity"], " -> ", item["member"])
     */
    godot::Array list_exposed() const;

    /**
     * @brief SWI foreign: prologot_property(Class, Property, Object, Value).
     */
    static bool foreign_property(term_t p_class,
                                 term_t p_property,
                                 term_t p_object,
                                 term_t p_value);

    /**
     * @brief SWI foreign: prologot_method(Class, Method, Object, Args, Result).
     */
    static bool foreign_method(term_t p_class,
                               term_t p_method,
                               term_t p_object,
                               term_t p_args,
                               term_t p_result);

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
     *
     * @example
     * if prolog.predicate_exists("calculate_total_tax", 2):
     *     var tax = prolog.variable("Tax")
     *     var sol = prolog.solve(prolog.predicate("calculate_total_tax").call("zorglub", tax)).first()
     */
    bool predicate_exists(godot::String const& p_predicate, int p_arity);

    /**
     * @brief Lists all currently defined predicates.
     *
     * This method uses Prolog's current_predicate/1 to query for all predicates
     * currently in the knowledge base.
     *
     * @return Array of Dictionary objects describing each predicate.
     *
     * @example
     * # List all predicates in the knowledge base
     * var predicates = prolog.list_predicates()
     * # Returns: [{"functor": "parent", "args": ["/", 2]}, ...]
     * # Format: Name/Arity
     */
    godot::Array list_predicates();

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
    std::pair<godot::String, godot::String> set_swi_home_dir(godot::String const& p_home_option);

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
    static godot::String resolve_godot_path(godot::String const& p_path);

    friend class PrologQuery;

    /**
     * @brief Calls a unary built-in (assertz, retract, retractall) on a goal.
     */
    bool apply_clause_predicate(char const* p_name,
                                godot::Ref<PrologGoal> const& p_goal,
                                godot::String const& p_context);

    /**
     * @brief Helper to push error messages respecting error handling options.
     *
     * This method checks the error handling options ("on error", "on warning")
     * and either prints the error, halts, or only stores it in m_last_error.
     *
     * @param p_message The error message to handle.
     * @param p_type The error type ("error" or "warning").
     */
    void push_error(godot::String const& p_message, godot::String const& p_type = "error");

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
    bool handle_prolog_exception(qid_t p_qid, godot::String const& p_context);

    /**
     * @brief Internal assertz of a Prolog source clause (used by expose_*).
     */
    bool add_fact(godot::String const& p_fact);

    /**
     * @brief Internal yes/no string query (editor / bootstrap helpers).
     */
    bool query_text(godot::String const& p_goal);

    /**
     * @brief Internal findall wrapper around a string goal.
     */
    godot::Array query_text_all(godot::String const& p_goal);

    /**
     * @brief Internal first-solution string query.
     */
    godot::Variant query_text_one(godot::String const& p_goal);

    /**
     * @brief Parses a typed editor goal and returns named bindings.
     *
     * Bound to GDScript as `_editor_query`. Game code should use solve().
     *
     * @example
     * # Editor console only
     * var rows = prolog._editor_query("parent(tom, X)")
     * # [{"X": "bob"}, {"X": "liz"}]
     */
    godot::Array query_text_named(godot::String const& p_goal);

    /**
     * @brief Registers prologot_property/4 and prologot_method/5 after init.
     */
    bool register_foreign_predicates();

    /**
     * @brief Marks this handle as attached to the already-running engine.
     */
    bool attach_handle();

    /**
     * @brief Abolishes user predicates while keeping Prologot bootstrap helpers.
     */
    void reset_user_knowledge();

    /**
     * @brief Logs and fails if the caller is not the Godot main thread.
     *
     * SWI is one process-global engine; Prologot instances are handles.
     */
    static bool require_main_thread(char const* p_where);

    /**
     * @brief Retracts expose_* wrappers installed by this handle.
     */
    void uninstall_exposed();

    struct ExposedBinding
    {
        godot::String kind;
        godot::String class_name;
        godot::String member;
        godot::String predicate;
        int arity = 0;
    };

    /** Whether this handle is attached to the process-global engine. */
    bool m_initialized;

    /** Wrappers installed by expose_property / expose_method. */
    std::vector<ExposedBinding> m_exposed;

    /** Last error message from Prolog. */
    godot::String m_last_error;

    /** Error handling option: "print", "halt", or "status". */
    godot::String m_on_error;

    /** Warning handling option: "print", "halt", or "status". */
    godot::String m_on_warning;

    /**
     * @brief C++ singleton handle used by foreign predicates.
     *
     * Never overwritten while it still points at a live instance.
     */
    static Prologot* m_singleton;
};

} // namespace prologot
