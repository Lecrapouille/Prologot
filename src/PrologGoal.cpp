/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologGoal.hpp"
#include <godot_cpp/core/class_db.hpp>

void PrologGoal::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_functor"), &PrologGoal::get_functor);
    ClassDB::bind_method(D_METHOD("get_args"), &PrologGoal::get_args);
    ClassDB::bind_method(D_METHOD("get_arity"), &PrologGoal::get_arity);
    ClassDB::bind_method(D_METHOD("as_text"), &PrologGoal::as_text);
    ClassDB::bind_method(D_METHOD("to_term"), &PrologGoal::to_term);
}

Ref<PrologGoal> PrologGoal::from_compound(String const& p_functor,
                                          Array const& p_args)
{
    Ref<PrologGoal> goal;
    goal.instantiate();
    goal->m_functor = p_functor;
    goal->m_args = p_args;
    return goal;
}

static String arg_as_text(Variant const& p_value)
{
    if (p_value.get_type() == Variant::OBJECT)
    {
        Ref<PrologTerm> term = p_value;
        if (term.is_valid())
            return term->as_text();
        Ref<PrologGoal> goal = p_value;
        if (goal.is_valid())
            return goal->as_text();
    }
    return String(p_value);
}

String PrologGoal::as_text() const
{
    String text = m_functor + String("(");
    for (int i = 0; i < m_args.size(); i++)
    {
        if (i > 0)
            text += String(", ");
        text += arg_as_text(m_args[i]);
    }
    text += String(")");
    return text;
}

Ref<PrologTerm> PrologGoal::to_term() const
{
    return PrologTerm::make_compound(m_functor, m_args);
}
