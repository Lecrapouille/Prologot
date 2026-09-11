/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file defines PrologPredicate: a reusable name/arity that builds a
 * PrologGoal through bind(). GDScript cannot write parent("tom", child) on
 * a stored object; bind() is the explicit replacement for Object.call().
 */

#pragma once

#include "PrologGoal.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

/**
 * @class PrologPredicate
 * @brief Reusable Prolog predicate (name/arity) that builds goals via bind().
 *
 * Create with prolog.predicate("parent", 2). Call bind(...) with exactly
 * that many arguments; a String is always an atom, a PrologVariable is a
 * variable. bindv(Array) is the same with an explicit array.
 *
 * @example
 * var parent = prolog.predicate("parent", 2)
 * print(parent.as_text())  # parent/2
 *
 * var child = prolog.variable()
 * var goal = parent.bind("tom", child)
 * for solution in prolog.solve(goal):
 *     print(solution.get(child))
 */
class PrologPredicate: public RefCounted
{
    GDCLASS(PrologPredicate, RefCounted)

public:

    /**
     * @brief Constructs an empty predicate (filled by create()).
     */
    PrologPredicate() = default;

    /**
     * @brief Destructs the predicate. RefCounted releases it automatically.
     */
    ~PrologPredicate() override = default;

    /**
     * @brief Creates a predicate with the given name and arity.
     *
     * Prefer prolog.predicate(name, arity) from GDScript.
     *
     * @param p_name Functor name (e.g. "parent").
     * @param p_arity Number of arguments bind() must receive.
     * @return A new PrologPredicate.
     *
     * @example
     * var parent = prolog.predicate("parent", 2)
     * print(parent.get_name())   # parent
     * print(parent.get_arity())  # 2
     */
    static Ref<PrologPredicate> create(String const& p_name, int p_arity);

    /**
     * @brief Returns the functor name.
     */
    String get_name() const { return m_name; }

    /**
     * @brief Returns the declared arity.
     */
    int get_arity() const { return m_arity; }

    /**
     * @brief Returns a debug label Name/Arity.
     *
     * @example
     * print(prolog.predicate("parent", 2).as_text())  # parent/2
     */
    String as_text() const;

    /**
     * @brief Builds a PrologGoal if p_args.size() equals the arity.
     *
     * Bound to GDScript as bindv(args). The vararg bind(...) calls this
     * after collecting arguments. A wrong arity pushes an error and
     * returns a null Ref.
     *
     * @param p_args Arguments in order (atoms, numbers, lists, PrologTerm,
     * PrologVariable).
     * @return A PrologGoal, or null if the arity does not match.
     *
     * @example
     * var parent = prolog.predicate("parent", 2)
     * var child = prolog.variable()
     * var goal = parent.bindv(["tom", child])
     */
    Ref<PrologGoal> bind(Array const& p_args) const;

    /**
     * @brief Vararg entry used by GDScript bind(arg1, arg2, ...).
     *
     * @example
     * var parent = prolog.predicate("parent", 2)
     * var child = prolog.variable()
     * var goal = parent.bind("tom", child)
     * prolog.succeeds(parent.bind("tom", "bob"))
     */
    Variant bind_varargs(Variant const** p_args,
                         GDExtensionInt p_arg_count,
                         GDExtensionCallError& r_error);

protected:

    /**
     * @brief Binds get_name / get_arity / as_text / bind / bindv to GDScript.
     */
    static void _bind_methods();

private:

    String m_name;
    int m_arity = 0;
};
