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

namespace prologot
{

void PrologTerm::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_kind"), &PrologTerm::get_kind);
    godot::ClassDB::bind_method(godot::D_METHOD("is_atom"), &PrologTerm::is_atom);
    godot::ClassDB::bind_method(godot::D_METHOD("is_integer"), &PrologTerm::is_integer);
    godot::ClassDB::bind_method(godot::D_METHOD("is_float"), &PrologTerm::is_float);
    godot::ClassDB::bind_method(godot::D_METHOD("is_string"), &PrologTerm::is_string);
    godot::ClassDB::bind_method(godot::D_METHOD("is_nil"), &PrologTerm::is_nil);
    godot::ClassDB::bind_method(godot::D_METHOD("is_list"), &PrologTerm::is_list);
    godot::ClassDB::bind_method(godot::D_METHOD("is_compound"), &PrologTerm::is_compound);
    godot::ClassDB::bind_method(godot::D_METHOD("is_variable"), &PrologTerm::is_variable);
    godot::ClassDB::bind_method(godot::D_METHOD("get_atom"), &PrologTerm::get_atom);
    godot::ClassDB::bind_method(godot::D_METHOD("get_integer"), &PrologTerm::get_integer);
    godot::ClassDB::bind_method(godot::D_METHOD("get_real"), &PrologTerm::get_real);
    godot::ClassDB::bind_method(godot::D_METHOD("get_string"), &PrologTerm::get_string_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_functor"), &PrologTerm::get_functor);
    godot::ClassDB::bind_method(godot::D_METHOD("get_args"), &PrologTerm::get_args);
    godot::ClassDB::bind_method(godot::D_METHOD("as_text"), &PrologTerm::as_text);
}

godot::Ref<PrologTerm> PrologTerm::make_atom(godot::String const& p_name)
{
    godot::Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_ATOM;
    term->m_text = p_name;
    return term;
}

godot::Ref<PrologTerm> PrologTerm::make_integer(int64_t p_value)
{
    godot::Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_INTEGER;
    term->m_integer = p_value;
    return term;
}

godot::Ref<PrologTerm> PrologTerm::make_real(double p_value)
{
    godot::Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_FLOAT;
    term->m_real = p_value;
    return term;
}

godot::Ref<PrologTerm> PrologTerm::make_string(godot::String const& p_value)
{
    godot::Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_STRING;
    term->m_text = p_value;
    return term;
}

godot::Ref<PrologTerm> PrologTerm::make_nil()
{
    godot::Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_NIL;
    return term;
}

godot::Ref<PrologTerm> PrologTerm::make_list(godot::Array const& p_items)
{
    if (p_items.is_empty())
        return make_nil();

    godot::Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_LIST;
    term->m_args = p_items;
    return term;
}

godot::Ref<PrologTerm> PrologTerm::make_compound(godot::String const& p_functor,
                                          godot::Array const& p_args)
{
    godot::Ref<PrologTerm> term;
    term.instantiate();
    term->m_kind = KIND_COMPOUND;
    term->m_text = p_functor;
    term->m_args = p_args;
    return term;
}

godot::String PrologTerm::get_kind() const
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

godot::String PrologTerm::get_atom() const
{
    return is_atom() ? m_text : godot::String();
}

godot::String PrologTerm::get_string_value() const
{
    return is_string() ? m_text : godot::String();
}

godot::String PrologTerm::get_functor() const
{
    return is_compound() ? m_text : godot::String();
}

static godot::String variant_as_text(godot::Variant const& p_value)
{
    if (p_value.get_type() == godot::Variant::OBJECT)
    {
        godot::Ref<PrologTerm> term = p_value;
        if (term.is_valid())
            return term->as_text();
    }
    return godot::String(p_value);
}

godot::String PrologTerm::as_text() const
{
    switch (m_kind)
    {
        case KIND_ATOM:
            return m_text;
        case KIND_INTEGER:
            return godot::String::num_int64(m_integer);
        case KIND_FLOAT:
            return godot::String::num(m_real);
        case KIND_STRING:
            return godot::String("\"") + m_text + godot::String("\"");
        case KIND_NIL:
            return "[]";
        case KIND_LIST:
        {
            godot::String text = godot::String("[");
            for (int i = 0; i < m_args.size(); i++)
            {
                if (i > 0)
                    text += godot::String(", ");
                text += variant_as_text(m_args[i]);
            }
            text += godot::String("]");
            return text;
        }
        case KIND_COMPOUND:
        {
            godot::String text = m_text + godot::String("(");
            for (int i = 0; i < m_args.size(); i++)
            {
                if (i > 0)
                    text += godot::String(", ");
                text += variant_as_text(m_args[i]);
            }
            text += godot::String(")");
            return text;
        }
        case KIND_VARIABLE:
            return m_text.is_empty() ? godot::String("_") : m_text;
    }
    return godot::String();
}

} // namespace prologot
