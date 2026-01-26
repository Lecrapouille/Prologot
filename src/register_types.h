/*
 * MIT License
 * Copyright (c) 2024 Lecrapouille <lecrapouille@gmail.com>
 *
 * Prologot - SWI-Prolog integration for Godot 4
 *
 * This file declares the module initialization functions for the Prologot
 * GDExtension. These functions are called by Godot when the extension
 * is loaded/unloaded.
 */

#pragma once

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

/**
 * @brief Initializes the Prologot module.
 *
 * Called by Godot when the extension is loaded. Registers all classes
 * that should be available to GDScript.
 *
 * @param p_level The initialization level at which this is called.
 */
void initialize_prologot_module(ModuleInitializationLevel p_level);

/**
 * @brief Uninitializes the Prologot module.
 *
 * Called by Godot when the extension is unloaded. Performs cleanup
 * operations if needed.
 *
 * @param p_level The initialization level at which this is called.
 */
void uninitialize_prologot_module(ModuleInitializationLevel p_level);
