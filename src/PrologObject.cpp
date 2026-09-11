/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 */

#include "PrologObject.hpp"
#include <SWI-Stream.h>
#include <cstdio>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

namespace
{
struct GodotObjectBlob
{
    uint64_t instance_id = 0;
};

int release_godot_object(atom_t)
{
    // The blob does not own the Godot object (weak handle via instance id).
    return TRUE;
}

int compare_godot_object(atom_t p_a, atom_t p_b)
{
    size_t len_a = 0;
    size_t len_b = 0;
    auto* a = static_cast<GodotObjectBlob*>(PL_blob_data(p_a, &len_a, nullptr));
    auto* b = static_cast<GodotObjectBlob*>(PL_blob_data(p_b, &len_b, nullptr));
    uint64_t id_a = a ? a->instance_id : 0;
    uint64_t id_b = b ? b->instance_id : 0;
    if (id_a < id_b)
        return -1;
    if (id_a > id_b)
        return 1;
    return 0;
}

int write_godot_object(IOSTREAM* p_stream, atom_t p_atom, int)
{
    size_t len = 0;
    auto* blob =
        static_cast<GodotObjectBlob*>(PL_blob_data(p_atom, &len, nullptr));
    uint64_t id = blob ? blob->instance_id : 0;
    Sfprintf(p_stream, "<godot_object:%llu>", (unsigned long long)id);
    return TRUE;
}

PL_blob_t g_godot_object_blob;
bool g_blob_ready = false;
} // namespace

void PrologObject::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_object"), &PrologObject::get_object);
    ClassDB::bind_method(D_METHOD("is_valid"), &PrologObject::is_valid);
    ClassDB::bind_method(D_METHOD("get_instance_id"),
                         &PrologObject::get_instance_id);
    ClassDB::bind_method(D_METHOD("get_class_name"),
                         &PrologObject::get_class_name);
    ClassDB::bind_method(D_METHOD("equals", "other"), &PrologObject::equals);
    ClassDB::bind_method(D_METHOD("as_text"), &PrologObject::as_text);
}

void PrologObject::register_blob_type()
{
    if (g_blob_ready)
        return;

    g_godot_object_blob = PL_blob_t{};
    g_godot_object_blob.magic = PL_BLOB_MAGIC;
    g_godot_object_blob.flags = PL_BLOB_UNIQUE;
    g_godot_object_blob.name = "godot_object";
    g_godot_object_blob.release = release_godot_object;
    g_godot_object_blob.compare = compare_godot_object;
    g_godot_object_blob.write = write_godot_object;
    PL_register_blob_type(&g_godot_object_blob);
    g_blob_ready = true;
}

bool PrologObject::is_blob_type(PL_blob_t* p_type)
{
    return g_blob_ready && p_type == &g_godot_object_blob;
}

Ref<PrologObject> PrologObject::create(Object* p_object)
{
    if (!p_object)
        return Ref<PrologObject>();
    return create_from_id(p_object->get_instance_id());
}

Ref<PrologObject> PrologObject::create_from_id(uint64_t p_instance_id)
{
    if (p_instance_id == 0)
        return Ref<PrologObject>();
    Ref<PrologObject> handle;
    handle.instantiate();
    handle->m_instance_id = p_instance_id;
    return handle;
}

term_t PrologObject::to_swi_term() const
{
    if (!g_blob_ready || m_instance_id == 0)
        return (term_t)0;

    GodotObjectBlob blob;
    blob.instance_id = m_instance_id;
    term_t t = PL_new_term_ref();
    if (!PL_put_blob(t, &blob, sizeof(blob), &g_godot_object_blob))
        return (term_t)0;
    return t;
}

Ref<PrologObject> PrologObject::from_swi_term(term_t p_term)
{
    void* data = nullptr;
    size_t len = 0;
    PL_blob_t* type = nullptr;
    if (!PL_get_blob(p_term, &data, &len, &type) || !is_blob_type(type) ||
        !data || len < sizeof(GodotObjectBlob))
        return Ref<PrologObject>();

    auto* blob = static_cast<GodotObjectBlob*>(data);
    return create_from_id(blob->instance_id);
}

Object* PrologObject::get_object() const
{
    if (m_instance_id == 0)
        return nullptr;
    return ObjectDB::get_instance(m_instance_id);
}

bool PrologObject::is_valid() const
{
    return get_object() != nullptr;
}

String PrologObject::get_class_name() const
{
    Object* object = get_object();
    if (!object)
        return String();
    return object->get_class();
}

bool PrologObject::equals(Ref<PrologObject> const& p_other) const
{
    if (p_other.is_null())
        return false;
    return m_instance_id == p_other->m_instance_id;
}

String PrologObject::as_text() const
{
    Object* object = get_object();
    if (!object)
        return String("<freed#") + String::num_uint64(m_instance_id) +
               String(">");
    return object->get_class() + String("#") +
           String::num_uint64(m_instance_id);
}
