/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
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
 */
class PrologGoal: public RefCounted
{
    GDCLASS(PrologGoal, RefCounted)

public:

    PrologGoal() = default;
    ~PrologGoal() override = default;

    static Ref<PrologGoal> from_compound(String const& p_functor,
                                         Array const& p_args);

    String get_functor() const { return m_functor; }
    Array get_args() const { return m_args; }
    int get_arity() const { return m_args.size(); }
    String as_text() const;
    Ref<PrologTerm> to_term() const;

    Ref<PrologGoal> conjunction(Ref<PrologGoal> const& p_other) const;
    Ref<PrologGoal> disjunction(Ref<PrologGoal> const& p_other) const;
    Ref<PrologGoal> negated() const;

protected:

    static void _bind_methods();

private:

    String m_functor;
    Array m_args;
};
