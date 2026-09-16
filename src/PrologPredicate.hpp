/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file defines PrologPredicate: a reusable functor that builds a
 * PrologGoal through call(). Arity is taken from the argument list.
 * GDScript cannot write parent("tom", child) on a stored object; call()
 * is the explicit constructor for a goal.
 */

#pragma once

#include "PrologGoal.hpp"
#include <gdextension_interface.h>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

namespace prologot
{

/**
 * @class PrologPredicate
 * @brief Reusable Prolog functor that builds goals via call().
 *
 * Create with prolog.predicate("parent"). call(...) receives any number
 * of arguments; a String is always an atom, a PrologVariable is a
 * variable. callv(Array) is the same with an explicit array.
 *
 * @example
 * var prolog := Prologot.new()
 * prolog.initialize()
 * var parent = prolog.predicate("parent")
 * print(parent.as_text())  # parent
 *
 * var child = prolog.variable("Child")
 * var goal = parent.call("tom", child)
 * for solution in prolog.solve(goal):
 *     print(solution.get(child))
 */
class PrologPredicate: public godot::RefCounted
{
    GDCLASS(PrologPredicate, godot::RefCounted)

public:

    /**
     * @brief Constructs an empty predicate (filled by create()).
     *
     * Prefer prolog.predicate(name) from GDScript.
     */
    PrologPredicate() = default;

    /**
     * @brief Destructs the predicate. RefCounted releases it automatically.
     */
    ~PrologPredicate() override = default;

    /**
     * @brief Creates a predicate with the given functor name.
     *
     * Prefer prolog.predicate(name) from GDScript. No arity is stored:
     * parent.call("tom") is parent/1, parent.call("tom", child) is parent/2.
     *
     * @param p_name Functor name (e.g. "parent", "member").
     * @return A reusable PrologPredicate.
     *
     * @example
     * var prolog := Prologot.new()
     * prolog.initialize()
     * var parent = prolog.predicate("parent")
     * print(parent.get_name())  # parent
     */
    static godot::Ref<PrologPredicate> create(godot::String const& p_name);

    /**
     * @brief Returns the functor name stored on this predicate.
     *
     * Same value as as_text(). Arity is not part of the name.
     *
     * @return The functor, e.g. "parent".
     *
     * @example
     * print(prolog.predicate("member").get_name())  # member
     */
    godot::String get_name() const { return m_name; }

    /**
     * @brief Returns the functor name for debug and the REPL.
     *
     * @return The functor only (no "/2" suffix).
     *
     * @example
     * print(prolog.predicate("parent").as_text())  # parent
     */
    godot::String as_text() const;

    /**
     * @brief Builds a PrologGoal from an argument Array.
     *
     * Bound to GDScript as callv(args). A String is always an atom; only
     * a PrologVariable is a variable. A Node / Resource is wrapped as
     * PrologObject. Arity is args.size().
     *
     * @param p_args Arguments in predicate order.
     * @return A PrologGoal ready for solve(), assert_fact(), or composition.
     *
     * @example
     * var prolog := Prologot.new()
     * prolog.initialize()
     * var parent = prolog.predicate("parent")
     * var child = prolog.variable("Child")
     * var goal = parent.callv(["tom", child])
     * print(goal.as_text())  # parent(tom, Child)
     */
    godot::Ref<PrologGoal> make_goal(godot::Array const& p_args) const;

    /**
     * @brief Vararg entry bound to GDScript as call(arg1, arg2, ...).
     *
     * C++ keeps the name make_goal_varargs so Object::call is not hidden
     * in this class. From GDScript write parent.call("tom", child).
     *
     * @param p_args Argument pointers supplied by Godot.
     * @param p_arg_count Number of arguments (the Prolog arity).
     * @param r_error Filled if Godot rejects the call.
     * @return A PrologGoal Variant, or null on failure.
     *
     * @example
     * var prolog := Prologot.new()
     * prolog.initialize()
     * var parent = prolog.predicate("parent")
     * var child = prolog.variable("Child")
     * var goal: PrologGoal = parent.call("tom", child)
     * if prolog.solve(goal).has_solution():
     *     print("tom has a child")
     */
    godot::Variant make_goal_varargs(godot::Variant const** p_args,
                              GDExtensionInt p_arg_count,
                              GDExtensionCallError& r_error);

protected:

    /**
     * @brief Binds get_name / as_text / call / callv to GDScript.
     */
    static void _bind_methods();

private:

    godot::String m_name;
};

} // namespace prologot
