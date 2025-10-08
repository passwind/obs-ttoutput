#pragma once

#include "ttoutput-plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

// Output manager functions
bool ttoutput_manager_init(void);
void ttoutput_manager_cleanup(void);

// Output creation and management
obs_output_t *ttoutput_create_rtmp_output(ttoutput_config_t *config);
obs_output_t *ttoutput_create_file_output(ttoutput_config_t *config);
obs_encoder_t *ttoutput_create_video_encoder(ttoutput_config_t *config);
obs_encoder_t *ttoutput_create_audio_encoder(ttoutput_config_t *config);
obs_service_t *ttoutput_create_rtmp_service(ttoutput_config_t *config);

// Output control
bool ttoutput_start_streaming(ttoutput_config_t *config);
bool ttoutput_start_recording(ttoutput_config_t *config);
bool ttoutput_stop_streaming(ttoutput_config_t *config);
bool ttoutput_stop_recording(ttoutput_config_t *config);

// Status monitoring
void ttoutput_update_status(ttoutput_config_t *config);
uint64_t ttoutput_get_bytes_sent(ttoutput_config_t *config);
uint32_t ttoutput_get_dropped_frames(ttoutput_config_t *config);
double ttoutput_get_cpu_usage(ttoutput_config_t *config);

// Source mixing
bool ttoutput_setup_video_mixer(ttoutput_config_t *config);
bool ttoutput_setup_audio_mixer(ttoutput_config_t *config);
void ttoutput_cleanup_video_mixer(ttoutput_config_t *config);
void ttoutput_cleanup_audio_mixer(ttoutput_config_t *config);

// Encoder configuration
bool ttoutput_configure_video_encoder(ttoutput_config_t *config);
bool ttoutput_configure_audio_encoder(ttoutput_config_t *config);

// Source management
bool ttoutput_add_video_source(ttoutput_config_t *config, const char *source_name);
bool ttoutput_add_audio_source(ttoutput_config_t *config, const char *source_name, float volume);
bool ttoutput_remove_source(ttoutput_config_t *config, const char *source_name);

// Output callbacks
void ttoutput_output_start_callback(void *data, calldata_t *cd);
void ttoutput_output_stop_callback(void *data, calldata_t *cd);
void ttoutput_output_error_callback(void *data, calldata_t *cd);

#ifdef __cplusplus
}
#endif
