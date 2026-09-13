/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file defines PrologVariable: a distinct object used as a logical
 * variable. A GDScript String is never a variable — only this class is.
 */

#pragma once

#include "PrologTerm.hpp"

/**
 * @class PrologVariable
 * @brief Distinct Prolog variable object (not a magic uppercase string).
 *
 * Identity is stable (get_id()) so the same object reused in a goal is the
 * same Prolog variable, and so a solution can be keyed by that object.
 * The optional name is for debugging only; it is not how Prologot decides
 * that something is a variable.
 *
 * A String passed to PrologPredicate.call() is always an atom. Only
 * prolog.variable() / prolog.anonymous() create variables.
 *
 * Anonymous variables (variable() without a name, or anonymous()) are
 * fresh each time they are compiled into a goal and do not appear in
 * solutions.
 *
 * @example
 * var parent = prolog.predicate("parent")
 * var child = prolog.variable("Child")
 * var other = prolog.variable()
 * print(child.get_id() != other.get_id())  # true: two distinct variables
 *
 * for solution in prolog.solve(parent.call("tom", child)):
 *     print(solution.get(child))  # bob, then liz
 */
class PrologVariable: public PrologTerm
{
    GDCLASS(PrologVariable, PrologTerm)

public:

    /**
     * @brief Constructs a variable and assigns a unique identity.
     *
     * Prefer prolog.variable() / prolog.anonymous() from GDScript.
     */
    PrologVariable();

    /**
     * @brief Destructs the variable. RefCounted releases it automatically.
     */
    ~PrologVariable() override = default;

    /**
     * @brief Creates a named or unnamed (but not anonymous) variable.
     *
     * An empty name still has a stable id; it is not the same as
     * create_anonymous(). Unnamed variables get a debug label like _3.
     *
     * @param p_name Optional debug name (e.g. "Child").
     * @return A new PrologVariable.
     *
     * @example
     * var child = prolog.variable("Child")
     * print(child.get_name())      # Child
     * print(child.is_anonymous())  # false
     */
    static Ref<PrologVariable> create(String const& p_name = String());

    /**
     * @brief Creates an anonymous variable (_).
     *
     * Each occurrence compiled into a goal is a fresh Prolog variable.
     * Anonymous variables are omitted from PrologSolution bindings.
     *
     * @example
     * var parent = prolog.predicate("parent")
     * # parent(tom, _) : succeed if tom has any child
     * prolog.solve(parent.call("tom", prolog.anonymous())).has_solution()
     */
    static Ref<PrologVariable> create_anonymous();

    /**
     * @brief Returns the stable identity used as a solution key.
     *
     * Two separately created variable() calls have different ids. Reusing
     * the same object in several goals shares the same Prolog variable.
     *
     * @example
     * var x = prolog.variable()
     * var y = prolog.variable()
     * print(x.get_id() != y.get_id())  # true
     */
    int64_t get_id() const { return m_id; }

    /**
     * @brief Returns the optional debug name (empty if unnamed or anonymous).
     *
     * The name is never used as a solution key. Two variable("X") objects
     * both report "X" but remain distinct.
     *
     * @return Debug name, or empty.
     *
     * @example
     * print(prolog.variable("Child").get_name())  # Child
     * print(prolog.variable().get_name())         # empty or _N
     * print(prolog.anonymous().get_name())        # empty
     */
    String get_name() const { return m_name; }

    /**
     * @brief Returns true if this was created with anonymous().
     *
     * @return true for prolog.anonymous(), false for variable(name).
     *
     * @example
     * print(prolog.anonymous().is_anonymous())     # true
     * print(prolog.variable("X").is_anonymous())   # false
     */
    bool is_anonymous() const { return m_anonymous; }

protected:

    /**
     * @brief Binds get_id / get_name / is_anonymous to GDScript.
     */
    static void _bind_methods();

private:

    static int64_t next_id();

    int64_t m_id = 0;
    String m_name;
    bool m_anonymous = false;
};
