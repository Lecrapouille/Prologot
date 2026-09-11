/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#pragma once

#include "PrologVariable.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>

using namespace godot;

/**
 * @class PrologSolution
 * @brief One set of variable bindings from solve().
 *
 * Access values with get(variable) / has(variable). bindings is a Dictionary
 * keyed by PrologVariable objects.
 */
class PrologSolution: public RefCounted
{
    GDCLASS(PrologSolution, RefCounted)

public:

    PrologSolution() = default;
    ~PrologSolution() override = default;

    static Ref<PrologSolution> create();

    void put(Ref<PrologVariable> const& p_variable, Variant const& p_value);
    Variant get(Ref<PrologVariable> const& p_variable) const;
    bool has(Ref<PrologVariable> const& p_variable) const;
    Dictionary get_bindings() const { return m_bindings; }
    Array values() const;

protected:

    static void _bind_methods();

private:

    Dictionary m_bindings;
};
