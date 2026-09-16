/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * Variant ↔ term_t conversion. Stateless: no Prologot instance required.
 */

#pragma once

#include "PrologGoal.hpp"
#include "PrologTerm.hpp"
#include "PrologVariable.hpp"
#include <SWI-Prolog.h>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <map>
#include <vector>

using namespace godot;

namespace PrologConversion
{

/**
 * @brief Converts a Prolog term to a Godot Variant.
 *
 * Handles atoms, integers, floats, strings, lists, blobs, and compounds.
 */
Variant term_to_variant(term_t p_term);

/**
 * @brief Converts a Godot Variant to a Prolog term.
 *
 * Handles NIL, bool, int, float, String, Array, vectors, and objects.
 * @return The created Prolog term (0 if conversion failed).
 */
term_t variant_to_term(Variant const& p_var);

/**
 * @brief Converts one call() argument (atom, number, list, variable, object).
 */
term_t object_arg_to_term(Variant const& p_arg,
                          std::map<int64_t, term_t>& p_vars,
                          std::vector<Ref<PrologVariable>>& p_order);

/**
 * @brief Converts a PrologTerm / PrologVariable tree into a SWI term_t.
 */
term_t prolog_term_to_swi(Ref<PrologTerm> const& p_term,
                          std::map<int64_t, term_t>& p_vars,
                          std::vector<Ref<PrologVariable>>& p_order);

/**
 * @brief Converts a Godot Object / PrologObject to a SWI blob term.
 */
term_t godot_object_to_term(Object* p_object);

/**
 * @brief Builds a SWI goal term from a PrologGoal (including compositions).
 *
 * On failure, writes a message to r_error when it is not null.
 */
bool compile_goal(Ref<PrologGoal> const& p_goal,
                  term_t p_out_goal,
                  std::map<int64_t, term_t>& p_vars,
                  std::vector<Ref<PrologVariable>>& p_order,
                  String* r_error = nullptr);

} // namespace PrologConversion
