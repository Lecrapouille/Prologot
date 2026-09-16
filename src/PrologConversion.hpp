/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * Variant ↔ term_t conversion. Stateless: no Prologot instance required.
 *
 * Text policy (hybrid, retained):
 *   Godot String  → Prolog atom          (never a Prolog string, never a variable)
 *   Prolog atom   → Godot String         (Variant has no ATOM tag; keep == / match)
 *   Prolog string → PrologTerm (KIND_STRING), so it is not confused with an atom
 *
 * Do not return PrologTerm::make_atom() from term_to_variant: that would make
 * solution.get() an object and break `v == "bob"` / `match v: "attack"`.
 * Use prolog.atom() only when the caller wants an explicit PrologTerm.
 *
 * See doc/glossary.md § conversion philosophies.
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

namespace prologot
{

/**
 * @brief Converts a Prolog term to a Godot Variant (solve / get() output).
 *
 * Mapping:
 *   atom            → String
 *   Prolog string   → PrologTerm (kind string)
 *   integer / float → int / float
 *   list / []       → Array
 *   compound        → Dictionary {functor, args} (functor is a String)
 *   Godot blob      → PrologObject
 *   unbound var     → NIL (omitted from PrologSolution)
 *
 * Atoms become String on purpose: Godot Variant has no ATOM type, and game
 * code treats names as String (`== "bob"`, `match`, `"bob" in array`).
 */
godot::Variant term_to_variant(term_t p_term);

/**
 * @brief Converts a Godot Variant to a Prolog term (call() input).
 *
 * Mapping:
 *   String / StringName → atom          (never a Prolog string, never a variable)
 *   PrologTerm          → its kind      (atom, string, integer, list, …)
 *   PrologVariable      → variable
 *   PrologObject / Node → Godot blob
 *   int / float         → integer / float
 *   bool                → atom true / false
 *   Array               → list ([] if empty)
 *   Vector2 / Vector2i  → [x, y]
 *   Vector3 / Vector3i  → [x, y, z]
 *   Dictionary          → compound if {functor, args}, else []
 *   NIL / unknown       → []
 *
 * A Prolog string must be a PrologTerm from prolog.string() /
 * PrologTerm::make_string(). "X" is the atom X.
 *
 * @return The created Prolog term (0 if conversion failed).
 */
term_t variant_to_term(godot::Variant const& p_var);

/**
 * @brief Converts one call() argument (atom, number, list, variable, object).
 */
term_t object_arg_to_term(godot::Variant const& p_arg,
                          std::map<int64_t, term_t>& p_vars,
                          std::vector<godot::Ref<PrologVariable>>& p_order);

/**
 * @brief Converts a PrologTerm / PrologVariable tree into a SWI term_t.
 */
term_t prolog_term_to_swi(godot::Ref<PrologTerm> const& p_term,
                          std::map<int64_t, term_t>& p_vars,
                          std::vector<godot::Ref<PrologVariable>>& p_order);

/**
 * @brief Converts a Godot Object / PrologObject to a SWI blob term.
 */
term_t godot_object_to_term(godot::Object* p_object);

/**
 * @brief Builds a SWI goal term from a PrologGoal (including compositions).
 *
 * On failure, writes a message to r_error when it is not null.
 */
bool compile_goal(godot::Ref<PrologGoal> const& p_goal,
                  term_t p_out_goal,
                  std::map<int64_t, term_t>& p_vars,
                  std::vector<godot::Ref<PrologVariable>>& p_order,
                  godot::String* r_error = nullptr);

} // namespace prologot
