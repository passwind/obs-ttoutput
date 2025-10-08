/*
OBS TTOutput Plugin
Copyright (C) 2024 TTOutput Team

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "ttoutput-plugin.h"
#include "ttoutput-dock.h"
#include "ttoutput-manager.h"
#include "ttoutput-config.h"
#include <plugin-support.h>

#include <QApplication>
#include <QMainWindow>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

// Plugin constants
const char *PLUGIN_DISPLAY_NAME = "TTOutput";
const char *PLUGIN_VERSION_STR = PLUGIN_VERSION;

// Global plugin data
ttoutput_data_t *g_ttoutput_data = nullptr;
static TTOutputDock *g_dock_widget = nullptr;

bool obs_module_load(void)
{
    obs_log(LOG_INFO, "TTOutput plugin loading (version %s)", PLUGIN_VERSION);
    
    // Initialize plugin data
    if (!ttoutput_init()) {
        obs_log(LOG_ERROR, "Failed to initialize TTOutput plugin");
        return false;
    }
    
    // Initialize configuration system
    if (!ttoutput_config_init()) {
        obs_log(LOG_ERROR, "Failed to initialize configuration system");
        ttoutput_cleanup();
        return false;
    }
    
    // Initialize output manager
    if (!ttoutput_manager_init()) {
        obs_log(LOG_ERROR, "Failed to initialize output manager");
        ttoutput_config_cleanup();
        ttoutput_cleanup();
        return false;
    }
    
    // Register dock widget
    ttoutput_register_dock();
    
    obs_log(LOG_INFO, "TTOutput plugin loaded successfully");
    return true;
}

void obs_module_unload(void)
{
    obs_log(LOG_INFO, "TTOutput plugin unloading");
    
    // Note: Dock widget is managed by OBS frontend and cleaned up automatically
    // during obs_shutdown(). No manual deletion needed.
    g_dock_widget = nullptr;
    
    // Cleanup subsystems
    ttoutput_manager_cleanup();
    ttoutput_config_cleanup();
    ttoutput_cleanup();
    
    obs_log(LOG_INFO, "TTOutput plugin unloaded");
}

bool ttoutput_init(void)
{
    // Allocate global data
    g_ttoutput_data = (ttoutput_data_t*)bzalloc(sizeof(ttoutput_data_t));
    if (!g_ttoutput_data) {
        return false;
    }
    
    // Initialize configuration directory
    g_ttoutput_data->config_dir = ttoutput_get_config_dir();
    if (!g_ttoutput_data->config_dir) {
        bfree(g_ttoutput_data);
        g_ttoutput_data = nullptr;
        return false;
    }
    
    // Initialize global settings
    g_ttoutput_data->global_settings = obs_data_create();
    
    obs_log(LOG_INFO, "TTOutput plugin initialized, config dir: %s", 
            g_ttoutput_data->config_dir);
    
    return true;
}

void ttoutput_cleanup(void)
{
    if (!g_ttoutput_data) {
        return;
    }
    
    // Cleanup configurations
    if (g_ttoutput_data->configs) {
        for (size_t i = 0; i < g_ttoutput_data->config_count; i++) {
            ttoutput_config_t *config = &g_ttoutput_data->configs[i];
            
            // Stop output if active
            if (config->output && obs_output_active(config->output)) {
                obs_output_stop(config->output);
            }
            
            // Release resources
            if (config->output) {
                obs_output_release(config->output);
            }
            if (config->video_encoder) {
                obs_encoder_release(config->video_encoder);
            }
            if (config->audio_encoder) {
                obs_encoder_release(config->audio_encoder);
            }
            if (config->service) {
                obs_service_release(config->service);
            }
            if (config->settings) {
                obs_data_release(config->settings);
            }
            
            bfree(config->config_name);
            bfree(config->description);
        }
        bfree(g_ttoutput_data->configs);
    }
    
    // Cleanup sources
    if (g_ttoutput_data->sources) {
        for (size_t i = 0; i < g_ttoutput_data->source_count; i++) {
            bfree(g_ttoutput_data->sources[i].source_name);
        }
        bfree(g_ttoutput_data->sources);
    }
    
    // Cleanup global settings
    if (g_ttoutput_data->global_settings) {
        obs_data_release(g_ttoutput_data->global_settings);
    }
    
    bfree(g_ttoutput_data->config_dir);
    bfree(g_ttoutput_data);
    g_ttoutput_data = nullptr;
}

void ttoutput_register_dock(void)
{
    // Create dock widget
    TTOutputDock *dock_widget = new TTOutputDock();
    
    // Register with OBS frontend using the new API
    // OBS frontend takes ownership of the widget after this call
    obs_frontend_add_dock_by_id("ttoutput_dock", "TTOutput", dock_widget);
    
    // Set global pointer for reference (but OBS frontend manages the lifecycle)
    g_dock_widget = dock_widget;
    
    obs_log(LOG_INFO, "TTOutput dock widget registered");
}

char *ttoutput_get_config_dir(void)
{
    char *config_path = obs_module_config_path("");
    if (!config_path) {
        return nullptr;
    }
    
    struct dstr path = {0};
    dstr_copy(&path, config_path);
    dstr_cat(&path, "/ttoutput");
    
    bfree(config_path);
    
    // Ensure directory exists
    os_mkdirs(path.array);
    
    return path.array;
}
