/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file defines PrologTerm, the Godot representation of a Prolog term
 * (atom, number, string, list, or compound). Users never touch SWI-Prolog
 * term_t. Create terms via Prologot factories (atom, integer, list, ...).
 */

#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace prologot
{

/**
 * @class PrologTerm
 * @brief Godot representation of a Prolog term.
 *
 * A term is an atom, integer, float, Prolog string, empty list (nil),
 * list, compound, or variable. Variables are the subclass PrologVariable.
 *
 * Prefer the factories on Prologot rather than constructing terms by hand:
 * prolog.atom(), prolog.integer(), prolog.real(), prolog.string(),
 * prolog.nil(), prolog.list(), prolog.compound().
 *
 * @example
 * var name = prolog.atom("tom")
 * var hp = prolog.integer(42)
 * var items = prolog.list(["sword", "shield"])
 * print(name.as_text())    # tom
 * print(items.get_kind())  # list
 */
class PrologTerm: public godot::RefCounted
{
    GDCLASS(PrologTerm, godot::RefCounted)

public:

    /**
     * @brief Discriminator for the stored Prolog term kind.
     */
    enum Kind
    {
        KIND_ATOM = 0,
        KIND_INTEGER,
        KIND_FLOAT,
        KIND_STRING,
        KIND_NIL,
        KIND_LIST,
        KIND_COMPOUND,
        KIND_VARIABLE
    };

    /**
     * @brief Constructs an empty term (kind nil until a factory fills it).
     */
    PrologTerm() = default;

    /**
     * @brief Destructs the term. RefCounted releases it automatically.
     */
    ~PrologTerm() override = default;

    /**
     * @brief Creates an atom term.
     *
     * A GDScript String passed to call() is also an atom. Use this factory
     * when you need an explicit PrologTerm object.
     *
     * @param p_name Atom name (e.g. "tom").
     * @return A term whose get_kind() is "atom".
     *
     * @example
     * var tom = prolog.atom("tom")
     * print(tom.is_atom())   # true
     * print(tom.get_atom())  # tom
     */
    static godot::Ref<PrologTerm> make_atom(godot::String const& p_name);

    /**
     * @brief Creates an integer term.
     *
     * @param p_value Signed 64-bit integer.
     * @return A term whose get_kind() is "integer".
     *
     * @example
     * var n = prolog.integer(42)
     * print(n.get_integer())  # 42
     */
    static godot::Ref<PrologTerm> make_integer(int64_t p_value);

    /**
     * @brief Creates a floating-point term.
     *
     * @param p_value IEEE-754 double.
     * @return A term whose get_kind() is "float".
     *
     * @example
     * var pi = prolog.real(3.14)
     * print(pi.get_real())  # 3.14
     */
    static godot::Ref<PrologTerm> make_real(double p_value);

    /**
     * @brief Creates a Prolog string term (distinct from an atom).
     *
     * A GDScript String sent as a call() argument is an atom, not a Prolog
     * string. Use this factory when you need a quoted Prolog string.
     *
     * @param p_value String contents.
     * @return A term whose get_kind() is "string".
     *
     * @example
     * var s = prolog.string("hello")
     * print(s.as_text())  # "hello"
     */
    static godot::Ref<PrologTerm> make_string(godot::String const& p_value);

    /**
     * @brief Creates the empty list [].
     *
     * @return A term whose get_kind() is "nil".
     *
     * @example
     * print(prolog.nil().as_text())  # []
     */
    static godot::Ref<PrologTerm> make_nil();

    /**
     * @brief Creates a list term from a Godot Array.
     *
     * An empty Array becomes nil ([]). Nested Arrays become nested lists.
     *
     * @param p_items List elements (Variants or PrologTerm).
     * @return A term whose get_kind() is "list", or nil if empty.
     *
     * @example
     * var nums = prolog.list([1, 2, 3])
     * print(nums.as_text())  # [1, 2, 3]
     */
    static godot::Ref<PrologTerm> make_list(godot::Array const& p_items);

    /**
     * @brief Creates a compound term functor(args...).
     *
     * @param p_functor Functor name (e.g. "point").
     * @param p_args Arguments in order.
     * @return A term whose get_kind() is "compound".
     *
     * @example
     * var point = prolog.compound("point", [10, 20])
     * print(point.as_text())      # point(10, 20)
     * print(point.get_functor())  # point
     */
    static godot::Ref<PrologTerm> make_compound(godot::String const& p_functor,
                                         godot::Array const& p_args);

    /**
     * @brief Returns the kind as a GDScript string.
     *
     * @return One of "atom", "integer", "float", "string", "nil", "list",
     * "compound", "variable".
     *
     * @example
     * print(prolog.atom("tom").get_kind())  # atom
     */
    godot::String get_kind() const;

    /**
     * @brief Returns the kind as a C++ enum (not bound to GDScript).
     */
    Kind get_kind_enum() const { return m_kind; }

    /**
     * @brief Returns true if this term is an atom.
     *
     * @example
     * print(prolog.atom("tom").is_atom())  # true
     */
    bool is_atom() const { return m_kind == KIND_ATOM; }

    /**
     * @brief Returns true if this term is an integer.
     *
     * @example
     * print(prolog.integer(42).is_integer())  # true
     */
    bool is_integer() const { return m_kind == KIND_INTEGER; }

    /**
     * @brief Returns true if this term is a float.
     *
     * @example
     * print(prolog.real(3.14).is_float())  # true
     */
    bool is_float() const { return m_kind == KIND_FLOAT; }

    /**
     * @brief Returns true if this term is a Prolog string.
     *
     * Distinct from an atom: prolog.string("hello") vs the atom hello.
     *
     * @example
     * print(prolog.string("hello").is_string())  # true
     * print(prolog.atom("hello").is_string())    # false
     */
    bool is_string() const { return m_kind == KIND_STRING; }

    /**
     * @brief Returns true if this term is the empty list.
     *
     * @example
     * print(prolog.nil().is_nil())      # true
     * print(prolog.list([]).is_nil())   # true
     */
    bool is_nil() const { return m_kind == KIND_NIL; }

    /**
     * @brief Returns true if this term is a non-empty list.
     *
     * @example
     * print(prolog.list([1, 2]).is_list())  # true
     * print(prolog.nil().is_list())         # false
     */
    bool is_list() const { return m_kind == KIND_LIST; }

    /**
     * @brief Returns true if this term is a compound.
     *
     * @example
     * print(prolog.compound("point", [1, 2]).is_compound())  # true
     */
    bool is_compound() const { return m_kind == KIND_COMPOUND; }

    /**
     * @brief Returns true if this term is a PrologVariable.
     *
     * @example
     * print(prolog.variable("X").is_variable())  # true
     * print(prolog.atom("X").is_variable())      # false
     */
    bool is_variable() const { return m_kind == KIND_VARIABLE; }

    /**
     * @brief Returns the atom name, or an empty string if not an atom.
     *
     * @example
     * print(prolog.atom("tom").get_atom())  # tom
     */
    godot::String get_atom() const;

    /**
     * @brief Returns the integer value (0 if this is not an integer).
     *
     * @example
     * print(prolog.integer(-7).get_integer())  # -7
     */
    int64_t get_integer() const { return m_integer; }

    /**
     * @brief Returns the float value (0.0 if this is not a float).
     *
     * @example
     * print(prolog.real(1.5).get_real())  # 1.5
     */
    double get_real() const { return m_real; }

    /**
     * @brief Returns the Prolog string contents, or empty if not a string.
     *
     * Bound to GDScript as get_string().
     *
     * @example
     * print(prolog.string("hello").get_string())  # hello
     */
    godot::String get_string_value() const;

    /**
     * @brief Returns the functor name, or empty if not a compound.
     *
     * @example
     * print(prolog.compound("point", [1, 2]).get_functor())  # point
     */
    godot::String get_functor() const;

    /**
     * @brief Returns list elements or compound arguments.
     *
     * @example
     * print(prolog.compound("point", [10, 20]).get_args())  # [10, 20]
     */
    godot::Array get_args() const { return m_args; }

    /**
     * @brief Returns a readable Prolog-like representation.
     *
     * @return e.g. tom, 42, "hello", [], [1, 2], point(10, 20), or _ / a name.
     *
     * @example
     * print(prolog.compound("parent", ["tom", "bob"]).as_text())
     * # parent(tom, bob)
     */
    godot::String as_text() const;

protected:

    /**
     * @brief Binds term inspectors to GDScript.
     */
    static void _bind_methods();

    void set_kind(Kind p_kind) { m_kind = p_kind; }
    void set_text(godot::String const& p_text) { m_text = p_text; }
    void set_integer(int64_t p_value) { m_integer = p_value; }
    void set_real(double p_value) { m_real = p_value; }
    void set_args(godot::Array const& p_args) { m_args = p_args; }

private:

    Kind m_kind = KIND_NIL;
    godot::String m_text;
    int64_t m_integer = 0;
    double m_real = 0.0;
    godot::Array m_args;
};

} // namespace prologot
