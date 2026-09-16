/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file defines PrologObject: a stable handle to a Godot Object
 * (Node, Resource, ...) stored in SWI-Prolog as a blob. Prolog never
 * holds a raw pointer; it stores the Godot instance id.
 */

#pragma once

#include <SWI-Prolog.h>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

namespace prologot
{

/**
 * @class PrologObject
 * @brief Handle to a Godot Object usable as a Prolog argument.
 *
 * Created with prolog.object(node) or by passing a Node / Resource to
 * call(). Identity is the Godot instance id: two handles to the same
 * live object unify. If the object is freed (scene change, queue_free),
 * is_valid() becomes false and get_object() returns null; existing
 * blobs keep the id so comparison still works.
 *
 * Not thread-safe: use from the Godot main thread only (see étape 17).
 *
 * @example
 * var player = prolog.object($Player)
 * var at = prolog.predicate("at")
 * prolog.assert_fact(at.call(player, "zone_1"))
 * prolog.solve(at.call(player, "zone_1")).has_solution()
 */
class PrologObject: public godot::RefCounted
{
    GDCLASS(PrologObject, godot::RefCounted)

public:

    /**
     * @brief Constructs an empty handle (filled by create()).
     */
    PrologObject() = default;

    /**
     * @brief Destructs the handle. Does not free the Godot object.
     */
    ~PrologObject() override = default;

    /**
     * @brief Wraps a Godot Object as a Prolog handle.
     *
     * @param p_object Node, Resource, or any Object. Null returns null.
     * @return A new PrologObject, or null if p_object is null.
     *
     * @example
     * var player = prolog.object($Player)
     * print(player.is_valid())  # true while the node exists
     */
    static godot::Ref<PrologObject> create(godot::Object* p_object);

    /**
     * @brief Wraps a Godot instance id (used when reading a blob back).
     *
     * @param p_instance_id Godot Object.get_instance_id().
     * @return A handle; is_valid() is false if that id is gone.
     */
    static godot::Ref<PrologObject> create_from_id(uint64_t p_instance_id);

    /**
     * @brief Registers the SWI-Prolog blob type. Call after PL_initialise().
     */
    static void register_blob_type();

    /**
     * @brief Returns true if p_type is the Godot-object blob type.
     */
    static bool is_blob_type(PL_blob_t* p_type);

    /**
     * @brief Writes this handle as a SWI-Prolog blob term.
     *
     * @return A term_t blob, or 0 on failure.
     */
    term_t to_swi_term() const;

    /**
     * @brief Reads a Godot-object blob from a Prolog term.
     *
     * @param p_term Term that may hold a godot_object blob.
     * @return A PrologObject, or null if the term is not our blob.
     */
    static godot::Ref<PrologObject> from_swi_term(term_t p_term);

    /**
     * @brief Returns the live Godot object, or null if it was freed.
     *
     * @return The original Object, or nullptr after queue_free / scene change.
     *
     * @example
     * var handle = prolog.object($Player)
     * print(handle.get_object() == $Player)  # true
     * $Player.free()
     * print(handle.get_object())             # null
     */
    godot::Object* get_object() const;

    /**
     * @brief Returns true if ObjectDB still has this instance id.
     *
     * @return true while the Godot object is alive.
     *
     * @example
     * var handle = prolog.object($Player)
     * print(handle.is_valid())  # true
     * $Player.free()
     * print(handle.is_valid())  # false
     */
    bool is_valid() const;

    /**
     * @brief Returns the Godot instance id (stable handle).
     *
     * Two handles with the same id unify in Prolog even if get_object()
     * is already null.
     *
     * @return Godot instance id stored in the SWI blob.
     *
     * @example
     * print(prolog.object($Player).get_instance_id() == $Player.get_instance_id())
     */
    uint64_t get_instance_id() const { return m_instance_id; }

    /**
     * @brief Returns the Godot class name, or empty if the object is gone.
     *
     * @return e.g. "Node2D", or "" after the object is freed.
     *
     * @example
     * print(prolog.object($Player).get_class_name())  # Node
     */
    godot::String get_class_name() const;

    /**
     * @brief Returns true if both handles refer to the same instance id.
     *
     * @param p_other Other handle. Null returns false.
     * @return true if the ids match.
     *
     * @example
     * var a = prolog.object($Player)
     * var b = prolog.object($Player)
     * print(a.equals(b))  # true
     */
    bool equals(godot::Ref<PrologObject> const& p_other) const;

    /**
     * @brief Debug label, e.g. Player:<Node#123> or <freed#123>.
     *
     * @return A short string for print() and as_text() of goals.
     *
     * @example
     * print(prolog.object($Player).as_text())
     */
    godot::String as_text() const;

protected:

    /**
     * @brief Binds get_object / is_valid / equals / as_text to GDScript.
     */
    static void _bind_methods();

private:

    uint64_t m_instance_id = 0;
};

} // namespace prologot
