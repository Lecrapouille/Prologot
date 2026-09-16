/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologSolution.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace prologot
{

void PrologSolution::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("put", "variable", "value"),
                         &PrologSolution::put);
    godot::ClassDB::bind_method(godot::D_METHOD("get", "variable"), &PrologSolution::get);
    godot::ClassDB::bind_method(godot::D_METHOD("has", "variable"), &PrologSolution::has);
    godot::ClassDB::bind_method(godot::D_METHOD("get_bindings"),
                         &PrologSolution::get_bindings);
    godot::ClassDB::bind_method(godot::D_METHOD("values"), &PrologSolution::values);
}

godot::Ref<PrologSolution> PrologSolution::create()
{
    godot::Ref<PrologSolution> solution;
    solution.instantiate();
    return solution;
}

void PrologSolution::put(godot::Ref<PrologVariable> const& p_variable,
                         godot::Variant const& p_value)
{
    if (p_variable.is_null())
        return;
    m_bindings[p_variable] = p_value;
}

godot::Variant PrologSolution::get(godot::Ref<PrologVariable> const& p_variable) const
{
    if (p_variable.is_null() || !m_bindings.has(p_variable))
        return godot::Variant();
    return m_bindings[p_variable];
}

bool PrologSolution::has(godot::Ref<PrologVariable> const& p_variable) const
{
    if (p_variable.is_null())
        return false;
    return m_bindings.has(p_variable);
}

godot::Array PrologSolution::values() const
{
    return m_bindings.values();
}

} // namespace prologot
