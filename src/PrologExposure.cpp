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
#include "PrologConversion.hpp"
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/object.hpp>

namespace prologot
{

// Lowercase Prolog identifier: [a-z][A-Za-z0-9_]*.
static bool is_valid_predicate_name(godot::String const& p_name)
{
    if (p_name.is_empty())
        return false;

    godot::CharString utf8 = p_name.utf8();
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

// Functors that would shadow SWI syntax or our foreign wrappers.
static bool is_reserved_predicate(godot::String const& p_name)
{
    return p_name == "name" || p_name == "is" || p_name == "true" ||
           p_name == "false" || p_name == "fail" || p_name == "cut" ||
           p_name == "prologot_property" || p_name == "prologot_method";
}

// Quote an atom for a generated clause (class / property / method names).
static godot::String quote_prolog_atom(godot::String const& p_name)
{
    return godot::String("'") + p_name.replace("'", "''") + godot::String("'");
}

// `name/2` is reserved in Prolog; expose Node.name as node_name/2 by default.
static godot::String default_property_predicate(godot::String const& p_property)
{
    if (p_property == "name")
        return "node_name";
    return p_property;
}

// Atom or Prolog string → Godot String (foreign predicate arguments).
static bool term_as_string(term_t p_term, godot::String& r_out)
{
    char* s = nullptr;
    if (!PL_get_chars(p_term, &s, CVT_ATOM | CVT_STRING | REP_UTF8))
        return false;
    r_out = godot::String::utf8(s);
    return true;
}

// Live Godot Object from a godot_object blob, or nullptr if freed / not a blob.
static godot::Object* object_from_term(term_t p_term)
{
    godot::Ref<PrologObject> handle = PrologObject::from_swi_term(p_term);
    if (handle.is_null() || !handle->is_valid())
        return nullptr;
    return handle->get_object();
}

// Empty class filter means any Object; otherwise Object.is_class().
static bool object_matches_class(godot::Object* p_object, godot::String const& p_class)
{
    if (p_class.is_empty())
        return true;
    return p_object->is_class(p_class);
}

// PrologObject blobs and nested arrays back to native Godot values for callv().
static godot::Variant unwrap_godot_arg(godot::Variant const& p_value)
{
    if (p_value.get_type() == godot::Variant::OBJECT)
    {
        godot::Ref<PrologObject> handle = p_value;
        if (handle.is_valid())
        {
            godot::Object* obj = handle->get_object();
            return obj ? godot::Variant(obj) : godot::Variant();
        }
    }
    if (p_value.get_type() == godot::Variant::ARRAY)
    {
        godot::Array src = p_value;
        godot::Array out;
        for (int i = 0; i < src.size(); ++i)
            out.push_back(unwrap_godot_arg(src[i]));
        return out;
    }
    return p_value;
}

// ClassDB lookup once at expose_property. Not Object.get_property_list()
// on each foreign call (that allocates every engine property of a Node).
static bool class_has_named_property(godot::String const& p_class, godot::String const& p_property)
{
    godot::ClassDBSingleton* cdb = godot::ClassDBSingleton::get_singleton();
    if (cdb == nullptr || p_class.is_empty() || !cdb->class_exists(p_class))
        return false;

    godot::TypedArray<godot::Dictionary> props =
        cdb->class_get_property_list(p_class, false);
    for (int i = 0; i < props.size(); ++i)
    {
        godot::Dictionary d = props[i];
        if (godot::String(d.get("name", "")) == p_property)
            return true;
    }
    return false;
}

// Resolve arity / has-return from ClassDB (once at expose_method).
static bool lookup_method(godot::String const& p_class,
                   godot::String const& p_method,
                   int& r_argc,
                   bool& r_has_return)
{
    r_argc = 0;
    r_has_return = true;

    godot::ClassDBSingleton* cdb = godot::ClassDBSingleton::get_singleton();
    if (cdb == nullptr || p_class.is_empty() || !cdb->class_exists(p_class))
        return p_class.is_empty();

    if (!cdb->class_has_method(p_class, p_method))
        return false;

    godot::TypedArray<godot::Dictionary> methods = cdb->class_get_method_list(p_class, false);
    for (int i = 0; i < methods.size(); ++i)
    {
        godot::Dictionary d = methods[i];
        if (godot::String(d.get("name", "")) != p_method)
            continue;

        godot::Array args = d.get("args", godot::Array());
        godot::Array defaults = d.get("default_args", godot::Array());
        r_argc = args.size() - defaults.size();
        if (r_argc < 0)
            r_argc = 0;

        godot::Variant ret = d.get("return", godot::Variant());
        if (ret.get_type() != godot::Variant::DICTIONARY)
            ret = d.get("return_val", godot::Variant());
        if (ret.get_type() == godot::Variant::DICTIONARY)
        {
            godot::Dictionary rd = ret;
            if (rd.has("type"))
            {
                int t = rd["type"];
                r_has_return = t != (int)godot::Variant::NIL;
            }
        }
        else if (ret.get_type() == godot::Variant::INT)
        {
            r_has_return = (int)ret != (int)godot::Variant::NIL;
        }
        return true;
    }

    r_argc = cdb->class_get_method_argument_count(p_class, p_method);
    return true;
}

// SWI foreign: prologot_property(Class, Property, Object, Value).
static foreign_t pl_prologot_property(term_t p_class,
                               term_t p_property,
                               term_t p_object,
                               term_t p_value)
{
    return Prologot::foreign_property(p_class, p_property, p_object, p_value)
               ? TRUE
               : FALSE;
}

// SWI foreign: prologot_method(Class, Method, Object, Args, Result).
static foreign_t pl_prologot_method(term_t p_class,
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

// Register the two foreign predicates used by generated expose_* wrappers.
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

bool Prologot::expose_property(godot::String const& p_class,
                               godot::String const& p_property,
                               godot::String const& p_predicate)
{
    if (!require_main_thread("expose_property()"))
        return false;
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

    godot::String predicate = p_predicate.is_empty()
                           ? default_property_predicate(p_property)
                           : p_predicate;
    if (!is_valid_predicate_name(predicate))
    {
        push_error(godot::String("Invalid Prolog predicate name: ") + predicate +
                   godot::String(" (use a lowercase identifier, e.g. node_name)"));
        return false;
    }
    if (is_reserved_predicate(predicate))
    {
        push_error(godot::String("Cannot expose as ") + predicate +
                   godot::String("/2 (reserved or clashes with SWI-Prolog). "
                          "Pass a custom predicate name."));
        return false;
    }

    if (!p_class.is_empty())
    {
        godot::ClassDBSingleton* cdb = godot::ClassDBSingleton::get_singleton();
        if (cdb != nullptr && cdb->class_exists(p_class) &&
            !class_has_named_property(p_class, p_property))
        {
            push_error(godot::String("Unknown property '") + p_property +
                       godot::String("' on class ") + p_class);
            return false;
        }
    }

    int const arity = 2;
    unexpose(predicate, arity);

    godot::String clause = predicate + godot::String("(Obj, Val) :- prologot_property(") +
                    quote_prolog_atom(p_class) + godot::String(", ") +
                    quote_prolog_atom(p_property) + godot::String(", Obj, Val)");
    if (!add_fact(clause))
    {
        push_error(godot::String("Failed to install property wrapper: ") + clause);
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

bool Prologot::expose_method(godot::String const& p_class,
                             godot::String const& p_method,
                             godot::String const& p_predicate)
{
    if (!require_main_thread("expose_method()"))
        return false;
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

    godot::String predicate = p_predicate.is_empty() ? p_method : p_predicate;
    if (!is_valid_predicate_name(predicate))
    {
        push_error(godot::String("Invalid Prolog predicate name: ") + predicate);
        return false;
    }
    if (is_reserved_predicate(predicate))
    {
        push_error(godot::String("Cannot expose as ") + predicate +
                   godot::String(" (reserved or clashes with SWI-Prolog). "
                          "Pass a custom predicate name."));
        return false;
    }

    int argc = 0;
    bool has_return = true;
    if (!lookup_method(p_class, p_method, argc, has_return))
    {
        push_error(godot::String("Unknown method '") + p_method +
                   godot::String("' on class ") +
                   (p_class.is_empty() ? godot::String("<any>") : p_class));
        return false;
    }

    int arity = 1 + argc + (has_return ? 1 : 0);
    unexpose(predicate, arity);

    godot::String head = predicate + godot::String("(Obj");
    godot::String args_list = godot::String("[");
    for (int i = 0; i < argc; ++i)
    {
        godot::String var = godot::String("A") + godot::String::num_int64(i + 1);
        head += godot::String(", ") + var;
        if (i > 0)
            args_list += godot::String(", ");
        args_list += var;
    }
    args_list += godot::String("]");
    if (has_return)
        head += godot::String(", Result");
    head += godot::String(")");

    godot::String result_var = has_return ? godot::String("Result") : godot::String("_");
    godot::String clause = head + godot::String(" :- prologot_method(") +
                    quote_prolog_atom(p_class) + godot::String(", ") +
                    quote_prolog_atom(p_method) + godot::String(", Obj, ") +
                    args_list + godot::String(", ") + result_var + godot::String(")");
    if (!add_fact(clause))
    {
        push_error(godot::String("Failed to install method wrapper: ") + clause);
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

bool Prologot::unexpose(godot::String const& p_predicate, int p_arity)
{
    if (!require_main_thread("unexpose()"))
        return false;
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

    godot::Array args;
    for (int i = 0; i < p_arity; ++i)
        args.push_back(PrologVariable::create_anonymous());
    retract_all(PrologGoal::from_compound(p_predicate, args));
    return found || predicate_exists(p_predicate, p_arity);
}

godot::Array Prologot::list_exposed() const
{
    godot::Array out;
    for (ExposedBinding const& binding : m_exposed)
    {
        godot::Dictionary d;
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

    godot::String class_name;
    godot::String property;
    if (!term_as_string(p_class, class_name) ||
        !term_as_string(p_property, property))
        return false;

    godot::Object* obj = object_from_term(p_object);
    if (obj == nullptr || !object_matches_class(obj, class_name))
        return false;
    // Property existence was checked at expose_property() via ClassDB.
    // Do not call get_property_list() here (allocates the full Node list).

    term_t converted = variant_to_term(obj->get(property));
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

    godot::String class_name;
    godot::String method;
    if (!term_as_string(p_class, class_name) ||
        !term_as_string(p_method, method))
        return false;

    godot::Object* obj = object_from_term(p_object);
    if (obj == nullptr || !object_matches_class(obj, class_name))
        return false;
    if (!obj->has_method(method))
        return false;

    godot::Variant args_var = term_to_variant(p_args);
    if (args_var.get_type() != godot::Variant::ARRAY)
        return false;
    godot::Array args = unwrap_godot_arg(args_var);

    godot::Variant result = obj->callv(method, args);
    term_t converted = variant_to_term(result);
    if (!converted)
        return false;
    return PL_unify(p_result, converted) != FALSE;
}

} // namespace prologot
