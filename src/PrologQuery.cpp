/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologQuery.hpp"
#include "PrologConversion.hpp"
#include "Prologot.hpp"
#include <SWI-Prolog.h>
#include <algorithm>
#include <godot_cpp/core/class_db.hpp>
#include <map>
#include <vector>

namespace prologot
{

static std::vector<PrologQuery*> g_open_queries;

static void register_open_query(PrologQuery* p_query)
{
    if (p_query)
        g_open_queries.push_back(p_query);
}

static void unregister_open_query(PrologQuery* p_query)
{
    g_open_queries.erase(
        std::remove(g_open_queries.begin(), g_open_queries.end(), p_query),
        g_open_queries.end());
}

struct PrologQuery::State
{
    Prologot* engine = nullptr;
    qid_t qid = 0;
    fid_t frame = 0;
    std::map<int64_t, term_t> vars;
    std::vector<godot::Ref<PrologVariable>> order;
    bool open = false;
    bool exhausted = false;
};

PrologQuery::PrologQuery() = default;

PrologQuery::~PrologQuery()
{
    close_query(true);
}

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

godot::Ref<PrologQuery> PrologQuery::open(Prologot* p_engine,
                                          godot::Ref<PrologGoal> const& p_goal,
                                          int64_t p_max_solutions)
{
    godot::Ref<PrologQuery> query;
    query.instantiate();
    query->m_max_solutions = p_max_solutions < 0 ? 0 : p_max_solutions;

    if (!p_engine || !p_engine->is_initialized() || p_goal.is_null())
        return query;

    if (PL_exception(0))
        PL_clear_exception();

    query->m_state = std::make_unique<State>();
    State& state = *query->m_state;
    state.engine = p_engine;
    state.frame = PL_open_foreign_frame();

    term_t goal = PL_new_term_ref();
    godot::String error;
    if (!compile_goal(p_goal, goal, state.vars, state.order, &error))
    {
        if (!error.is_empty())
            p_engine->m_last_error = error;
        PL_discard_foreign_frame(state.frame);
        query->m_state.reset();
        return query;
    }

    state.qid = PL_open_query(
        NULL, PL_Q_CATCH_EXCEPTION, PL_predicate("call", 1, "system"), goal);
    state.open = true;
    register_open_query(query.ptr());
    return query;
}

void PrologQuery::abandon_all_open()
{
    std::vector<PrologQuery*> open = g_open_queries;
    g_open_queries.clear();
    for (PrologQuery* query : open)
    {
        if (query)
            query->close_query(true);
    }
}

void PrologQuery::close_query(bool p_cut_remaining)
{
    if (!m_state)
        return;

    bool const engine_up = PL_is_initialised(nullptr, nullptr) != FALSE;

    if (m_state->open)
    {
        unregister_open_query(this);
        if (engine_up)
        {
            int closed = FALSE;
            if (p_cut_remaining && !m_state->exhausted)
                closed = PL_cut_query(m_state->qid);
            if (!closed)
                PL_close_query(m_state->qid);
        }
        m_state->open = false;
        m_state->qid = 0;
    }

    if (m_state->frame)
    {
        if (engine_up)
            PL_discard_foreign_frame(m_state->frame);
        m_state->frame = 0;
    }

    m_state->exhausted = true;
}

bool PrologQuery::pull_one()
{
    if (!m_state || !m_state->open || m_state->exhausted)
        return false;

    if (m_max_solutions > 0 && m_solutions.size() >= m_max_solutions)
    {
        close_query(true);
        return false;
    }

    int result = PL_next_solution(m_state->qid);
    if (result == PL_S_EXCEPTION)
    {
        if (m_state->engine)
            m_state->engine->handle_prolog_exception(m_state->qid, "Solve goal");
        close_query(false);
        return false;
    }
    if (!result)
    {
        close_query(false);
        return false;
    }

    godot::Ref<PrologSolution> solution = PrologSolution::create();
    for (godot::Ref<PrologVariable> const& variable : m_state->order)
    {
        auto it = m_state->vars.find(variable->get_id());
        if (it != m_state->vars.end())
            solution->put(variable, term_to_variant(it->second));
    }
    m_solutions.push_back(solution);

    if (m_max_solutions > 0 && m_solutions.size() >= m_max_solutions)
        close_query(true);

    return true;
}

bool PrologQuery::ensure_count(int p_count)
{
    while (m_solutions.size() < p_count)
    {
        if (!pull_one())
            return false;
    }
    return true;
}

bool PrologQuery::has_solution()
{
    bool ok = ensure_count(1);
    // Temporaries like solve(g).has_solution() stay alive until the
    // GDScript function ends; cut so abolish/cleanup can proceed.
    close_query(true);
    return ok;
}

godot::Variant PrologQuery::first()
{
    if (!ensure_count(1))
        return godot::Variant();
    close_query(true);
    return m_solutions[0];
}

godot::Array PrologQuery::all()
{
    while (pull_one())
    {
    }
    return m_solutions;
}

bool PrologQuery::_iter_init(godot::Variant const&)
{
    m_iter_index = 0;
    return ensure_count(1);
}

bool PrologQuery::_iter_next(godot::Variant const&)
{
    m_iter_index += 1;
    return ensure_count(m_iter_index + 1);
}

godot::Variant PrologQuery::_iter_get(godot::Variant const&)
{
    if (m_iter_index < 0 || m_iter_index >= m_solutions.size())
        return godot::Variant();
    return m_solutions[m_iter_index];
}

} // namespace prologot
