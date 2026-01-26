/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file implements the module initialization functions for the Prologot
 * GDExtension. It registers the Prologot class with Godot's class database
 * so it can be used from GDScript.
 */

#include "register_types.h"
#include "Prologot.hpp"

#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

/**
 * @brief Initializes the Prologot module.
 *
 * This function is called by Godot at the SCENE initialization level.
 * It registers the Prologot class with Godot's class database, making it
 * available to GDScript.
 *
 * @param p_level The initialization level. We only initialize at SCENE level.
 */
void initialize_prologot_module(ModuleInitializationLevel p_level)
{
    // Only initialize at SCENE level (after core systems are ready)
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }

    // Register the Prologot class with Godot's class database
    // This makes it available to GDScript and the editor
    ClassDB::register_class<Prologot>();
}

/**
 * @brief Uninitializes the Prologot module.
 *
 * This function is called by Godot when the extension is unloaded.
 * Currently, no cleanup is needed as Prologot handles its own cleanup
 * in the destructor.
 *
 * @param p_level The initialization level. We only uninitialize at SCENE level.
 */
void uninitialize_prologot_module(ModuleInitializationLevel p_level)
{
    // Only uninitialize at SCENE level
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        return;
    }
    // No cleanup needed - Prologot destructor handles cleanup
}

/**
 * @brief Entry point for the GDExtension library.
 *
 * This function is called by Godot when the extension is loaded.
 * It sets up the initialization and termination callbacks for the module.
 *
 * @param p_get_proc_address Function pointer to get Godot API functions.
 * @param p_library Pointer to the library instance.
 * @param r_initialization Pointer to initialization structure to fill.
 * @return true if initialization succeeded, false otherwise.
 */
extern "C"
{
    GDExtensionBool GDE_EXPORT
    prologot_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                          const GDExtensionClassLibraryPtr p_library,
                          GDExtensionInitialization* r_initialization)
    {
        // Create initialization object with Godot API access
        godot::GDExtensionBinding::InitObject init_obj(
            p_get_proc_address, p_library, r_initialization);

        // Register our initialization function (called when extension loads)
        init_obj.register_initializer(initialize_prologot_module);

        // Register our termination function (called when extension unloads)
        init_obj.register_terminator(uninitialize_prologot_module);

        // Set the minimum initialization level required
        // SCENE level means we need scene systems to be ready
        init_obj.set_minimum_library_initialization_level(
            MODULE_INITIALIZATION_LEVEL_SCENE);

        // Perform the actual initialization
        return init_obj.init();
    }
}
