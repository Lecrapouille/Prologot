/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file defines PrologQuery: the lazy result of Prologot.solve().
 * See the class comment below for the full GDScript reading API
 * (has_solution / first / for / all). There is no query.values().
 */

#pragma once

#include "PrologGoal.hpp"
#include "PrologSolution.hpp"
#include <cstdint>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <memory>

namespace prologot
{

class Prologot;

/**
 * @class PrologQuery
 * @brief Lazy stream of PrologSolution values returned by solve().
 *
 * solve() opens an SWI query (`PL_open_query` + foreign frame). Each
 * later call pulls at most one more answer with `PL_next_solution`.
 * Nothing is collected up front: `first()` is a real solve-one, and a
 * `break` in GDScript plus destroying this object cuts remaining Prolog
 * choice points (`PL_cut_query`). An infinite goal such as
 * `between(1, inf, N)` therefore does not block before the first
 * `get()`.
 *
 * # How to read answers (GDScript)
 *
 * Several **solutions** = several `PrologSolution` objects (`for` /
 * `all()` / `first()`). Several **variables of one** solution stay on
 * that object: `sol.get(foo)` then `sol.get(bar)`. There is no
 * `get(foo, bar)`. GDScript cannot write `for via, child in query`.
 * There is no `query.values()` — use `all()` + `sol.get(var)`, or a
 * small wrapper if you want a matrix (one row per solution).
 *
 * Knowledge used in the examples:
 *
 *     parent(tom, bob). parent(tom, liz). parent(bob, ann).
 *
 *     var prolog := Prologot.new()
 *     prolog.initialize()
 *     var parent = prolog.predicate("parent")
 *     var child = prolog.variable("Child")
 *     var via = prolog.variable("Via")
 *
 * ## has_solution() — yes / no (at most one pull, then cut)
 *
 *     if prolog.solve(parent.call("tom", "bob")).has_solution():
 *         print("true")
 *
 * A ground goal that holds still yields one (empty) PrologSolution, so
 * this is true. The remaining choice points are cut so a temporary
 * `solve(g).has_solution()` does not keep Prolog open until the caller
 * returns. After this call, `all()` / `for` on the **same** object only
 * see the cached first answer. Open a new `solve()` to search again.
 *
 * ## first() — one PrologSolution or null (at most one pull, then cut)
 *
 *     var sol = prolog.solve(parent.call("tom", child)).first()
 *     if sol != null:
 *         print(sol.get(child))   # bob
 *
 * Same cut as has_solution(). Use `for` / `all()` when you need every
 * answer.
 *
 * ## for (`_iter_*`) — one solution per turn; break cuts Prolog
 *
 *     for sol in prolog.solve(parent.call("tom", child)):
 *         print(sol.get(child))   # bob, then liz
 *         if sol.get(child) == "bob":
 *             break               # remaining answers are not computed
 *
 * Two variables: still one `sol` per turn.
 *
 *     for sol in prolog.solve(parent.call("tom", via).conjunction(
 *             parent.call(via, child))):
 *         print(sol.get(via), "->", sol.get(child))   # bob -> ann
 *
 * `_iter_init` is a single pass from the start of the cache. Relooping
 * `for` on the same query replays solutions already pulled; it does not
 * reopen Prolog. After a `break`, `all()` continues and pulls the rest.
 *
 * ## all() — Array of every PrologSolution (drains what remains)
 *
 *     var sols = prolog.solve(parent.call("tom", child)).all()
 *     print(sols.size())             # 2
 *     print(sols[0].get(child))      # bob
 *
 * ## Optional cap
 *
 *     prolog.solve(between.call(1, 1000000, n), 5).all()   # 5, not 1e6
 *
 * `max_solutions` of `0` (default) means unlimited. Even when lazy,
 * `all()` or a `for` without `break` on an infinite goal will not
 * return.
 *
 * ## Constraints
 *
 * Call from the same thread that initialized the engine. Do not
 * `cleanup()` the Prologot handle while a query is still open. A
 * PrologQuery is RefCounted, so `if prolog.solve(goal):` is always
 * true — use has_solution().
 */
class PrologQuery: public godot::RefCounted
{
    GDCLASS(PrologQuery, godot::RefCounted)

public:

    /**
     * @brief Constructs an empty query (no solutions, no open SWI query).
     *
     * Prefer Prologot.solve() from GDScript. Tests may use create().
     */
    PrologQuery();

