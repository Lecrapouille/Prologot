/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#pragma once

#include "PrologGoal.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

/**
 * @class PrologPredicate
 * @brief Reusable Prolog predicate (name/arity) that builds goals via bind().
 */
class PrologPredicate: public RefCounted
{
    GDCLASS(PrologPredicate, RefCounted)

public:

    PrologPredicate() = default;
    ~PrologPredicate() override = default;

    static Ref<PrologPredicate> create(String const& p_name, int p_arity);

    String get_name() const { return m_name; }
    int get_arity() const { return m_arity; }
    String as_text() const;

    Ref<PrologGoal> bind(Array const& p_args) const;
    Variant bind_varargs(Variant const** p_args,
                         GDExtensionInt p_arg_count,
                         GDExtensionCallError& r_error);

protected:

    static void _bind_methods();

private:

    String m_name;
    int m_arity = 0;
};
