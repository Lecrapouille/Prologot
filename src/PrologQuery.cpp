/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologQuery.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace prologot
{

void PrologQuery::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("has_solution"), &PrologQuery::has_solution);
    godot::ClassDB::bind_method(godot::D_METHOD("first"), &PrologQuery::first);
    godot::ClassDB::bind_method(godot::D_METHOD("all"), &PrologQuery::all);
    godot::ClassDB::bind_method(godot::D_METHOD("_iter_init", "iter"),
                         &PrologQuery::_iter_init);
    godot::ClassDB::bind_method(godot::D_METHOD("_iter_next", "iter"),
                         &PrologQuery::_iter_next);
    godot::ClassDB::bind_method(godot::D_METHOD("_iter_get", "iter"), &PrologQuery::_iter_get);
}

godot::Ref<PrologQuery> PrologQuery::create(godot::Array const& p_solutions)
{
    godot::Ref<PrologQuery> query;
    query.instantiate();
    query->m_solutions = p_solutions;
    return query;
}

bool PrologQuery::has_solution() const
{
    return !m_solutions.is_empty();
}

godot::Variant PrologQuery::first() const
{
    if (m_solutions.is_empty())
        return godot::Variant();
    return m_solutions[0];
}

godot::Array PrologQuery::all() const
{
    return m_solutions;
}

bool PrologQuery::_iter_init(godot::Variant const&)
{
    m_iter_index = 0;
    return m_iter_index < m_solutions.size();
}

bool PrologQuery::_iter_next(godot::Variant const&)
{
    m_iter_index += 1;
    return m_iter_index < m_solutions.size();
}

godot::Variant PrologQuery::_iter_get(godot::Variant const&)
{
    if (m_iter_index < 0 || m_iter_index >= m_solutions.size())
        return godot::Variant();
    return m_solutions[m_iter_index];
}

} // namespace prologot
