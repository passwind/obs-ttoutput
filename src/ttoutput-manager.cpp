#include "ttoutput-manager.h"
#include "ttoutput-plugin.h"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/threading.h>

// Global manager data
static struct {
    bool initialized;
    obs_video_info video_info;
    obs_audio_info audio_info;
    pthread_mutex_t mutex;
} g_manager_data = {0};

// Output callbacks
static void output_start_callback(void *data, calldata_t *cd)
{
    UNUSED_PARAMETER(cd);
    ttoutput_config_t *config = (ttoutput_config_t*)data;
    if (config) {
        config->status = OUTPUT_STATUS_STARTING;
    }
}

static void output_stop_callback(void *data, calldata_t *cd)
{
    UNUSED_PARAMETER(cd);
    ttoutput_config_t *config = (ttoutput_config_t*)data;
    if (config) {
        config->status = OUTPUT_STATUS_STOPPED;
    }
}

static void output_activate_callback(void *data, calldata_t *cd)
{
    UNUSED_PARAMETER(cd);
    ttoutput_config_t *config = (ttoutput_config_t*)data;
    if (config) {
        config->status = OUTPUT_STATUS_ACTIVE;
    }
}

static void output_deactivate_callback(void *data, calldata_t *cd)
{
    UNUSED_PARAMETER(cd);
    ttoutput_config_t *config = (ttoutput_config_t*)data;
    if (config) {
        config->status = OUTPUT_STATUS_STOPPED;
    }
}

// Manager functions
bool ttoutput_manager_init(void)
{
    if (g_manager_data.initialized) {
        return true;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&g_manager_data.mutex, NULL) != 0) {
        blog(LOG_ERROR, "TTOutput: Failed to initialize manager mutex");
        return false;
    }
    
    // Get current video/audio info
    obs_get_video_info(&g_manager_data.video_info);
    obs_get_audio_info(&g_manager_data.audio_info);
    
    g_manager_data.initialized = true;
    
    blog(LOG_INFO, "TTOutput: Manager initialized successfully");
    return true;
}

void ttoutput_manager_cleanup(void)
{
    if (!g_manager_data.initialized) {
        return;
    }
    
    pthread_mutex_destroy(&g_manager_data.mutex);
    g_manager_data.initialized = false;
    
    blog(LOG_INFO, "TTOutput: Manager cleanup completed");
}

obs_output_t* ttoutput_create_rtmp_output(ttoutput_config_t *config)
{
    if (!config || config->output_type != OUTPUT_TYPE_RTMP) {
        return NULL;
    }
    
    obs_output_t *output = obs_output_create(OUTPUT_TYPE_RTMP_STR, "ttoutput_rtmp", NULL, NULL);
    if (!output) {
        blog(LOG_ERROR, "TTOutput: Failed to create RTMP output");
        return NULL;
    }
    
    // Set output callbacks
    signal_handler_t *handler = obs_output_get_signal_handler(output);
    signal_handler_connect(handler, "start", output_start_callback, config);
    signal_handler_connect(handler, "stop", output_stop_callback, config);
    signal_handler_connect(handler, "activate", output_activate_callback, config);
    signal_handler_connect(handler, "deactivate", output_deactivate_callback, config);
    
    blog(LOG_INFO, "TTOutput: RTMP output created successfully");
    return output;
}

obs_output_t* ttoutput_create_file_output(ttoutput_config_t *config)
{
    if (!config || config->output_type != OUTPUT_TYPE_FILE) {
        return NULL;
    }
    
    obs_output_t *output = obs_output_create(OUTPUT_TYPE_FILE_STR, "ttoutput_file", NULL, NULL);
    if (!output) {
        blog(LOG_ERROR, "TTOutput: Failed to create file output");
        return NULL;
    }
    
    // Set output callbacks
    signal_handler_t *handler = obs_output_get_signal_handler(output);
    signal_handler_connect(handler, "start", output_start_callback, config);
    signal_handler_connect(handler, "stop", output_stop_callback, config);
    signal_handler_connect(handler, "activate", output_activate_callback, config);
    signal_handler_connect(handler, "deactivate", output_deactivate_callback, config);
    
    blog(LOG_INFO, "TTOutput: File output created successfully");
    return output;
}

