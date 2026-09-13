/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologPredicate.hpp"
#include <godot_cpp/core/class_db.hpp>

void PrologPredicate::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_name"), &PrologPredicate::get_name);
    ClassDB::bind_method(D_METHOD("as_text"), &PrologPredicate::as_text);
    ClassDB::bind_method(D_METHOD("callv", "args"),
                         &PrologPredicate::make_goal);

    MethodInfo call_info;
    call_info.name = "call";
    call_info.return_val = PropertyInfo(Variant::OBJECT,
                                        "goal",
                                        PROPERTY_HINT_RESOURCE_TYPE,
                                        "PrologGoal");
    ClassDB::bind_vararg_method(METHOD_FLAGS_DEFAULT,
                                "call",
                                &PrologPredicate::make_goal_varargs,
                                call_info);
}

Ref<PrologPredicate> PrologPredicate::create(String const& p_name)
{
    Ref<PrologPredicate> predicate;
    predicate.instantiate();
    predicate->m_name = p_name;
    return predicate;
}

String PrologPredicate::as_text() const
{
    return m_name;
}

Ref<PrologGoal> PrologPredicate::make_goal(Array const& p_args) const
{
    return PrologGoal::from_compound(m_name, p_args);
}

Variant PrologPredicate::make_goal_varargs(Variant const** p_args,
                                           GDExtensionInt p_arg_count,
                                           GDExtensionCallError& r_error)
{
    r_error.error = GDEXTENSION_CALL_OK;
    Array args;
    for (GDExtensionInt i = 0; i < p_arg_count; i++)
        args.push_back(*p_args[i]);
    return make_goal(args);
}
