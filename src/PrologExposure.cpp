/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * Explicit exposure of Godot properties and methods as Prolog predicates.
 * Nothing is auto-exported: the game author lists each member.
 */

#include "Prologot.hpp"
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/object.hpp>

using namespace godot;

namespace
{
bool is_valid_predicate_name(String const& p_name)
{
    if (p_name.is_empty())
        return false;

    CharString utf8 = p_name.utf8();
    char const* s = utf8.get_data();
    if (s == nullptr || s[0] < 'a' || s[0] > 'z')
        return false;

    for (int i = 1; s[i] != '\0'; ++i)
    {
        char c = s[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_';
        if (!ok)
            return false;
    }
    return true;
}

bool is_reserved_predicate(String const& p_name)
{
    return p_name == "name" || p_name == "is" || p_name == "true" ||
           p_name == "false" || p_name == "fail" || p_name == "cut" ||
           p_name == "prologot_property" || p_name == "prologot_method";
}

String quote_prolog_atom(String const& p_name)
{
    return String("'") + p_name.replace("'", "''") + String("'");
}

String default_property_predicate(String const& p_property)
{
    if (p_property == "name")
        return "node_name";
    return p_property;
}

bool term_as_string(term_t p_term, String& r_out)
{
    char* s = nullptr;
    if (!PL_get_chars(p_term, &s, CVT_ATOM | CVT_STRING | REP_UTF8))
        return false;
    r_out = String::utf8(s);
    return true;
}

Object* object_from_term(term_t p_term)
{
    Ref<PrologObject> handle = PrologObject::from_swi_term(p_term);
    if (handle.is_null() || !handle->is_valid())
        return nullptr;
    return handle->get_object();
}

bool object_matches_class(Object* p_object, String const& p_class)
{
    if (p_class.is_empty())
        return true;
    return p_object->is_class(p_class);
}

bool object_has_property(Object* p_object, String const& p_property)
{
    TypedArray<Dictionary> list = p_object->get_property_list();
    for (int i = 0; i < list.size(); ++i)
    {
        Dictionary d = list[i];
        if (String(d.get("name", "")) == p_property)
            return true;
    }
    return false;
}

Variant unwrap_godot_arg(Variant const& p_value)
{
    if (p_value.get_type() == Variant::OBJECT)
    {
        Ref<PrologObject> handle = p_value;
        if (handle.is_valid())
        {
            Object* obj = handle->get_object();
            return obj ? Variant(obj) : Variant();
        }
    }
    if (p_value.get_type() == Variant::ARRAY)
    {
        Array src = p_value;
        Array out;
        for (int i = 0; i < src.size(); ++i)
            out.push_back(unwrap_godot_arg(src[i]));
        return out;
    }
    return p_value;
}

bool class_has_named_property(String const& p_class, String const& p_property)
{
    ClassDBSingleton* cdb = ClassDBSingleton::get_singleton();
    if (cdb == nullptr || p_class.is_empty() || !cdb->class_exists(p_class))
        return false;

    TypedArray<Dictionary> props = cdb->class_get_property_list(p_class, false);
    for (int i = 0; i < props.size(); ++i)
    {
        Dictionary d = props[i];
        if (String(d.get("name", "")) == p_property)
            return true;
    }
    return false;
}

bool lookup_method(String const& p_class,
                   String const& p_method,
                   int& r_argc,
                   bool& r_has_return)
{
    r_argc = 0;
    r_has_return = true;

    ClassDBSingleton* cdb = ClassDBSingleton::get_singleton();
    if (cdb == nullptr || p_class.is_empty() || !cdb->class_exists(p_class))
        return p_class.is_empty();

    if (!cdb->class_has_method(p_class, p_method))
        return false;

    TypedArray<Dictionary> methods = cdb->class_get_method_list(p_class, false);
    for (int i = 0; i < methods.size(); ++i)
    {
        Dictionary d = methods[i];
        if (String(d.get("name", "")) != p_method)
            continue;

        Array args = d.get("args", Array());
        Array defaults = d.get("default_args", Array());
        r_argc = args.size() - defaults.size();
        if (r_argc < 0)
            r_argc = 0;

        Variant ret = d.get("return", Variant());
        if (ret.get_type() != Variant::DICTIONARY)
            ret = d.get("return_val", Variant());
        if (ret.get_type() == Variant::DICTIONARY)
        {
            Dictionary rd = ret;
            if (rd.has("type"))
            {
                int t = rd["type"];
                r_has_return = t != (int)Variant::NIL;
            }
        }
        else if (ret.get_type() == Variant::INT)
        {
            r_has_return = (int)ret != (int)Variant::NIL;
        }
        return true;
    }

    r_argc = cdb->class_get_method_argument_count(p_class, p_method);
    return true;
}

foreign_t pl_prologot_property(term_t p_class,
                               term_t p_property,
                               term_t p_object,
                               term_t p_value)
{
    return Prologot::foreign_property(p_class, p_property, p_object, p_value)
               ? TRUE
               : FALSE;
}

foreign_t pl_prologot_method(term_t p_class,
                             term_t p_method,
                             term_t p_object,
                             term_t p_args,
                             term_t p_result)
{
    return Prologot::foreign_method(
               p_class, p_method, p_object, p_args, p_result)
               ? TRUE
               : FALSE;
}
} // namespace

bool Prologot::register_foreign_predicates()
{
    if (!PL_register_foreign(
            "prologot_property",
            4,
            reinterpret_cast<pl_function_t>(pl_prologot_property),
            0))
        return false;
    if (!PL_register_foreign(
            "prologot_method",
            5,
            reinterpret_cast<pl_function_t>(pl_prologot_method),
            0))
        return false;
    return true;
}

bool Prologot::expose_property(String const& p_class,
                               String const& p_property,
                               String const& p_predicate)
{
    if (!m_initialized)
    {
        push_error("expose_property() requires an initialized engine");
        return false;
    }
    if (p_property.is_empty())
    {
        push_error("expose_property() needs a property name");
        return false;
    }

    String predicate = p_predicate.is_empty()
                           ? default_property_predicate(p_property)
                           : p_predicate;
    if (!is_valid_predicate_name(predicate))
    {
        push_error(String("Invalid Prolog predicate name: ") + predicate +
                   String(" (use a lowercase identifier, e.g. node_name)"));
        return false;
    }
    if (is_reserved_predicate(predicate))
    {
        push_error(String("Cannot expose as ") + predicate +
                   String("/2 (reserved or clashes with SWI-Prolog). "
                          "Pass a custom predicate name."));
        return false;
    }

    if (!p_class.is_empty())
    {
        ClassDBSingleton* cdb = ClassDBSingleton::get_singleton();
        if (cdb != nullptr && cdb->class_exists(p_class) &&
            !class_has_named_property(p_class, p_property))
        {
            push_error(String("Unknown property '") + p_property +
                       String("' on class ") + p_class);
            return false;
        }
    }

    int const arity = 2;
    unexpose(predicate, arity);

    String clause = predicate + String("(Obj, Val) :- prologot_property(") +
                    quote_prolog_atom(p_class) + String(", ") +
                    quote_prolog_atom(p_property) + String(", Obj, Val)");
    if (!add_fact(clause))
    {
        push_error(String("Failed to install property wrapper: ") + clause);
        return false;
    }

    ExposedBinding binding;
    binding.kind = "property";
    binding.class_name = p_class;
    binding.member = p_property;
    binding.predicate = predicate;
    binding.arity = arity;
    m_exposed.push_back(binding);
    return true;
}

bool Prologot::expose_method(String const& p_class,
                             String const& p_method,
                             String const& p_predicate)
{
    if (!m_initialized)
    {
        push_error("expose_method() requires an initialized engine");
        return false;
    }
    if (p_method.is_empty())
    {
        push_error("expose_method() needs a method name");
        return false;
    }

    String predicate = p_predicate.is_empty() ? p_method : p_predicate;
    if (!is_valid_predicate_name(predicate))
    {
        push_error(String("Invalid Prolog predicate name: ") + predicate);
        return false;
    }
    if (is_reserved_predicate(predicate))
    {
        push_error(String("Cannot expose as ") + predicate +
                   String(" (reserved or clashes with SWI-Prolog). "
                          "Pass a custom predicate name."));
        return false;
    }

    int argc = 0;
    bool has_return = true;
    if (!lookup_method(p_class, p_method, argc, has_return))
    {
        push_error(String("Unknown method '") + p_method +
                   String("' on class ") +
                   (p_class.is_empty() ? String("<any>") : p_class));
        return false;
    }

    int arity = 1 + argc + (has_return ? 1 : 0);
    unexpose(predicate, arity);

    String head = predicate + String("(Obj");
    String args_list = String("[");
    for (int i = 0; i < argc; ++i)
    {
        String var = String("A") + String::num_int64(i + 1);
        head += String(", ") + var;
        if (i > 0)
            args_list += String(", ");
        args_list += var;
    }
    args_list += String("]");
    if (has_return)
        head += String(", Result");
    head += String(")");

    String result_var = has_return ? String("Result") : String("_");
    String clause = head + String(" :- prologot_method(") +
                    quote_prolog_atom(p_class) + String(", ") +
                    quote_prolog_atom(p_method) + String(", Obj, ") +
                    args_list + String(", ") + result_var + String(")");
    if (!add_fact(clause))
    {
        push_error(String("Failed to install method wrapper: ") + clause);
        return false;
    }

    ExposedBinding binding;
    binding.kind = "method";
    binding.class_name = p_class;
    binding.member = p_method;
    binding.predicate = predicate;
    binding.arity = arity;
    m_exposed.push_back(binding);
    return true;
}

bool Prologot::unexpose(String const& p_predicate, int p_arity)
{
    if (!m_initialized)
        return false;
    if (p_predicate.is_empty() || p_arity < 0)
        return false;

    bool found = false;
    for (size_t i = 0; i < m_exposed.size();)
    {
        if (m_exposed[i].predicate == p_predicate &&
            m_exposed[i].arity == p_arity)
        {
            m_exposed.erase(m_exposed.begin() + static_cast<long>(i));
            found = true;
        }
        else
        {
            ++i;
        }
    }

    Array args;
    for (int i = 0; i < p_arity; ++i)
        args.push_back(PrologVariable::create_anonymous());
    retract_all(PrologGoal::from_compound(p_predicate, args));
    return found || predicate_exists(p_predicate, p_arity);
}

Array Prologot::list_exposed() const
{
    Array out;
    for (ExposedBinding const& binding : m_exposed)
    {
        Dictionary d;
        d["kind"] = binding.kind;
        d["class"] = binding.class_name;
        d["member"] = binding.member;
        d["predicate"] = binding.predicate;
        d["arity"] = binding.arity;
        out.push_back(d);
    }
    return out;
}

bool Prologot::foreign_property(term_t p_class,
                                term_t p_property,
                                term_t p_object,
                                term_t p_value)
{
    Prologot* self = get_singleton();
    if (self == nullptr || !self->m_initialized)
        return false;
    if (PL_exception(0))
        PL_clear_exception();

    String class_name;
    String property;
    if (!term_as_string(p_class, class_name) ||
        !term_as_string(p_property, property))
        return false;

    Object* obj = object_from_term(p_object);
    if (obj == nullptr || !object_matches_class(obj, class_name))
        return false;
    if (!object_has_property(obj, property))
        return false;

    term_t converted = self->variant_to_term(obj->get(property));
    if (!converted)
        return false;
    return PL_unify(p_value, converted) != FALSE;
}

bool Prologot::foreign_method(term_t p_class,
                              term_t p_method,
                              term_t p_object,
                              term_t p_args,
                              term_t p_result)
{
    Prologot* self = get_singleton();
    if (self == nullptr || !self->m_initialized)
        return false;

    String class_name;
    String method;
    if (!term_as_string(p_class, class_name) ||
        !term_as_string(p_method, method))
        return false;

    Object* obj = object_from_term(p_object);
    if (obj == nullptr || !object_matches_class(obj, class_name))
        return false;
    if (!obj->has_method(method))
        return false;

    Variant args_var = self->term_to_variant(p_args);
    if (args_var.get_type() != Variant::ARRAY)
        return false;
    Array args = unwrap_godot_arg(args_var);

    Variant result = obj->callv(method, args);
    term_t converted = self->variant_to_term(result);
    if (!converted)
        return false;
    return PL_unify(p_result, converted) != FALSE;
}
