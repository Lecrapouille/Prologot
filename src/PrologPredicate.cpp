/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologPredicate.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace prologot
{

void PrologPredicate::_bind_methods()
{
    godot::ClassDB::bind_method(godot::D_METHOD("get_name"), &PrologPredicate::get_name);
    godot::ClassDB::bind_method(godot::D_METHOD("as_text"), &PrologPredicate::as_text);
    godot::ClassDB::bind_method(godot::D_METHOD("callv", "args"),
                         &PrologPredicate::make_goal);

    godot::MethodInfo call_info;
    call_info.name = "call";
    call_info.return_val = godot::PropertyInfo(godot::Variant::OBJECT,
                                        "goal",
                                        godot::PROPERTY_HINT_RESOURCE_TYPE,
                                        "PrologGoal");
    godot::ClassDB::bind_vararg_method(godot::METHOD_FLAGS_DEFAULT,
                                "call",
                                &PrologPredicate::make_goal_varargs,
                                call_info);
}

godot::Ref<PrologPredicate> PrologPredicate::create(godot::String const& p_name)
{
    godot::Ref<PrologPredicate> predicate;
    predicate.instantiate();
    predicate->m_name = p_name;
    return predicate;
}

godot::String PrologPredicate::as_text() const
{
    return m_name;
}

godot::Ref<PrologGoal> PrologPredicate::make_goal(godot::Array const& p_args) const
{
    return PrologGoal::from_compound(m_name, p_args);
}

godot::Variant PrologPredicate::make_goal_varargs(godot::Variant const** p_args,
                                           GDExtensionInt p_arg_count,
                                           GDExtensionCallError& r_error)
{
    r_error.error = GDEXTENSION_CALL_OK;
    godot::Array args;
    for (GDExtensionInt i = 0; i < p_arg_count; i++)
        args.push_back(*p_args[i]);
    return make_goal(args);
}

} // namespace prologot
