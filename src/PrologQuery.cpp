/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologQuery.hpp"
#include <godot_cpp/core/class_db.hpp>

void PrologQuery::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("has_solution"), &PrologQuery::has_solution);
    ClassDB::bind_method(D_METHOD("first"), &PrologQuery::first);
    ClassDB::bind_method(D_METHOD("all"), &PrologQuery::all);
    ClassDB::bind_method(D_METHOD("_iter_init", "iter"),
                         &PrologQuery::_iter_init);
    ClassDB::bind_method(D_METHOD("_iter_next", "iter"),
                         &PrologQuery::_iter_next);
    ClassDB::bind_method(D_METHOD("_iter_get", "iter"), &PrologQuery::_iter_get);
}

Ref<PrologQuery> PrologQuery::create(Array const& p_solutions)
{
    Ref<PrologQuery> query;
    query.instantiate();
    query->m_solutions = p_solutions;
    return query;
}

bool PrologQuery::has_solution() const
{
    return !m_solutions.is_empty();
}

Variant PrologQuery::first() const
{
    if (m_solutions.is_empty())
        return Variant();
    return m_solutions[0];
}

Array PrologQuery::all() const
{
    return m_solutions;
}

bool PrologQuery::_iter_init(Variant const&)
{
    m_iter_index = 0;
    return m_iter_index < m_solutions.size();
}

bool PrologQuery::_iter_next(Variant const&)
{
    m_iter_index += 1;
    return m_iter_index < m_solutions.size();
}

Variant PrologQuery::_iter_get(Variant const&)
{
    if (m_iter_index < 0 || m_iter_index >= m_solutions.size())
        return Variant();
    return m_solutions[m_iter_index];
}
