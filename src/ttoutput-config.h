#pragma once

#include "ttoutput-plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configuration management functions
bool ttoutput_config_init(void);
void ttoutput_config_cleanup(void);

// Configuration object management
ttoutput_config_t *ttoutput_config_create(void);
void ttoutput_config_free(ttoutput_config_t *config);

// Configuration file operations
bool ttoutput_config_save(ttoutput_config_t *config, const char *name);
ttoutput_config_t *ttoutput_config_load(const char *name);
bool ttoutput_config_delete(const char *name);
char **ttoutput_config_get_list(size_t *count);
void ttoutput_config_free_list(char **list, size_t count);

// Configuration directory management
const char *ttoutput_config_get_dir(void);
bool ttoutput_config_ensure_dir(void);

// Default configuration
ttoutput_config_t *ttoutput_config_get_default(void);
bool ttoutput_config_apply_default(ttoutput_config_t *config);

// Configuration validation
bool ttoutput_config_validate(ttoutput_config_t *config);
bool ttoutput_config_validate_general(ttoutput_config_t *config);
bool ttoutput_config_validate_rtmp(ttoutput_config_t *config);
bool ttoutput_config_validate_file(ttoutput_config_t *config);
bool ttoutput_config_validate_encoder(ttoutput_config_t *config);

// Configuration conversion
obs_data_t *ttoutput_config_from_json(const char *json_str);
char *ttoutput_config_to_json(obs_data_t *config);

// Configuration migration
bool ttoutput_config_migrate(obs_data_t *config, int from_version, int to_version);
int ttoutput_config_get_version(obs_data_t *config);
void ttoutput_config_set_version(obs_data_t *config, int version);

// Global settings
obs_data_t *ttoutput_config_get_global_settings(void);
void ttoutput_config_save_global_settings(obs_data_t *settings);

#ifdef __cplusplus
}
#endif
