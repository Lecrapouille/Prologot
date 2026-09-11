/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologTerm.hpp"
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/variant.hpp>

void PrologTerm::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_kind"), &PrologTerm::get_kind);
    ClassDB::bind_method(D_METHOD("is_atom"), &PrologTerm::is_atom);
    ClassDB::bind_method(D_METHOD("is_integer"), &PrologTerm::is_integer);
    ClassDB::bind_method(D_METHOD("is_float"), &PrologTerm::is_float);
    ClassDB::bind_method(D_METHOD("is_string"), &PrologTerm::is_string);
    ClassDB::bind_method(D_METHOD("is_nil"), &PrologTerm::is_nil);
    ClassDB::bind_method(D_METHOD("is_list"), &PrologTerm::is_list);
    ClassDB::bind_method(D_METHOD("is_compound"), &PrologTerm::is_compound);
    ClassDB::bind_method(D_METHOD("is_variable"), &PrologTerm::is_variable);
    ClassDB::bind_method(D_METHOD("get_atom"), &PrologTerm::get_atom);
    ClassDB::bind_method(D_METHOD("get_integer"), &PrologTerm::get_integer);
    ClassDB::bind_method(D_METHOD("get_real"), &PrologTerm::get_real);
    ClassDB::bind_method(D_METHOD("get_string"), &PrologTerm::get_string_value);
    ClassDB::bind_method(D_METHOD("get_functor"), &PrologTerm::get_functor);
    ClassDB::bind_method(D_METHOD("get_args"), &PrologTerm::get_args);
    ClassDB::bind_method(D_METHOD("as_text"), &PrologTerm::as_text);
}

Ref<PrologTerm> PrologTerm::make_atom(String const& p_name)
{
    Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_ATOM;
    term->m_text = p_name;
    return term;
}

Ref<PrologTerm> PrologTerm::make_integer(int64_t p_value)
{
    Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_INTEGER;
    term->m_integer = p_value;
    return term;
}

Ref<PrologTerm> PrologTerm::make_real(double p_value)
{
    Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_FLOAT;
    term->m_real = p_value;
    return term;
}

Ref<PrologTerm> PrologTerm::make_string(String const& p_value)
{
    Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_STRING;
    term->m_text = p_value;
    return term;
}

Ref<PrologTerm> PrologTerm::make_nil()
{
    Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_NIL;
    return term;
}

Ref<PrologTerm> PrologTerm::make_list(Array const& p_items)
{
    if (p_items.is_empty())
        return make_nil();

    Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_LIST;
    term->m_args = p_items;
    return term;
}

Ref<PrologTerm> PrologTerm::make_compound(String const& p_functor,
                                          Array const& p_args)
{
    Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_COMPOUND;
    term->m_text = p_functor;
    term->m_args = p_args;
    return term;
}

String PrologTerm::get_kind() const
{
    switch (m_kind)
    {
        case KIND_ATOM:
            return "atom";
        case KIND_INTEGER:
            return "integer";
        case KIND_FLOAT:
            return "float";
        case KIND_STRING:
            return "string";
        case KIND_NIL:
            return "nil";
        case KIND_LIST:
            return "list";
        case KIND_COMPOUND:
            return "compound";
        case KIND_VARIABLE:
            return "variable";
    }
    return "unknown";
}

String PrologTerm::get_atom() const
{
    return is_atom() ? m_text : String();
}

String PrologTerm::get_string_value() const
{
    return is_string() ? m_text : String();
}

String PrologTerm::get_functor() const
{
    return is_compound() ? m_text : String();
}

static String variant_as_text(Variant const& p_value)
{
    if (p_value.get_type() == Variant::OBJECT)
    {
        Ref<PrologTerm> term = p_value;
        if (term.is_valid())
            return term->as_text();
    }
    return String(p_value);
}

String PrologTerm::as_text() const
{
    switch (m_kind)
    {
        case KIND_ATOM:
            return m_text;
        case KIND_INTEGER:
            return String::num_int64(m_integer);
        case KIND_FLOAT:
            return String::num(m_real);
        case KIND_STRING:
            return String("\"") + m_text + String("\"");
        case KIND_NIL:
            return "[]";
        case KIND_LIST:
        {
            String text = String("[");
            for (int i = 0; i < m_args.size(); i++)
            {
                if (i > 0)
                    text += String(", ");
                text += variant_as_text(m_args[i]);
            }
            text += String("]");
            return text;
        }
        case KIND_COMPOUND:
        {
            String text = m_text + String("(");
            for (int i = 0; i < m_args.size(); i++)
            {
                if (i > 0)
                    text += String(", ");
                text += variant_as_text(m_args[i]);
            }
            text += String(")");
            return text;
        }
        case KIND_VARIABLE:
            return m_text.is_empty() ? String("_") : m_text;
    }
    return String();
}