obs_encoder_t* ttoutput_create_video_encoder(ttoutput_config_t *config)
{
    if (!config) {
        return NULL;
    }
    
    obs_encoder_t *encoder = obs_video_encoder_create(config->video_codec, "ttoutput_video_encoder", NULL, NULL);
    if (!encoder) {
        blog(LOG_ERROR, "TTOutput: Failed to create video encoder: %s", config->video_codec);
        return NULL;
    }
    
    // Configure encoder settings
    obs_data_t *settings = obs_data_create();
    
    // Basic video settings
    obs_data_set_int(settings, "bitrate", config->video_bitrate);
    obs_data_set_string(settings, "preset", config->video_preset);
    obs_data_set_string(settings, "profile", "main");
    obs_data_set_string(settings, "tune", "zerolatency");
    
    // Advanced settings
    obs_data_set_int(settings, "keyint_sec", 2);
    obs_data_set_string(settings, "rate_control", "CBR");
    obs_data_set_int(settings, "buffer_size", config->video_bitrate);
    
    // Apply settings
    obs_encoder_update(encoder, settings);
    obs_data_release(settings);
    
    blog(LOG_INFO, "TTOutput: Video encoder created: %s, bitrate: %d", 
         config->video_codec, config->video_bitrate);
    return encoder;
}

obs_encoder_t* ttoutput_create_audio_encoder(ttoutput_config_t *config)
{
    if (!config) {
        return NULL;
    }
    
    obs_encoder_t *encoder = obs_audio_encoder_create("ffmpeg_aac", "ttoutput_audio_encoder", NULL, 0, NULL);
    if (!encoder) {
        blog(LOG_ERROR, "TTOutput: Failed to create audio encoder");
        return NULL;
    }
    
    // Configure encoder settings
    obs_data_t *settings = obs_data_create();
    obs_data_set_int(settings, "bitrate", config->audio_bitrate);
    
    // Apply settings
    obs_encoder_update(encoder, settings);
    obs_data_release(settings);
    
    blog(LOG_INFO, "TTOutput: Audio encoder created, bitrate: %d", config->audio_bitrate);
    return encoder;
}

obs_service_t* ttoutput_create_rtmp_service(ttoutput_config_t *config)
{
    if (!config || config->output_type != OUTPUT_TYPE_RTMP) {
        return NULL;
    }
    
    obs_service_t *service = obs_service_create("rtmp_common", "ttoutput_rtmp_service", NULL, NULL);
    if (!service) {
        blog(LOG_ERROR, "TTOutput: Failed to create RTMP service");
        return NULL;
    }
    
    // Configure service settings
    obs_data_t *settings = obs_data_create();
    obs_data_set_string(settings, "server", config->rtmp_url);
    obs_data_set_string(settings, "key", config->rtmp_key);
    
    // Apply settings
    obs_service_update(service, settings);
    obs_data_release(settings);
    
    blog(LOG_INFO, "TTOutput: RTMP service created for URL: %s", config->rtmp_url);
    return service;
}

