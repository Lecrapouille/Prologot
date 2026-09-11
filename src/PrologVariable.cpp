/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologVariable.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <atomic>

namespace
{
std::atomic<int64_t> g_next_variable_id{1};
}

void PrologVariable::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_id"), &PrologVariable::get_id);
    ClassDB::bind_method(D_METHOD("get_name"), &PrologVariable::get_name);
    ClassDB::bind_method(D_METHOD("is_anonymous"),
                         &PrologVariable::is_anonymous);
}

int64_t PrologVariable::next_id()
{
    return g_next_variable_id.fetch_add(1);
}

PrologVariable::PrologVariable()
{
    set_kind(KIND_VARIABLE);
    m_id = next_id();
}

Ref<PrologVariable> PrologVariable::create(String const& p_name)
{
    Ref<PrologVariable> variable;
    variable.instantiate();
    variable->m_name = p_name;
    variable->m_anonymous = false;
    variable->set_text(p_name.is_empty() ? String("_") + String::num_int64(variable->m_id)
                                         : p_name);
    return variable;
}

Ref<PrologVariable> PrologVariable::create_anonymous()
{
    Ref<PrologVariable> variable;
    variable.instantiate();
    variable->m_anonymous = true;
    variable->set_text("_");
    return variable;
}
