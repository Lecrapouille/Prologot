/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#pragma once

#include "PrologTerm.hpp"

/**
 * @class PrologVariable
 * @brief Distinct Prolog variable object (not a magic uppercase string).
 *
 * Identity is stable so a variable can be used as a solution key.
 * The Prolog name is optional and only used for debugging.
 */
class PrologVariable: public PrologTerm
{
    GDCLASS(PrologVariable, PrologTerm)

public:

    PrologVariable();
    ~PrologVariable() override = default;

    static Ref<PrologVariable> create(String const& p_name = String());
    static Ref<PrologVariable> create_anonymous();

    int64_t get_id() const { return m_id; }
    String get_name() const { return m_name; }
    bool is_anonymous() const { return m_anonymous; }

protected:

    static void _bind_methods();

private:

    static int64_t next_id();

    int64_t m_id = 0;
    String m_name;
    bool m_anonymous = false;
};