bool ttoutput_start_streaming(ttoutput_config_t *config)
{
    if (!config || config->output_type != OUTPUT_TYPE_RTMP) {
        return false;
    }
    
    pthread_mutex_lock(&g_manager_data.mutex);
    
    // Create RTMP service
    config->service = ttoutput_create_rtmp_service(config);
    if (!config->service) {
        pthread_mutex_unlock(&g_manager_data.mutex);
        return false;
    }
    
    // Create output
    config->output = ttoutput_create_rtmp_output(config);
    if (!config->output) {
        obs_service_release(config->service);
        config->service = NULL;
        pthread_mutex_unlock(&g_manager_data.mutex);
        return false;
    }
    
    // Create encoders
    config->video_encoder = ttoutput_create_video_encoder(config);
    config->audio_encoder = ttoutput_create_audio_encoder(config);
    
    if (!config->video_encoder || !config->audio_encoder) {
        ttoutput_stop_streaming(config);
        pthread_mutex_unlock(&g_manager_data.mutex);
        return false;
    }
    
    // Set up video mixer
    if (!ttoutput_setup_video_mixer(config)) {
        ttoutput_stop_streaming(config);
        pthread_mutex_unlock(&g_manager_data.mutex);
        return false;
    }
    
    // Set up audio mixer
    if (!ttoutput_setup_audio_mixer(config)) {
        ttoutput_stop_streaming(config);
        pthread_mutex_unlock(&g_manager_data.mutex);
        return false;
    }
    
    // Connect encoders to output
    obs_output_set_video_encoder(config->output, config->video_encoder);
    obs_output_set_audio_encoder(config->output, config->audio_encoder, 0);
    obs_output_set_service(config->output, config->service);
    
    // Start output
    bool success = obs_output_start(config->output);
    if (success) {
        config->status = OUTPUT_STATUS_STARTING;
        blog(LOG_INFO, "TTOutput: RTMP streaming started");
    } else {
        blog(LOG_ERROR, "TTOutput: Failed to start RTMP streaming");
        ttoutput_stop_streaming(config);
    }
    
    pthread_mutex_unlock(&g_manager_data.mutex);
    return success;
}

bool ttoutput_start_recording(ttoutput_config_t *config)
{
    if (!config || config->output_type != OUTPUT_TYPE_FILE) {
        return false;
    }
    
    pthread_mutex_lock(&g_manager_data.mutex);
    
    // Create output
    config->output = ttoutput_create_file_output(config);
    if (!config->output) {
        pthread_mutex_unlock(&g_manager_data.mutex);
        return false;
    }
    
    // Configure file output settings
    obs_data_t *settings = obs_data_create();
    obs_data_set_string(settings, "path", config->file_path);
    obs_data_set_string(settings, "format_name", config->file_format);
    obs_output_update(config->output, settings);
    obs_data_release(settings);
    
    // Create encoders
    config->video_encoder = ttoutput_create_video_encoder(config);
    config->audio_encoder = ttoutput_create_audio_encoder(config);
    
    if (!config->video_encoder || !config->audio_encoder) {
        ttoutput_stop_recording(config);
        pthread_mutex_unlock(&g_manager_data.mutex);
        return false;
    }
    
    // Set up video mixer
    if (!ttoutput_setup_video_mixer(config)) {
        ttoutput_stop_recording(config);
        pthread_mutex_unlock(&g_manager_data.mutex);
        return false;
    }
    
    // Set up audio mixer
    if (!ttoutput_setup_audio_mixer(config)) {
        ttoutput_stop_recording(config);
        pthread_mutex_unlock(&g_manager_data.mutex);
        return false;
    }
    
    // Connect encoders to output
    obs_output_set_video_encoder(config->output, config->video_encoder);
    obs_output_set_audio_encoder(config->output, config->audio_encoder, 0);
    
    // Start output
    bool success = obs_output_start(config->output);
    if (success) {
        config->status = OUTPUT_STATUS_STARTING;
        blog(LOG_INFO, "TTOutput: File recording started: %s", config->file_path);
    } else {
        blog(LOG_ERROR, "TTOutput: Failed to start file recording");
        ttoutput_stop_recording(config);
    }
    
    pthread_mutex_unlock(&g_manager_data.mutex);
    return success;
}

bool ttoutput_stop_streaming(ttoutput_config_t *config)
{
    if (!config || config->output_type != OUTPUT_TYPE_RTMP) {
        return false;
    }
    
    pthread_mutex_lock(&g_manager_data.mutex);
    
    // Stop output
    if (config->output) {
        obs_output_stop(config->output);
        obs_output_release(config->output);
        config->output = NULL;
    }
    
    // Clean up encoders
    if (config->video_encoder) {
        obs_encoder_release(config->video_encoder);
        config->video_encoder = NULL;
    }
    
    if (config->audio_encoder) {
        obs_encoder_release(config->audio_encoder);
        config->audio_encoder = NULL;
    }
    
    // Clean up service
    if (config->service) {
        obs_service_release(config->service);
        config->service = NULL;
    }
    
    // Clean up mixers
    ttoutput_cleanup_video_mixer(config);
    ttoutput_cleanup_audio_mixer(config);
    
    config->status = OUTPUT_STATUS_STOPPED;
    
    pthread_mutex_unlock(&g_manager_data.mutex);
    
    blog(LOG_INFO, "TTOutput: RTMP streaming stopped");
    return true;
}

