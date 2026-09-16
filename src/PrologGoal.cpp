/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologGoal.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace prologot
{

void PrologGoal::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_functor"), &PrologGoal::get_functor);
    godot::ClassDB::bind_method(godot::D_METHOD("get_args"), &PrologGoal::get_args);
    godot::ClassDB::bind_method(godot::D_METHOD("get_arity"), &PrologGoal::get_arity);
    godot::ClassDB::bind_method(godot::D_METHOD("as_text"), &PrologGoal::as_text);
    godot::ClassDB::bind_method(godot::D_METHOD("to_term"), &PrologGoal::to_term);
    godot::ClassDB::bind_method(godot::D_METHOD("conjunction", "other"),
                         &PrologGoal::conjunction);
    godot::ClassDB::bind_method(godot::D_METHOD("disjunction", "other"),
                         &PrologGoal::disjunction);
    godot::ClassDB::bind_method(godot::D_METHOD("negated"), &PrologGoal::negated);
    godot::ClassDB::bind_method(godot::D_METHOD("cut"), &PrologGoal::cut);
}

godot::Ref<PrologGoal> PrologGoal::from_compound(godot::String const& p_functor,
                                          godot::Array const& p_args)
{
    godot::Ref<PrologGoal> goal;
    goal.instantiate();
    goal->m_functor = p_functor;
    goal->m_args = p_args;
    return goal;
}

static godot::String arg_as_text(godot::Variant const& p_value)
{
    if (p_value.get_type() == godot::Variant::OBJECT)
    {
        godot::Ref<PrologTerm> term = p_value;
        if (term.is_valid())
            return term->as_text();
        godot::Ref<PrologGoal> goal = p_value;
        if (goal.is_valid())
            return goal->as_text();
    }
    return godot::String(p_value);
}

godot::String PrologGoal::as_text() const
{
    if (m_args.is_empty())
        return m_functor;

    godot::String text = m_functor + godot::String("(");
    for (int i = 0; i < m_args.size(); i++)
    {
        if (i > 0)
            text += godot::String(", ");
        text += arg_as_text(m_args[i]);
    }
    text += godot::String(")");
    return text;
}

godot::Ref<PrologTerm> PrologGoal::to_term() const
{
    return PrologTerm::make_compound(m_functor, m_args);
}

godot::Ref<PrologGoal> PrologGoal::conjunction(godot::Ref<PrologGoal> const& p_other) const
{
    if (p_other.is_null())
        return godot::Ref<PrologGoal>();
    godot::Array args;
    args.push_back(godot::Ref<PrologGoal>(const_cast<PrologGoal*>(this)));
    args.push_back(p_other);
    return from_compound(",", args);
}

godot::Ref<PrologGoal> PrologGoal::disjunction(godot::Ref<PrologGoal> const& p_other) const
{
    if (p_other.is_null())
        return godot::Ref<PrologGoal>();
    godot::Array args;
    args.push_back(godot::Ref<PrologGoal>(const_cast<PrologGoal*>(this)));
    args.push_back(p_other);
    return from_compound(";", args);
}

godot::Ref<PrologGoal> PrologGoal::negated() const
{
    godot::Array args;
    args.push_back(godot::Ref<PrologGoal>(const_cast<PrologGoal*>(this)));
    return from_compound("\\+", args);
}

godot::Ref<PrologGoal> PrologGoal::cut() const
{
    return conjunction(from_compound("!", godot::Array()));
}

} // namespace prologot
