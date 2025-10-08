#pragma once

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.h>
#include <util/config-file.h>
#include <util/platform.h>
#include <util/dstr.h>

#include <QWidget>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QProgressBar>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>

#ifdef __cplusplus
extern "C" {
#endif

// Plugin information
#define PLUGIN_NAME "obs-ttoutput"
#define PLUGIN_VERSION "1.0.0"

// Output types (string constants for OBS)
#define OUTPUT_TYPE_RTMP_STR "rtmp_output"
#define OUTPUT_TYPE_FILE_STR "ffmpeg_muxer"

// Configuration keys
#define CONFIG_RTMP_URL "rtmp_url"
#define CONFIG_RTMP_KEY "rtmp_key"
#define CONFIG_FILE_PATH "file_path"
#define CONFIG_FILE_FORMAT "file_format"
#define CONFIG_ENCODER_CODEC "encoder_codec"
#define CONFIG_ENCODER_BITRATE "encoder_bitrate"
#define CONFIG_ENCODER_WIDTH "encoder_width"
#define CONFIG_ENCODER_HEIGHT "encoder_height"
#define CONFIG_ENCODER_FPS "encoder_fps"
#define CONFIG_ENCODER_PRESET "encoder_preset"

// Data structures
typedef enum {
    OUTPUT_STATUS_STOPPED,
    OUTPUT_STATUS_STARTING,
    OUTPUT_STATUS_ACTIVE,
    OUTPUT_STATUS_STOPPING,
    OUTPUT_STATUS_ERROR
} output_status_t;

typedef enum {
    OUTPUT_TYPE_RTMP,
    OUTPUT_TYPE_FILE
} output_type_t;

#define MAX_SOURCES 32

typedef struct {
    char name[256];
    bool enabled;
    float volume;
} source_info_t;

typedef struct {
    output_type_t output_type;
    output_status_t status;
    
    // Configuration metadata
    char *config_name;
    char *description;
    obs_data_t *settings;
    
    // RTMP settings
    char rtmp_url[512];
    char rtmp_key[512];
    
    // File settings
    char file_path[512];
    char file_format[64];
    
    // Video settings
    char video_codec[64];
    int video_bitrate;
    int video_width;
    int video_height;
    int video_fps;
    char video_preset[64];
    
    // Audio settings
    int audio_bitrate;
    int audio_samplerate;
    int audio_channels;
    
    // Sources
    source_info_t sources[MAX_SOURCES];
    int source_count;
    
    // OBS objects
    obs_output_t *output;
    obs_encoder_t *video_encoder;
    obs_encoder_t *audio_encoder;
    obs_service_t *service;
    size_t audio_mixer_idx;
    
    // Custom video mixing
    obs_view_t *custom_view;
    video_t *custom_video;
} ttoutput_config_t;

typedef struct {
    char *source_name;
    obs_source_t *source;
    float volume;
    bool enabled;
    bool is_video;
    bool is_audio;
} obs_source_info_t;

typedef struct {
    ttoutput_config_t *configs;
    size_t config_count;
    obs_source_info_t *sources;
    size_t source_count;
    char *config_dir;
    obs_data_t *global_settings;
} ttoutput_data_t;

// Global plugin data
extern ttoutput_data_t *g_ttoutput_data;

// Function declarations
bool ttoutput_init(void);
void ttoutput_cleanup(void);
void ttoutput_register_dock(void);

// Configuration management
bool ttoutput_save_config(const char *config_name, obs_data_t *config);
obs_data_t *ttoutput_load_config(const char *config_name);
char **ttoutput_get_config_list(size_t *count);
bool ttoutput_delete_config(const char *config_name);
char *ttoutput_get_config_dir(void);

// Output management
obs_output_t *ttoutput_create_output(const char *type, obs_data_t *settings);
bool ttoutput_start_output(ttoutput_config_t *config);
bool ttoutput_stop_output(ttoutput_config_t *config);
output_status_t ttoutput_get_output_status(ttoutput_config_t *config);

// Source management
obs_source_t **ttoutput_get_available_sources(size_t *count);
bool ttoutput_add_source_to_mixer(obs_source_t *source);
void ttoutput_remove_source_from_mixer(obs_source_t *source);
void ttoutput_set_source_volume(obs_source_t *source, float volume);

#ifdef __cplusplus
}
#endif