bool ttoutput_stop_recording(ttoutput_config_t *config)
{
    if (!config || config->output_type != OUTPUT_TYPE_FILE) {
        return false;
    }
    
    pthread_mutex_lock(&g_manager_data.mutex);
    
    // Stop output
    if (config->output) {
        obs_output_stop(config->output);
        obs_output_release(config->output);
        config->output = NULL;
    }
    
    // Clean up encoders
    if (config->video_encoder) {
        obs_encoder_release(config->video_encoder);
        config->video_encoder = NULL;
    }
    
    if (config->audio_encoder) {
        obs_encoder_release(config->audio_encoder);
        config->audio_encoder = NULL;
    }
    
    // Clean up mixers
    ttoutput_cleanup_video_mixer(config);
    ttoutput_cleanup_audio_mixer(config);
    
    config->status = OUTPUT_STATUS_STOPPED;
    
    pthread_mutex_unlock(&g_manager_data.mutex);
    
    blog(LOG_INFO, "TTOutput: File recording stopped");
    return true;
}

bool ttoutput_start_output(ttoutput_config_t *config)
{
    if (!config) {
        return false;
    }
    
    if (config->output_type == OUTPUT_TYPE_RTMP) {
        return ttoutput_start_streaming(config);
    } else if (config->output_type == OUTPUT_TYPE_FILE) {
        return ttoutput_start_recording(config);
    }
    
    return false;
}

bool ttoutput_stop_output(ttoutput_config_t *config)
{
    if (!config) {
        return false;
    }
    
    if (config->output_type == OUTPUT_TYPE_RTMP) {
        return ttoutput_stop_streaming(config);
    } else if (config->output_type == OUTPUT_TYPE_FILE) {
        return ttoutput_stop_recording(config);
    }
    
    return false;
}

void ttoutput_update_status(ttoutput_config_t *config)
{
    if (!config || !config->output) {
        return;
    }
    
    if (obs_output_active(config->output)) {
        if (config->status != OUTPUT_STATUS_ACTIVE) {
            config->status = OUTPUT_STATUS_ACTIVE;
        }
    } else {
        if (config->status != OUTPUT_STATUS_STOPPED) {
            config->status = OUTPUT_STATUS_STOPPED;
        }
    }
}

uint64_t ttoutput_get_bytes_sent(ttoutput_config_t *config)
{
    if (!config || !config->output) {
        return 0;
    }
    
    return obs_output_get_total_bytes(config->output);
}

uint32_t ttoutput_get_dropped_frames(ttoutput_config_t *config)
{
    if (!config || !config->output) {
        return 0;
    }
    
    return obs_output_get_frames_dropped(config->output);
}

double ttoutput_get_cpu_usage(ttoutput_config_t *config)
{
    UNUSED_PARAMETER(config);
    
    // Get system CPU usage (simplified implementation)
    // In a real implementation, you would use platform-specific APIs
    return 0.0;
}

// Video/Audio mixer functions
bool ttoutput_setup_video_mixer(ttoutput_config_t *config)
{
    if (!config) {
        return false;
    }
    
    // Create video mixer with custom resolution
    struct obs_video_info ovi = {0};
    ovi.fps_num = config->video_fps;
    ovi.fps_den = 1;
    ovi.base_width = config->video_width;
    ovi.base_height = config->video_height;
    ovi.output_width = config->video_width;
    ovi.output_height = config->video_height;
    ovi.output_format = VIDEO_FORMAT_NV12;
    ovi.adapter = 0;
    ovi.gpu_conversion = true;
    ovi.colorspace = VIDEO_CS_709;
    ovi.range = VIDEO_RANGE_PARTIAL;
    
    // Create a custom video context for this output
    // Note: This is a simplified approach. In practice, you might want to
    // create a separate video context or use OBS's existing video system
    
    blog(LOG_INFO, "TTOutput: Video mixer setup completed (%dx%d@%dfps)", 
         config->video_width, config->video_height, config->video_fps);
    return true;
}

