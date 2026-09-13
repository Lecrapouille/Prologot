/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file defines PrologSolution: one set of variable bindings from
 * solve(). GDScript cannot use solution[child] via _get (keys are
 * StringName only); use get(variable) / has(variable) or the Dictionary
 * returned by get_bindings().
 */

#pragma once

#include "PrologVariable.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/**
 * @class PrologSolution
 * @brief One set of variable bindings from solve().
 *
 * Access values with get(variable) / has(variable). get_bindings() is a
 * Dictionary keyed by the same PrologVariable objects used in the goal.
 * Anonymous variables are omitted. Atoms become String, lists become
 * Array; compounds stay as PrologTerm or the existing Dictionary form.
 *
 * @example
 * var parent = prolog.predicate("parent")
 * var child = prolog.variable("Child")
 * for solution in prolog.solve(parent.call("tom", child)):
 *     if solution.has(child):
 *         print(solution.get(child))
 *     print(solution.get_bindings()[child])
 */
class PrologSolution: public RefCounted
{
    GDCLASS(PrologSolution, RefCounted)

public:

    /**
     * @brief Constructs an empty solution (no bindings).
     */
    PrologSolution() = default;

    /**
     * @brief Destructs the solution. RefCounted releases it automatically.
     */
    ~PrologSolution() override = default;

    /**
     * @brief Creates an empty solution.
     *
     * Used internally by solve(); tests may fill it with put().
     *
     * @return A new PrologSolution.
     */
    static Ref<PrologSolution> create();

    /**
     * @brief Stores a binding for a variable.
     *
     * Null variables are ignored. Used by the solver and by unit tests
     * that build a solution by hand.
     *
     * @param p_variable The PrologVariable object used in the goal.
     * @param p_value Bound Godot value (atom as String, list as Array, ...).
     *
     * @example
     * var child = prolog.variable()
     * var solution = PrologSolution.new()
     * solution.put(child, "bob")
     * print(solution.get(child))  # bob
     */
    void put(Ref<PrologVariable> const& p_variable, Variant const& p_value);

    /**
     * @brief Returns the value bound to p_variable, or null if unbound.
     *
     * Object-key indexing (solution[child]) does not work via _get.
     *
     * @param p_variable The same object passed to call().
     * @return The bound Variant, or a null Variant.
     *
     * @example
     * var child = prolog.variable("Child")
     * var solution = prolog.solve(parent.call("tom", child)).first()
     * print(solution.get(child))  # bob
     */
    Variant get(Ref<PrologVariable> const& p_variable) const;

    /**
     * @brief Returns true if p_variable has a binding in this solution.
     *
     * @param p_variable The same object passed to call().
     * @return true if get() would return a stored value.
     *
     * @example
     * print(solution.has(child))   # true
     * print(solution.has(unused))  # false
     */
    bool has(Ref<PrologVariable> const& p_variable) const;

    /**
     * @brief Returns the Dictionary of bindings keyed by PrologVariable.
     *
     * This is the supported way to write solution.bindings[child] from
     * GDScript (property name: get_bindings).
     *
     * @example
     * print(solution.get_bindings()[child])  # bob
     */
    Dictionary get_bindings() const { return m_bindings; }

    /**
     * @brief Returns the bound values only (no variable keys).
     *
     * @example
     * print(solution.values())  # ["bob"]
     */
    Array values() const;

protected:

    /**
     * @brief Binds put / get / has / get_bindings / values to GDScript.
     */
    static void _bind_methods();

private:

    Dictionary m_bindings;
};
