/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file defines PrologQuery: the result of Prologot.solve().
 * GDScript objects are always truthy, so use has_solution() instead of
 * `if query:`. Iterate the query, or call first() / all().
 */

#pragma once

#include "PrologSolution.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace prologot
{

/**
 * @class PrologQuery
 * @brief Eager collection of PrologSolution values returned by solve().
 *
 * Solutions are collected immediately (no open SWI query). A PrologQuery
 * is a RefCounted object, so `if prolog.solve(goal):` is always true.
 * Use has_solution() for a yes/no test, first() for one answer, or
 * iterate / all() for every answer.
 *
 * @example
 * var parent = prolog.predicate("parent")
 * var child = prolog.variable("Child")
 * var query = prolog.solve(parent.call("tom", child))
 *
 * if query.has_solution():
 *     print(query.first().get(child))  # bob
 *
 * for solution in query:
 *     print(solution.get(child))       # bob, then liz
 *
 * print(query.all().size())            # 2
 */
class PrologQuery: public godot::RefCounted
{
    GDCLASS(PrologQuery, godot::RefCounted)

public:

    /**
     * @brief Constructs an empty query (no solutions).
     *
     * Prefer Prologot.solve() from GDScript. Tests may use create().
     */
    PrologQuery() = default;

    /**
     * @brief Destructs the query. RefCounted releases it automatically.
     */
    ~PrologQuery() override = default;

    /**
     * @brief Wraps an Array of PrologSolution as a query.
     *
     * Used by Prologot.solve() after collect_goal_solutions().
     *
     * @param p_solutions Solutions in the order Prolog produced them.
     * @return A new PrologQuery (empty if p_solutions is empty).
     */
    static godot::Ref<PrologQuery> create(godot::Array const& p_solutions = godot::Array());

    /**
     * @brief Returns true if the query produced at least one solution.
     *
     * Replaces the former succeeds() API. A ground goal that holds
     * yields one empty PrologSolution, so this is still true.
     *
     * @return true if all() is not empty.
     *
     * @example
     * var parent = prolog.predicate("parent")
     * if prolog.solve(parent.call("tom", "bob")).has_solution():
     *     print("tom is a parent of bob")
     * if not prolog.solve(parent.call("bob", "tom")).has_solution():
     *     print("the reverse fact is missing")
     */
    bool has_solution() const;

    /**
     * @brief Returns the first PrologSolution, or null if there is none.
     *
     * Replaces the former solve_one() API. Later solutions are still
     * available via iteration or all().
     *
     * @return The first PrologSolution, or a null Variant.
     *
     * @example
     * var parent = prolog.predicate("parent")
     * var child = prolog.variable("Child")
     * var solution = prolog.solve(parent.call("tom", child)).first()
     * if solution != null:
     *     print(solution.get(child))  # bob
     */
    godot::Variant first() const;

    /**
     * @brief Returns every PrologSolution as a Godot Array.
     *
     * Prefer `for solution in query:` when you only need to walk the
     * answers. Use all() when you need the size or random access.
     *
     * @return Array of PrologSolution (empty if the goal failed).
     *
     * @example
     * var parent = prolog.predicate("parent")
     * var child = prolog.variable("Child")
     * var solutions = prolog.solve(parent.call("tom", child)).all()
     * print(solutions.size())            # 2
     * print(solutions[0].get(child))     # bob
     */
    godot::Array all() const;

    /**
     * @brief Starts a GDScript `for` loop over the solutions.
     *
     * Bound as `_iter_init`. Resets the cursor to the first solution.
     *
     * @param p_iter Unused iterator state (Godot protocol).
     * @return true if there is at least one solution.
     *
     * @example
     * for solution in prolog.solve(parent.call("tom", child)):
     *     print(solution.get(child))
     */
    bool _iter_init(godot::Variant const& p_iter);

    /**
     * @brief Advances a GDScript `for` loop to the next solution.
     *
     * @param p_iter Unused iterator state (Godot protocol).
     * @return true if another solution remains.
     */
    bool _iter_next(godot::Variant const& p_iter);

    /**
     * @brief Returns the current solution during a GDScript `for` loop.
     *
     * @param p_iter Unused iterator state (Godot protocol).
     * @return The current PrologSolution, or null if the cursor is past the end.
     */
    godot::Variant _iter_get(godot::Variant const& p_iter);

protected:

    /**
     * @brief Binds has_solution / first / all and the `_iter_*` protocol.
     */
    static void _bind_methods();

private:

    godot::Array m_solutions;
    int m_iter_index = 0;
};

} // namespace prologot
