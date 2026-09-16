/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * Variant ↔ term_t conversion. Stateless: no Prologot instance required.
 *
 * Text policy (see PrologConversion.hpp and doc/glossary.md):
 *   String → atom on the way in; atom → String on the way out.
 *   A Prolog string stays a PrologTerm so hello and "hello" stay distinct
 *   after get(). Do not wrap atoms in PrologTerm::make_atom() here.
 */

#include "PrologConversion.hpp"
#include "PrologObject.hpp"
#include <cstring>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector3i.hpp>

namespace prologot
{

term_t object_arg_to_term(godot::Variant const& p_arg,
                          std::map<int64_t, term_t>& p_vars,
                          std::vector<godot::Ref<PrologVariable>>& p_order)
{
    if (p_arg.get_type() == godot::Variant::OBJECT)
    {
        godot::Ref<PrologVariable> variable = p_arg;
        if (variable.is_valid())
        {
            if (variable->is_anonymous())
            {
                term_t fresh = PL_new_term_ref();
                if (!PL_put_variable(fresh))
                    return (term_t)0;
                return fresh;
            }
            auto it = p_vars.find(variable->get_id());
            if (it != p_vars.end())
                return it->second;
            term_t var = PL_new_term_ref();
            if (!PL_put_variable(var))
                return (term_t)0;
            p_vars[variable->get_id()] = var;
            p_order.push_back(variable);
            return var;
        }

        godot::Ref<PrologGoal> nested = p_arg;
        if (nested.is_valid())
        {
            term_t goal = PL_new_term_ref();
            if (!compile_goal(nested, goal, p_vars, p_order))
                return (term_t)0;
            return goal;
        }

        godot::Ref<PrologTerm> term = p_arg;
        if (term.is_valid())
            return prolog_term_to_swi(term, p_vars, p_order);

        godot::Ref<PrologObject> handle = p_arg;
        if (handle.is_valid())
            return handle->to_swi_term();

        godot::Object* native = p_arg;
        if (native)
            return godot_object_to_term(native);
    }

    if (p_arg.get_type() == godot::Variant::ARRAY)
    {
        godot::Array arr = p_arg;
        term_t list = PL_new_term_ref();
        if (!PL_put_nil(list))
            return (term_t)0;
        for (int i = arr.size() - 1; i >= 0; i--)
        {
            term_t elem = object_arg_to_term(arr[i], p_vars, p_order);
            if (!elem)
                return (term_t)0;
            term_t new_list = PL_new_term_ref();
            if (!PL_cons_list(new_list, elem, list))
                return (term_t)0;
            list = new_list;
        }
        return list;
    }

    return variant_to_term(p_arg);
}

term_t prolog_term_to_swi(godot::Ref<PrologTerm> const& p_term,
                          std::map<int64_t, term_t>& p_vars,
                          std::vector<godot::Ref<PrologVariable>>& p_order)
{
    if (p_term.is_null())
        return (term_t)0;

    godot::Ref<PrologVariable> variable = p_term;
    if (variable.is_valid())
        return object_arg_to_term(variable, p_vars, p_order);

    switch (p_term->get_kind_enum())
    {
        case PrologTerm::KIND_ATOM:
            return variant_to_term(p_term->get_atom());
        case PrologTerm::KIND_INTEGER:
            return variant_to_term(p_term->get_integer());
        case PrologTerm::KIND_FLOAT:
            return variant_to_term(p_term->get_real());
        case PrologTerm::KIND_STRING:
        {
            term_t t = PL_new_term_ref();
            if (!PL_put_string_chars(
                    t, p_term->get_string_value().utf8().get_data()))
                return (term_t)0;
            return t;
        }
        case PrologTerm::KIND_NIL:
        {
            term_t t = PL_new_term_ref();
            if (!PL_put_nil(t))
                return (term_t)0;
            return t;
        }
        case PrologTerm::KIND_LIST:
            return object_arg_to_term(p_term->get_args(), p_vars, p_order);
        case PrologTerm::KIND_COMPOUND:
        {
            godot::Ref<PrologGoal> goal = PrologGoal::from_compound(
                p_term->get_functor(), p_term->get_args());
            term_t t = PL_new_term_ref();
            if (!compile_goal(goal, t, p_vars, p_order))
                return (term_t)0;
            return t;
        }
        case PrologTerm::KIND_VARIABLE:
            return (term_t)0;
    }
    return (term_t)0;
}

bool compile_goal(godot::Ref<PrologGoal> const& p_goal,
                  term_t p_out_goal,
                  std::map<int64_t, term_t>& p_vars,
                  std::vector<godot::Ref<PrologVariable>>& p_order,
                  godot::String* r_error)
{
    if (p_goal.is_null())
        return false;

    godot::Array args = p_goal->get_args();
    int arity = args.size();
    term_t arg_refs = arity > 0 ? PL_new_term_refs(arity) : (term_t)0;
    for (int i = 0; i < arity; i++)
    {
        term_t arg = object_arg_to_term(args[i], p_vars, p_order);
        if (!arg || !PL_put_term(arg_refs + i, arg))
        {
            if (r_error)
            {
                *r_error =
                    "Failed to convert goal argument " + godot::String::num_int64(i);
            }
            return false;
        }
    }

    functor_t f = PL_new_functor(
        PL_new_atom(p_goal->get_functor().utf8().get_data()), arity);
    if (!PL_cons_functor_v(p_out_goal, f, arg_refs))
    {
        if (r_error)
            *r_error = "Failed to construct goal term";
        return false;
    }
    return true;
}

term_t godot_object_to_term(godot::Object* p_object)
{
    godot::Ref<PrologObject> handle = PrologObject::create(p_object);
    if (handle.is_null())
        return (term_t)0;
    return handle->to_swi_term();
}

static constexpr int MAX_TERM_DEPTH = 64;

static godot::Variant term_to_variant_at(term_t p_term, int p_depth);

static godot::Array list_term_to_array(term_t p_term, int p_depth)
{
    godot::Array list_array;
    term_t head = PL_new_term_ref();
    term_t tail = PL_copy_term_ref(p_term);
    while (PL_get_list(tail, head, tail))
        list_array.push_back(term_to_variant_at(head, p_depth + 1));
    return list_array;
}

static godot::Variant term_to_variant_at(term_t p_term, int p_depth)
{
    if (p_depth > MAX_TERM_DEPTH)
        return godot::Variant();

    int type = PL_term_type(p_term);

    switch (type)
    {
        case PL_VARIABLE:
            // Unbound variables cannot be converted to a concrete value
            return godot::Variant();

        case PL_ATOM:
        case PL_BLOB:
        {
            PL_blob_t* blob_type = nullptr;
            if (PL_is_blob(p_term, &blob_type) &&
                PrologObject::is_blob_type(blob_type))
            {
                godot::Ref<PrologObject> handle = PrologObject::from_swi_term(p_term);
                if (handle.is_valid())
                    return handle;
            }

            if (type == PL_BLOB)
                return godot::Variant();

            // Retained policy: Prolog atom → Godot String.
            //
            // Variant has no ATOM tag. Wrapping in PrologTerm::make_atom()
            // would keep the Prolog kind, but then solution.get() is an
            // object: `v == "bob"`, `match v: "attack"`, and `"bob" in names`
            // all fail (RefCounted identity, not text equality).
            //
            // The Prolog kind is dropped on purpose. A Prolog *string*
            // (PL_STRING below) stays a PrologTerm so it is not this String.
            // An explicit atom wrapper is only prolog.atom() on the way in.
            char* s;
            if (!PL_get_atom_chars(p_term, &s))
                return godot::Variant();
            return godot::String(s);
        }

        case PL_INTEGER:
        {
            int64_t i;
            if (!PL_get_int64(p_term, &i))
                return godot::Variant();
            return i;
        }

        case PL_FLOAT:
        {
            double d;
            if (!PL_get_float(p_term, &d))
                return godot::Variant();
            return d;
        }

        case PL_STRING:
        {
            // Prolog string → PrologTerm (kind string), not Godot String.
            // In SWI, hello and "hello" do not unify. If this returned a
            // String, get() could not tell it apart from the atom case above.
            char* s;
            size_t len;
            if (!PL_get_string_chars(p_term, &s, &len))
                return godot::Variant();
            (void)len;
            return PrologTerm::make_string(godot::String(s));
        }

        case PL_NIL:
            return godot::Array();

        case PL_LIST_PAIR:
            return list_term_to_array(p_term, p_depth);

        case PL_TERM:
        {
            if (PL_is_list(p_term))
                return list_term_to_array(p_term, p_depth);

            atom_t name;
            size_t arity;
            if (PL_get_name_arity(p_term, &name, &arity))
            {
                const char* atom_name = PL_atom_chars(name);
                if (arity == 0 && strcmp(atom_name, "[]") == 0)
                    return godot::Array();

                godot::Dictionary compound;
                compound["functor"] = godot::String(atom_name);

                godot::Array args;
                for (size_t i = 1; i <= arity; i++)
                {
                    term_t arg = PL_new_term_ref();
                    if (!PL_get_arg(i, p_term, arg))
                        return godot::Variant();
                    args.push_back(term_to_variant_at(arg, p_depth + 1));
                }
                compound["args"] = args;
                return compound;
            }
        }
    }

    return godot::Variant();
}

godot::Variant term_to_variant(term_t p_term)
{
    // Cyclic terms (X = [a|X], mutual compounds, …) would make the
    // iterative list walker run forever. Depth only guards nesting.
    if (!PL_is_acyclic(p_term))
        return godot::Variant();
    return term_to_variant_at(p_term, 0);
}

term_t variant_to_term(godot::Variant const& p_var)
{
    term_t t = PL_new_term_ref();

    switch (p_var.get_type())
    {
        case godot::Variant::NIL:
            if (!PL_put_atom_chars(t, "[]"))
                return (term_t)0;
            break;

        case godot::Variant::BOOL:
            if (!PL_put_atom_chars(t, (bool)p_var ? "true" : "false"))
                return (term_t)0;
            break;

        case godot::Variant::INT:
            if (!PL_put_int64(t, (int64_t)p_var))
                return (term_t)0;
            break;

        case godot::Variant::FLOAT:
            if (!PL_put_float(t, (double)p_var))
                return (term_t)0;
            break;

        case godot::Variant::STRING:
        case godot::Variant::STRING_NAME:
            // Retained policy: Godot String → Prolog atom.
            // "X" is the atom X, not a variable. A Prolog string must be
            // PrologTerm::make_string() / prolog.string().
            if (!PL_put_atom_chars(t, godot::String(p_var).utf8().get_data()))
                return (term_t)0;
            break;

        case godot::Variant::OBJECT:
        {
            godot::Ref<PrologTerm> term = p_var;
            if (term.is_valid())
            {
                std::map<int64_t, term_t> vars;
                std::vector<godot::Ref<PrologVariable>> order;
                return prolog_term_to_swi(term, vars, order);
            }
            godot::Ref<PrologObject> handle = p_var;
            if (handle.is_valid())
                return handle->to_swi_term();
            godot::Object* native = p_var;
            if (native)
                return godot_object_to_term(native);
            return (term_t)0;
        }

        case godot::Variant::ARRAY:
        {
            godot::Array arr = p_var;
            if (arr.size() == 0)
            {
                if (!PL_put_nil(t))
                    return (term_t)0;
            }
            else
            {
                term_t list = PL_new_term_ref();
                if (!PL_put_nil(list))
                    return (term_t)0;

                for (int i = arr.size() - 1; i >= 0; i--)
                {
                    term_t elem = variant_to_term(arr[i]);
                    if (!elem)
                        return (term_t)0;
                    term_t new_list = PL_new_term_ref();
                    if (!PL_cons_list(new_list, elem, list))
                        return (term_t)0;
                    list = new_list;
                }

                if (!PL_put_term(t, list))
                    return (term_t)0;
            }
            break;
        }

        case godot::Variant::VECTOR2:
        {
            godot::Vector2 v = p_var;
            godot::Array arr;
            arr.push_back(v.x);
            arr.push_back(v.y);
            return variant_to_term(arr);
        }

        case godot::Variant::VECTOR2I:
        {
            godot::Vector2i v = p_var;
            godot::Array arr;
            arr.push_back(v.x);
            arr.push_back(v.y);
            return variant_to_term(arr);
        }

        case godot::Variant::VECTOR3:
        {
            godot::Vector3 v = p_var;
            godot::Array arr;
            arr.push_back(v.x);
            arr.push_back(v.y);
            arr.push_back(v.z);
            return variant_to_term(arr);
        }

        case godot::Variant::VECTOR3I:
        {
            godot::Vector3i v = p_var;
            godot::Array arr;
            arr.push_back(v.x);
            arr.push_back(v.y);
            arr.push_back(v.z);
            return variant_to_term(arr);
        }

        case godot::Variant::DICTIONARY:
        {
            godot::Dictionary dict = p_var;
            if (dict.has("functor") && dict.has("args"))
            {
                godot::String functor = dict["functor"];
                godot::Array args_arr = dict["args"];
                int arity = args_arr.size();

                functor_t f = PL_new_functor(
                    PL_new_atom(functor.utf8().get_data()), arity);
                term_t args = PL_new_term_refs(arity);

                for (int i = 0; i < arity; i++)
                {
                    term_t arg = variant_to_term(args_arr[i]);
                    if (!arg || !PL_put_term(args + i, arg))
                        return (term_t)0;
                }

                if (!PL_cons_functor_v(t, f, args))
                    return (term_t)0;
            }
            else if (!PL_put_atom_chars(t, "[]"))
            {
                return (term_t)0;
            }
            break;
        }

        default:
            if (!PL_put_atom_chars(t, "[]"))
                return (term_t)0;
    }

    return t;
}

} // namespace prologot