bool ttoutput_setup_audio_mixer(ttoutput_config_t *config)
{
    if (!config) {
        return false;
    }
    
    // Use default audio mixer (index 0)
    config->audio_mixer_idx = 0;
    
    // Configure audio sources to use the specified mixer
    for (int i = 0; i < config->source_count; i++) {
        if (!config->sources[i].enabled) {
            continue;
        }
        
        obs_source_t *source = obs_get_source_by_name(config->sources[i].name);
        if (!source) {
            blog(LOG_WARNING, "TTOutput: Audio source not found: %s", config->sources[i].name);
            continue;
        }
        
        uint32_t flags = obs_source_get_output_flags(source);
        if (flags & OBS_SOURCE_AUDIO) {
            // Set audio mixer flags for this source
            uint32_t mixer_flags = 1 << config->audio_mixer_idx;
            obs_source_set_audio_mixers(source, mixer_flags);
            blog(LOG_INFO, "TTOutput: Configured audio source for mixer %zu: %s", 
                 config->audio_mixer_idx, config->sources[i].name);
        }
        
        obs_source_release(source);
    }
    
    blog(LOG_INFO, "TTOutput: Audio mixer setup completed (mixer index: %zu)", config->audio_mixer_idx);
    return true;
}

void ttoutput_cleanup_video_mixer(ttoutput_config_t *config)
{
    if (!config) {
        return;
    }
    
    // Clean up video mixer resources
    // In a full implementation, you would clean up any custom video contexts here
    
    blog(LOG_INFO, "TTOutput: Video mixer cleanup completed");
}

void ttoutput_cleanup_audio_mixer(ttoutput_config_t *config)
{
    if (!config) {
        return;
    }
    
    // Reset audio mixer flags for all sources
    for (int i = 0; i < config->source_count; i++) {
        obs_source_t *source = obs_get_source_by_name(config->sources[i].name);
        if (source) {
            // Reset to default mixer (all mixers enabled)
            obs_source_set_audio_mixers(source, 0xFFFFFFFF);
            obs_source_release(source);
        }
    }
    
    config->audio_mixer_idx = 0;
    
    blog(LOG_INFO, "TTOutput: Audio mixer cleanup completed");
}

bool ttoutput_configure_video_encoder(ttoutput_config_t *config)
{
    if (!config || !config->video_encoder) {
        return false;
    }
    
    // Get video output from OBS
    video_t *video = obs_get_video();
    if (!video) {
        blog(LOG_ERROR, "TTOutput: No video context available");
        return false;
    }
    
    // Set video encoder to use custom video settings
    obs_encoder_set_video(config->video_encoder, video);
    
    // Configure encoder with selected sources
    // This is where you would implement custom video mixing logic
    // For now, we'll use OBS's default video output
    
    blog(LOG_INFO, "TTOutput: Video encoder configured");
    return true;
}

bool ttoutput_configure_audio_encoder(ttoutput_config_t *config)
{
    if (!config || !config->audio_encoder) {
        return false;
    }
    
    // Get audio output from OBS
    audio_t *audio = obs_get_audio();
    if (!audio) {
        blog(LOG_ERROR, "TTOutput: No audio context available");
        return false;
    }
    
    // Set audio encoder to use our mixer index
    obs_encoder_set_audio(config->audio_encoder, audio);
    
    blog(LOG_INFO, "TTOutput: Audio encoder configured with mixer index %zu", config->audio_mixer_idx);
    return true;
}

// Source management functions
bool ttoutput_add_video_source(ttoutput_config_t *config, const char *source_name)
{
    if (!config || !source_name || config->source_count >= MAX_SOURCES) {
        return false;
    }
    
    obs_source_t *source = obs_get_source_by_name(source_name);
    if (!source) {
        blog(LOG_WARNING, "TTOutput: Video source not found: %s", source_name);
        return false;
    }
    
    uint32_t flags = obs_source_get_output_flags(source);
    if (!(flags & OBS_SOURCE_VIDEO)) {
        blog(LOG_WARNING, "TTOutput: Source is not a video source: %s", source_name);
        obs_source_release(source);
        return false;
    }
    
    // Add to source list
    strncpy(config->sources[config->source_count].name, source_name, 
            sizeof(config->sources[config->source_count].name) - 1);
    config->sources[config->source_count].enabled = true;
    config->sources[config->source_count].volume = 1.0f;
    config->source_count++;
    
    obs_source_release(source);
    
    blog(LOG_INFO, "TTOutput: Added video source: %s", source_name);
    return true;
}

