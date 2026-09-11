/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

/**
 * @class PrologTerm
 * @brief Godot representation of a Prolog term (atom, number, string, list,
 * compound).
 *
 * Users never touch SWI-Prolog term_t. Variables are a subclass
 * (PrologVariable).
 */
class PrologTerm: public RefCounted
{
    GDCLASS(PrologTerm, RefCounted)

public:

    enum Kind
    {
        KIND_ATOM = 0,
        KIND_INTEGER,
        KIND_FLOAT,
        KIND_STRING,
        KIND_NIL,
        KIND_LIST,
        KIND_COMPOUND,
        KIND_VARIABLE
    };

    PrologTerm() = default;
    ~PrologTerm() override = default;

    static Ref<PrologTerm> make_atom(String const& p_name);
    static Ref<PrologTerm> make_integer(int64_t p_value);
    static Ref<PrologTerm> make_real(double p_value);
    static Ref<PrologTerm> make_string(String const& p_value);
    static Ref<PrologTerm> make_nil();
    static Ref<PrologTerm> make_list(Array const& p_items);
    static Ref<PrologTerm> make_compound(String const& p_functor,
                                         Array const& p_args);

    String get_kind() const;
    Kind get_kind_enum() const { return m_kind; }

    bool is_atom() const { return m_kind == KIND_ATOM; }
    bool is_integer() const { return m_kind == KIND_INTEGER; }
    bool is_float() const { return m_kind == KIND_FLOAT; }
    bool is_string() const { return m_kind == KIND_STRING; }
    bool is_nil() const { return m_kind == KIND_NIL; }
    bool is_list() const { return m_kind == KIND_LIST; }
    bool is_compound() const { return m_kind == KIND_COMPOUND; }
    bool is_variable() const { return m_kind == KIND_VARIABLE; }

    String get_atom() const;
    int64_t get_integer() const { return m_integer; }
    double get_real() const { return m_real; }
    String get_string_value() const;
    String get_functor() const;
    Array get_args() const { return m_args; }

    String as_text() const;

protected:

    static void _bind_methods();

    void set_kind(Kind p_kind) { m_kind = p_kind; }
    void set_text(String const& p_text) { m_text = p_text; }
    void set_integer(int64_t p_value) { m_integer = p_value; }
    void set_real(double p_value) { m_real = p_value; }
    void set_args(Array const& p_args) { m_args = p_args; }

private:

    Kind m_kind = KIND_NIL;
    String m_text;
    int64_t m_integer = 0;
    double m_real = 0.0;
    Array m_args;
};