    /**
     * @brief Cuts or closes an open SWI query, then releases the frame.
     */
    ~PrologQuery() override;

    /**
     * @brief Wraps an Array of PrologSolution as a finished query.
     *
     * Used by tests. Game code uses Prologot.solve().
     *
     * @param p_solutions Solutions in the order Prolog produced them.
     * @return A new PrologQuery (empty if p_solutions is empty).
     */
    static godot::Ref<PrologQuery> create(godot::Array const& p_solutions = godot::Array());

    /**
     * @brief Opens an SWI query for p_goal (used by Prologot.solve()).
     *
     * @param p_engine Engine that compiled the goal (for errors).
     * @param p_goal Goal to prove.
     * @param p_max_solutions 0 = unlimited; otherwise stop after this many.
     * @return A PrologQuery (never null; may have no solutions).
     */
    static godot::Ref<PrologQuery> open(Prologot* p_engine,
                                        godot::Ref<PrologGoal> const& p_goal,
                                        int64_t p_max_solutions = 0);

    /**
     * @brief Cuts every still-open SWI query (last-handle knowledge wipe).
     */
    static void abandon_all_open();

    /**
     * @brief Returns true if the query produced at least one solution.
     *
     * Pulls at most one `PL_next_solution`, then cuts remaining choice
     * points. A temporary `solve(g).has_solution()` therefore does not
     * keep a query open until the GDScript function ends. A ground goal
     * that holds yields one empty PrologSolution, so this is still true.
     *
     * After this call, further iteration on **this** object only sees
     * the cached first answer. Open a new solve() to search again.
     *
     * @example
     * if prolog.solve(parent.call("tom", "bob")).has_solution():
     *     print("true")
     */
    bool has_solution();

    /**
     * @brief Returns the first PrologSolution, or null if there is none.
     *
     * Pulls at most one solution, then cuts remaining choice points
     * (same as has_solution()). This is a real solve-one: later answers
     * are not computed. Use `for` / `all()` when you need every answer.
     *
     * @example
     * var sol = prolog.solve(parent.call("tom", child)).first()
     * if sol != null:
     *     print(sol.get(child))   # bob
     */
    godot::Variant first();

    /**
     * @brief Drains remaining solutions and returns every PrologSolution.
     *
     * Already-pulled answers stay at the front of the array. After a
     * `break` in `for`, this continues and pulls the rest. After
     * has_solution() / first(), only the cached first answer remains
     * (those methods cut). Prefer `for` when you may stop early.
     *
     * There is no `query.values()`. Read columns with `sol.get(var)`.
     *
     * @return Array of PrologSolution (empty if the goal failed).
     *
     * @example
     * var sols = prolog.solve(parent.call("tom", child)).all()
     * print(sols.size())             # 2
     * print(sols[0].get(child))      # bob
     */
    godot::Array all();

    /**
     * @brief Starts a GDScript `for` loop over the solutions.
     *
     * Bound as `_iter_init`. Pulls the first answer if the cache is
     * empty. Relooping the same query replays the cache from index 0
     * and does not reopen Prolog. A `break` leaves the query open so
     * `all()` can still drain the rest; destroying the query cuts
     * leftover choice points.
     *
     * @example
     * for sol in prolog.solve(parent.call("tom", child)):
     *     print(sol.get(child))
     *     if sol.get(child) == "bob":
     *         break
     */
    bool _iter_init(godot::Variant const& p_iter);

    /**
     * @brief Advances a GDScript `for` loop, pulling the next solution.
     *
     * Bound as `_iter_next`. Returns false when there is no further
     * answer (or the optional max_solutions cap is reached).
     */
    bool _iter_next(godot::Variant const& p_iter);

    /**
     * @brief Returns the current PrologSolution during a GDScript `for`.
     *
     * Bound as `_iter_get`. One object per turn; read each variable
     * with `sol.get(var)`.
     */
    godot::Variant _iter_get(godot::Variant const& p_iter);

protected:

    static void _bind_methods();

private:

    struct State;

    void close_query(bool p_cut_remaining);
    bool pull_one();
    bool ensure_count(int p_count);

    std::unique_ptr<State> m_state;
    godot::Array m_solutions;
    int m_iter_index = 0;
    int64_t m_max_solutions = 0;
};

} // namespace prologot