bool ttoutput_add_audio_source(ttoutput_config_t *config, const char *source_name, float volume)
{
    if (!config || !source_name || config->source_count >= MAX_SOURCES) {
        return false;
    }
    
    obs_source_t *source = obs_get_source_by_name(source_name);
    if (!source) {
        blog(LOG_WARNING, "TTOutput: Audio source not found: %s", source_name);
        return false;
    }
    
    uint32_t flags = obs_source_get_output_flags(source);
    if (!(flags & OBS_SOURCE_AUDIO)) {
        blog(LOG_WARNING, "TTOutput: Source is not an audio source: %s", source_name);
        obs_source_release(source);
        return false;
    }
    
    // Add to source list
    strncpy(config->sources[config->source_count].name, source_name, 
            sizeof(config->sources[config->source_count].name) - 1);
    config->sources[config->source_count].enabled = true;
    config->sources[config->source_count].volume = volume;
    config->source_count++;
    
    obs_source_release(source);
    
    blog(LOG_INFO, "TTOutput: Added audio source: %s (volume: %.2f)", source_name, volume);
    return true;
}

bool ttoutput_remove_source(ttoutput_config_t *config, const char *source_name)
{
    if (!config || !source_name) {
        return false;
    }
    
    for (int i = 0; i < config->source_count; i++) {
        if (strcmp(config->sources[i].name, source_name) == 0) {
            // Reset audio mixer flags for this source
            obs_source_t *source = obs_get_source_by_name(source_name);
            if (source) {
                obs_source_set_audio_mixers(source, 0xFFFFFFFF);
                obs_source_release(source);
            }
            
            // Shift remaining sources
            for (int j = i; j < config->source_count - 1; j++) {
                config->sources[j] = config->sources[j + 1];
            }
            config->source_count--;
            
            blog(LOG_INFO, "TTOutput: Removed source: %s", source_name);
            return true;
        }
    }
    
    blog(LOG_WARNING, "TTOutput: Source not found in list: %s", source_name);
    return false;
}

bool ttoutput_set_source_volume(ttoutput_config_t *config, const char *source_name, float volume)
{
    if (!config || !source_name) {
        return false;
    }
    
    for (int i = 0; i < config->source_count; i++) {
        if (strcmp(config->sources[i].name, source_name) == 0) {
            config->sources[i].volume = volume;
            
            // Note: OBS handles volume control at the source level
            // Volume is applied when the source is mixed
            
            blog(LOG_INFO, "TTOutput: Set source volume: %s = %.2f", source_name, volume);
            return true;
        }
    }
    
    blog(LOG_WARNING, "TTOutput: Source not found for volume adjustment: %s", source_name);
    return false;
}

bool ttoutput_enable_source(ttoutput_config_t *config, const char *source_name, bool enabled)
{
    if (!config || !source_name) {
        return false;
    }
    
    for (int i = 0; i < config->source_count; i++) {
        if (strcmp(config->sources[i].name, source_name) == 0) {
            config->sources[i].enabled = enabled;
            
            // Update mixer flags for this source
            obs_source_t *source = obs_get_source_by_name(source_name);
            if (source) {
                if (enabled) {
                    uint32_t mixer_flags = 1 << config->audio_mixer_idx;
                    obs_source_set_audio_mixers(source, mixer_flags);
                } else {
                    obs_source_set_audio_mixers(source, 0);
                }
                obs_source_release(source);
            }
            
            blog(LOG_INFO, "TTOutput: %s source: %s", enabled ? "Enabled" : "Disabled", source_name);
            return true;
        }
    }
    
    blog(LOG_WARNING, "TTOutput: Source not found for enable/disable: %s", source_name);
    return false;
}
