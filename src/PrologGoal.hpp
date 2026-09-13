/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file defines PrologGoal: an explicit goal built from a predicate
 * call() or from conjunction / disjunction / negation. Game code passes
 * this object to Prologot.solve().
 */

#pragma once

#include "PrologTerm.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

/**
 * @class PrologGoal
 * @brief Explicit Prolog goal built from predicates and compositions.
 *
 * A goal is a compound term (parent(tom, Child)) or a composition:
 * conjunction (','/2), disjunction (';'/2), negation (\\+/1).
 * GDScript keywords and/or cannot be method names, so the API uses
 * conjunction() / disjunction() / negated().
 *
 * Cut and meta-predicates are not exposed yet.
 *
 * @example
 * var parent = prolog.predicate("parent")
 * var via = prolog.variable()
 * var grandchild = prolog.variable()
 * var chain = parent.call("tom", via).conjunction(parent.call(via, grandchild))
 * for solution in prolog.solve(chain):
 *     print(solution.get(via), " -> ", solution.get(grandchild))
 */
class PrologGoal: public RefCounted
{
    GDCLASS(PrologGoal, RefCounted)

public:

    /**
     * @brief Constructs an empty goal (filled by from_compound() or call()).
     */
    PrologGoal() = default;

    /**
     * @brief Destructs the goal. RefCounted releases it automatically.
     */
    ~PrologGoal() override = default;

    /**
     * @brief Creates a goal functor(args...).
     *
     * Prefer PrologPredicate.call() from GDScript.
     *
     * @param p_functor Functor name (e.g. "parent", ",", ";", "\\+").
     * @param p_args Arguments in order.
     * @return A new PrologGoal.
     */
    static Ref<PrologGoal> from_compound(String const& p_functor,
                                         Array const& p_args);

    /**
     * @brief Returns the outermost functor name.
     *
     * @example
     * print(parent.call("tom", child).get_functor())  # parent
     */
    String get_functor() const { return m_functor; }

    /**
     * @brief Returns the outermost arguments.
     *
     * For a conjunction the two arguments are the left and right PrologGoal.
     *
     * @return Array of arguments (atoms, variables, nested goals, ...).
     *
     * @example
     * var goal: PrologGoal = parent.call("tom", child)
     * print(goal.get_args())  # ["tom", <PrologVariable>]
     */
    Array get_args() const { return m_args; }

    /**
     * @brief Returns the number of outermost arguments.
     *
     * This is the arity of this goal, not of the PrologPredicate.
     *
     * @return args.size() (2 for parent(tom, Child), 2 for a conjunction).
     *
     * @example
     * print(parent.call("tom", child).get_arity())  # 2
     * print(parent.call("tom").get_arity())         # 1
     */
    int get_arity() const { return m_args.size(); }

    /**
     * @brief Returns a readable Prolog-like representation.
     *
     * @example
     * print(parent.call("tom", child).as_text())  # parent(tom, Child)
     */
    String as_text() const;

    /**
     * @brief Converts this goal to a compound PrologTerm.
     *
     * Useful for inspection; solve() compiles the goal directly.
     *
     * @return A PrologTerm whose functor and args match this goal.
     *
     * @example
     * var term = parent.call("tom", "bob").to_term()
     * print(term.is_compound())  # true
     * print(term.as_text())      # parent(tom, bob)
     */
    Ref<PrologTerm> to_term() const;

    /**
     * @brief Returns this , Other (Prolog conjunction).
     *
     * Both subgoals must succeed. Shared PrologVariable objects stay
     * unified across the two sides.
     *
     * @param p_other Right-hand goal. Null returns a null Ref.
     * @return A goal whose functor is ",".
     *
     * @example
     * var parent = prolog.predicate("parent")
     * var via = prolog.variable()
     * var goal = parent.call("tom", via).conjunction(parent.call(via, "ann"))
     * prolog.solve(goal).has_solution()  # true if tom -> via -> ann
     */
    Ref<PrologGoal> conjunction(Ref<PrologGoal> const& p_other) const;

    /**
     * @brief Returns this ; Other (Prolog disjunction).
     *
     * Succeeds if either subgoal succeeds.
     *
     * @param p_other Right-hand goal. Null returns a null Ref.
     * @return A goal whose functor is ";".
     *
     * @example
     * var animal = prolog.predicate("animal")
     * var goal = animal.call("dog").disjunction(animal.call("cat"))
     */
    Ref<PrologGoal> disjunction(Ref<PrologGoal> const& p_other) const;

    /**
     * @brief Returns \\+(this) (Prolog negation as failure).
     *
     * @return A goal whose functor is "\\+".
     *
     * @example
     * var parent = prolog.predicate("parent")
     * prolog.solve(parent.call("bob", "tom").negated()).has_solution()
     */
    Ref<PrologGoal> negated() const;

protected:

    /**
     * @brief Binds inspectors and composition methods to GDScript.
     */
    static void _bind_methods();

private:

    String m_functor;
    Array m_args;
};
