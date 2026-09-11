/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologSolution.hpp"
#include <godot_cpp/core/class_db.hpp>

void PrologSolution::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("put", "variable", "value"),
                         &PrologSolution::put);
    ClassDB::bind_method(D_METHOD("get", "variable"), &PrologSolution::get);
    ClassDB::bind_method(D_METHOD("has", "variable"), &PrologSolution::has);
    ClassDB::bind_method(D_METHOD("get_bindings"),
                         &PrologSolution::get_bindings);
    ClassDB::bind_method(D_METHOD("values"), &PrologSolution::values);
}

Ref<PrologSolution> PrologSolution::create()
{
    Ref<PrologSolution> solution;
    solution.instantiate();
    return solution;
}

void PrologSolution::put(Ref<PrologVariable> const& p_variable,
                         Variant const& p_value)
{
    if (p_variable.is_null())
        return;
    m_bindings[p_variable] = p_value;
}

Variant PrologSolution::get(Ref<PrologVariable> const& p_variable) const
{
    if (p_variable.is_null() || !m_bindings.has(p_variable))
        return Variant();
    return m_bindings[p_variable];
}

bool PrologSolution::has(Ref<PrologVariable> const& p_variable) const
{
    if (p_variable.is_null())
        return false;
    return m_bindings.has(p_variable);
}

Array PrologSolution::values() const
{
    return m_bindings.values();
}
