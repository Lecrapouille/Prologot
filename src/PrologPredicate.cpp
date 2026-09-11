/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologPredicate.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

void PrologPredicate::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_name"), &PrologPredicate::get_name);
    ClassDB::bind_method(D_METHOD("get_arity"), &PrologPredicate::get_arity);
    ClassDB::bind_method(D_METHOD("as_text"), &PrologPredicate::as_text);
    ClassDB::bind_method(D_METHOD("bindv", "args"), &PrologPredicate::bind);

    MethodInfo bind_info;
    bind_info.name = "bind";
    bind_info.return_val = PropertyInfo(Variant::OBJECT, "goal", PROPERTY_HINT_RESOURCE_TYPE, "PrologGoal");
    ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT,
                                "bind",
                                &PrologPredicate::bind_varargs,
                                bind_info);
}

Ref<PrologPredicate> PrologPredicate::create(String const& p_name, int p_arity)
{
    Ref<PrologPredicate> predicate;
    predicate.instantiate();
    predicate->m_name = p_name;
    predicate->m_arity = p_arity;
    return predicate;
}

String PrologPredicate::as_text() const
{
    return m_name + String("/") + String::num_int64(m_arity);
}

Ref<PrologGoal> PrologPredicate::bind(Array const& p_args) const
{
    if (p_args.size() != m_arity)
    {
        UtilityFunctions::push_error(
            String("PrologPredicate.bind: expected ") +
            String::num_int64(m_arity) + String(" argument(s) for ") +
            as_text() + String(", got ") + String::num_int64(p_args.size()));
        return Ref<PrologGoal>();
    }
    return PrologGoal::from_compound(m_name, p_args);
}

Variant PrologPredicate::bind_varargs(Variant const** p_args,
                                      GDExtensionInt p_arg_count,
                                      GDExtensionCallError& r_error)
{
    r_error.error = GDEXTENSION_CALL_OK;
    Array args;
    for (GDExtensionInt i = 0; i < p_arg_count; i++)
        args.push_back(*p_args[i]);
    return bind(args);
}
