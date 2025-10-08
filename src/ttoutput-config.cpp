#include "ttoutput-config.h"
#include "ttoutput-plugin.h"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <util/config-file.h>
#include <util/dstr.h>

#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>

// Global config data
static struct {
    bool initialized;
    char config_dir[512];
    config_t *global_config;
    pthread_mutex_t mutex;
} g_config_data = {0};

// Configuration management functions
bool ttoutput_config_init(void)
{
    if (g_config_data.initialized) {
        return true;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&g_config_data.mutex, NULL) != 0) {
        blog(LOG_ERROR, "TTOutput: Failed to initialize config mutex");
        return false;
    }
    
    // Get configuration directory
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    configPath += "/obs-ttoutput";
    
    QDir configDir(configPath);
    if (!configDir.exists()) {
        if (!configDir.mkpath(".")) {
            blog(LOG_ERROR, "TTOutput: Failed to create config directory: %s", 
                 configPath.toUtf8().constData());
            pthread_mutex_destroy(&g_config_data.mutex);
            return false;
        }
    }
    
    strncpy(g_config_data.config_dir, configPath.toUtf8().constData(), 
            sizeof(g_config_data.config_dir) - 1);
    
    // Load global configuration
    char global_config_path[512];
    snprintf(global_config_path, sizeof(global_config_path), 
             "%s/global.ini", g_config_data.config_dir);
    
    g_config_data.global_config = config_create(global_config_path);
    if (!g_config_data.global_config) {
        blog(LOG_ERROR, "TTOutput: Failed to create global config");
        pthread_mutex_destroy(&g_config_data.mutex);
        return false;
    }
    
    // Load existing config if it exists
    if (os_file_exists(global_config_path)) {
        config_open(&g_config_data.global_config, global_config_path, CONFIG_OPEN_EXISTING);
    }
    
    g_config_data.initialized = true;
    
    blog(LOG_INFO, "TTOutput: Configuration system initialized: %s", g_config_data.config_dir);
    return true;
}

char **ttoutput_config_get_list(size_t *count)
{
    if (!g_config_data.initialized) {
        if (count) *count = 0;
        return NULL;
    }
    
    pthread_mutex_lock(&g_config_data.mutex);
    
    QDir configDir(g_config_data.config_dir);
    QStringList filters;
    filters << "*.json";
    QStringList files = configDir.entryList(filters, QDir::Files);
    
    if (files.isEmpty()) {
        pthread_mutex_unlock(&g_config_data.mutex);
        if (count) *count = 0;
        return NULL;
    }
    
    // Allocate array
    char **list = (char**)bzalloc(sizeof(char*) * files.size());
    if (!list) {
        pthread_mutex_unlock(&g_config_data.mutex);
        if (count) *count = 0;
        return NULL;
    }
    
    // Copy file names (without .json extension)
    for (int i = 0; i < files.size(); i++) {
        QString baseName = QFileInfo(files[i]).baseName();
        list[i] = bstrdup(baseName.toUtf8().constData());
    }
    
    if (count) *count = files.size();
    
    pthread_mutex_unlock(&g_config_data.mutex);
    
    return list;
}

void ttoutput_config_free_list(char **list, size_t count)
{
    if (!list) {
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        if (list[i]) {
            bfree(list[i]);
        }
    }
    bfree(list);
}

void ttoutput_config_cleanup(void)
{
    if (!g_config_data.initialized) {
        return;
    }
    
    // Save global config
    if (g_config_data.global_config) {
        config_save_safe(g_config_data.global_config, "tmp", NULL);
        config_close(g_config_data.global_config);
        g_config_data.global_config = NULL;
    }
    
    pthread_mutex_destroy(&g_config_data.mutex);
    g_config_data.initialized = false;
    
    blog(LOG_INFO, "TTOutput: Configuration system cleanup completed");
}

ttoutput_config_t* ttoutput_config_create(void)
{
    ttoutput_config_t *config = (ttoutput_config_t*)bzalloc(sizeof(ttoutput_config_t));
    if (!config) {
        return NULL;
    }
    
    // Set default values
    config->output_type = OUTPUT_TYPE_RTMP;
    config->status = OUTPUT_STATUS_STOPPED;
    
    // Default RTMP settings
    strncpy(config->rtmp_url, "rtmp://live.twitch.tv/live/", sizeof(config->rtmp_url) - 1);
    config->rtmp_key[0] = '\0';
    
    // Default file settings
    strncpy(config->file_path, "", sizeof(config->file_path) - 1);
    strncpy(config->file_format, "mp4", sizeof(config->file_format) - 1);
    
    // Default video settings
    strncpy(config->video_codec, "libx264", sizeof(config->video_codec) - 1);
    config->video_bitrate = 6000;
    config->video_width = 1920;
    config->video_height = 1080;
    config->video_fps = 60;
    strncpy(config->video_preset, "veryfast", sizeof(config->video_preset) - 1);
    
    // Default audio settings
    config->audio_bitrate = 128;
    config->audio_samplerate = 44100;
    config->audio_channels = 2;
    
    // Initialize source list
    config->source_count = 0;
    
    // Initialize metadata
    config->config_name = NULL;
    config->description = NULL;
    config->settings = obs_data_create();
    
    // Initialize pointers
    config->output = NULL;
    config->video_encoder = NULL;
    config->audio_encoder = NULL;
    config->service = NULL;
    config->audio_mixer_idx = 0;
    
    return config;
}

void ttoutput_config_free(ttoutput_config_t *config)
{
    if (!config) {
        return;
    }
    
    // Clean up metadata
    if (config->config_name) {
        bfree(config->config_name);
    }
    if (config->description) {
        bfree(config->description);
    }
    if (config->settings) {
        obs_data_release(config->settings);
    }
    
    // Clean up OBS objects if they exist
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
    // Audio mixer cleanup is handled by OBS core
    
    bfree(config);
}

bool ttoutput_config_save(ttoutput_config_t *config, const char *name)
{
    if (!config || !name || !g_config_data.initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_config_data.mutex);
    
    // Create JSON object
    QJsonObject json;
    
    // Basic settings
    json["output_type"] = (int)config->output_type;
    json["rtmp_url"] = config->rtmp_url;
    json["rtmp_key"] = config->rtmp_key;
    json["file_path"] = config->file_path;
    json["file_format"] = config->file_format;
    
    // Video settings
    QJsonObject video;
    video["codec"] = config->video_codec;
    video["bitrate"] = config->video_bitrate;
    video["width"] = config->video_width;
    video["height"] = config->video_height;
    video["fps"] = config->video_fps;
    video["preset"] = config->video_preset;
    json["video"] = video;
    
    // Audio settings
    QJsonObject audio;
    audio["bitrate"] = config->audio_bitrate;
    audio["samplerate"] = config->audio_samplerate;
    audio["channels"] = config->audio_channels;
    json["audio"] = audio;
    
    // Sources
    QJsonArray sources;
    for (int i = 0; i < config->source_count; i++) {
        QJsonObject source;
        source["name"] = config->sources[i].name;
        source["enabled"] = config->sources[i].enabled;
        source["volume"] = config->sources[i].volume;
        sources.append(source);
    }
    json["sources"] = sources;
    
    // Save to file
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/%s.json", g_config_data.config_dir, name);
    
    QFile file(config_path);
    if (!file.open(QIODevice::WriteOnly)) {
        blog(LOG_ERROR, "TTOutput: Failed to open config file for writing: %s", config_path);
        pthread_mutex_unlock(&g_config_data.mutex);
        return false;
    }
    
    QJsonDocument doc(json);
    file.write(doc.toJson());
    file.close();
    
    pthread_mutex_unlock(&g_config_data.mutex);
    
    blog(LOG_INFO, "TTOutput: Configuration saved: %s", name);
    return true;
}

ttoutput_config_t* ttoutput_config_load(const char *name)
{
    if (!name || !g_config_data.initialized) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_config_data.mutex);
    
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/%s.json", g_config_data.config_dir, name);
    
    QFile file(config_path);
    if (!file.open(QIODevice::ReadOnly)) {
        blog(LOG_WARNING, "TTOutput: Failed to open config file: %s", config_path);
        pthread_mutex_unlock(&g_config_data.mutex);
        return NULL;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        blog(LOG_ERROR, "TTOutput: Failed to parse config JSON: %s", error.errorString().toUtf8().constData());
        pthread_mutex_unlock(&g_config_data.mutex);
        return NULL;
    }
    
    QJsonObject json = doc.object();
    
    // Create config
    ttoutput_config_t *config = ttoutput_config_create();
    if (!config) {
        pthread_mutex_unlock(&g_config_data.mutex);
        return NULL;
    }
    
    // Load basic settings
    config->output_type = (output_type_t)json["output_type"].toInt();
    strncpy(config->rtmp_url, json["rtmp_url"].toString().toUtf8().constData(), sizeof(config->rtmp_url) - 1);
    strncpy(config->rtmp_key, json["rtmp_key"].toString().toUtf8().constData(), sizeof(config->rtmp_key) - 1);
    strncpy(config->file_path, json["file_path"].toString().toUtf8().constData(), sizeof(config->file_path) - 1);
    strncpy(config->file_format, json["file_format"].toString().toUtf8().constData(), sizeof(config->file_format) - 1);
    
    // Load video settings
    QJsonObject video = json["video"].toObject();
    strncpy(config->video_codec, video["codec"].toString().toUtf8().constData(), sizeof(config->video_codec) - 1);
    config->video_bitrate = video["bitrate"].toInt();
    config->video_width = video["width"].toInt();
    config->video_height = video["height"].toInt();
    config->video_fps = video["fps"].toInt();
    strncpy(config->video_preset, video["preset"].toString().toUtf8().constData(), sizeof(config->video_preset) - 1);
    
    // Load audio settings
    QJsonObject audio = json["audio"].toObject();
    config->audio_bitrate = audio["bitrate"].toInt();
    config->audio_samplerate = audio["samplerate"].toInt();
    config->audio_channels = audio["channels"].toInt();
    
    // Load sources
    QJsonArray sources = json["sources"].toArray();
    config->source_count = qMin(sources.size(), MAX_SOURCES);
    for (int i = 0; i < config->source_count; i++) {
        QJsonObject source = sources[i].toObject();
        strncpy(config->sources[i].name, source["name"].toString().toUtf8().constData(), 
                sizeof(config->sources[i].name) - 1);
        config->sources[i].enabled = source["enabled"].toBool();
        config->sources[i].volume = source["volume"].toDouble();
    }
    
    pthread_mutex_unlock(&g_config_data.mutex);
    
    blog(LOG_INFO, "TTOutput: Configuration loaded: %s", name);
    return config;
}

bool ttoutput_config_delete(const char *name)
{
    if (!name || !g_config_data.initialized) {
        return false;
    }
    
    pthread_mutex_lock(&g_config_data.mutex);
    
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/%s.json", g_config_data.config_dir, name);
    
    bool success = (os_unlink(config_path) == 0);
    
    pthread_mutex_unlock(&g_config_data.mutex);
    
    if (success) {
        blog(LOG_INFO, "TTOutput: Configuration deleted: %s", name);
    } else {
        blog(LOG_WARNING, "TTOutput: Failed to delete configuration: %s", name);
    }
    
    return success;
}

char** ttoutput_config_list(void)
{
    if (!g_config_data.initialized) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_config_data.mutex);
    
    QDir configDir(g_config_data.config_dir);
    QStringList filters;
    filters << "*.json";
    QStringList files = configDir.entryList(filters, QDir::Files);
    
    if (files.isEmpty()) {
        pthread_mutex_unlock(&g_config_data.mutex);
        return NULL;
    }
    
    // Allocate array
    char **list = (char**)bzalloc(sizeof(char*) * (files.size() + 1));
    if (!list) {
        pthread_mutex_unlock(&g_config_data.mutex);
        return NULL;
    }
    
    // Copy file names (without .json extension)
    for (int i = 0; i < files.size(); i++) {
        QString baseName = QFileInfo(files[i]).baseName();
        list[i] = bstrdup(baseName.toUtf8().constData());
    }
    
    list[files.size()] = NULL; // Null terminator
    
    pthread_mutex_unlock(&g_config_data.mutex);
    
    return list;
}

void ttoutput_config_list_free(char **list)
{
    if (!list) {
        return;
    }
    
    for (int i = 0; list[i]; i++) {
        bfree(list[i]);
    }
    bfree(list);
}

const char* ttoutput_config_get_dir(void)
{
    if (!g_config_data.initialized) {
        return NULL;
    }
    
    return g_config_data.config_dir;
}

ttoutput_config_t* ttoutput_config_get_default(void)
{
    if (!g_config_data.initialized || !g_config_data.global_config) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_config_data.mutex);
    
    ttoutput_config_t *config = ttoutput_config_create();
    if (!config) {
        pthread_mutex_unlock(&g_config_data.mutex);
        return NULL;
    }
    
    // Load from global config
    config->output_type = (output_type_t)config_get_int(g_config_data.global_config, "general", "output_type");
    
    const char *rtmp_url = config_get_string(g_config_data.global_config, "rtmp", "url");
    if (rtmp_url) {
        strncpy(config->rtmp_url, rtmp_url, sizeof(config->rtmp_url) - 1);
    }
    
    const char *rtmp_key = config_get_string(g_config_data.global_config, "rtmp", "key");
    if (rtmp_key) {
        strncpy(config->rtmp_key, rtmp_key, sizeof(config->rtmp_key) - 1);
    }
    
    const char *file_path = config_get_string(g_config_data.global_config, "file", "path");
    if (file_path) {
        strncpy(config->file_path, file_path, sizeof(config->file_path) - 1);
    }
    
    const char *file_format = config_get_string(g_config_data.global_config, "file", "format");
    if (file_format) {
        strncpy(config->file_format, file_format, sizeof(config->file_format) - 1);
    }
    
    // Video settings
    const char *video_codec = config_get_string(g_config_data.global_config, "video", "codec");
    if (video_codec) {
        strncpy(config->video_codec, video_codec, sizeof(config->video_codec) - 1);
    }
    
    config->video_bitrate = config_get_int(g_config_data.global_config, "video", "bitrate");
    config->video_width = config_get_int(g_config_data.global_config, "video", "width");
    config->video_height = config_get_int(g_config_data.global_config, "video", "height");
    config->video_fps = config_get_int(g_config_data.global_config, "video", "fps");
    
    const char *video_preset = config_get_string(g_config_data.global_config, "video", "preset");
    if (video_preset) {
        strncpy(config->video_preset, video_preset, sizeof(config->video_preset) - 1);
    }
    
    // Audio settings
    config->audio_bitrate = config_get_int(g_config_data.global_config, "audio", "bitrate");
    config->audio_samplerate = config_get_int(g_config_data.global_config, "audio", "samplerate");
    config->audio_channels = config_get_int(g_config_data.global_config, "audio", "channels");
    
    pthread_mutex_unlock(&g_config_data.mutex);
    
    return config;
}

bool ttoutput_config_apply_default(ttoutput_config_t *config)
{
    if (!config || !g_config_data.initialized || !g_config_data.global_config) {
        return false;
    }
    
    pthread_mutex_lock(&g_config_data.mutex);
    
    // Save to global config
    config_set_int(g_config_data.global_config, "general", "output_type", config->output_type);
    
    config_set_string(g_config_data.global_config, "rtmp", "url", config->rtmp_url);
    config_set_string(g_config_data.global_config, "rtmp", "key", config->rtmp_key);
    
    config_set_string(g_config_data.global_config, "file", "path", config->file_path);
    config_set_string(g_config_data.global_config, "file", "format", config->file_format);
    
    config_set_string(g_config_data.global_config, "video", "codec", config->video_codec);
    config_set_int(g_config_data.global_config, "video", "bitrate", config->video_bitrate);
    config_set_int(g_config_data.global_config, "video", "width", config->video_width);
    config_set_int(g_config_data.global_config, "video", "height", config->video_height);
    config_set_int(g_config_data.global_config, "video", "fps", config->video_fps);
    config_set_string(g_config_data.global_config, "video", "preset", config->video_preset);
    
    config_set_int(g_config_data.global_config, "audio", "bitrate", config->audio_bitrate);
    config_set_int(g_config_data.global_config, "audio", "samplerate", config->audio_samplerate);
    config_set_int(g_config_data.global_config, "audio", "channels", config->audio_channels);
    
    // Save to file
    bool success = config_save_safe(g_config_data.global_config, "tmp", NULL) == CONFIG_SUCCESS;
    
    pthread_mutex_unlock(&g_config_data.mutex);
    
    if (success) {
        blog(LOG_INFO, "TTOutput: Default configuration applied");
    } else {
        blog(LOG_ERROR, "TTOutput: Failed to apply default configuration");
    }
    
    return success;
}

// Validation functions
bool ttoutput_config_validate(ttoutput_config_t *config)
{
    if (!config) {
        return false;
    }
    
    return ttoutput_config_validate_general(config) &&
           ttoutput_config_validate_rtmp(config) &&
           ttoutput_config_validate_file(config) &&
           ttoutput_config_validate_encoder(config);
}

bool ttoutput_config_validate_general(ttoutput_config_t *config)
{
    if (!config) {
        return false;
    }
    
    if (config->output_type != OUTPUT_TYPE_RTMP && config->output_type != OUTPUT_TYPE_FILE) {
        blog(LOG_ERROR, "TTOutput: Invalid output type: %d", config->output_type);
        return false;
    }
    
    return true;
}

bool ttoutput_config_validate_rtmp(ttoutput_config_t *config)
{
    if (!config || config->output_type != OUTPUT_TYPE_RTMP) {
        return true; // Not applicable
    }
    
    if (strlen(config->rtmp_url) == 0) {
        blog(LOG_ERROR, "TTOutput: RTMP URL is empty");
        return false;
    }
    
    if (strlen(config->rtmp_key) == 0) {
        blog(LOG_ERROR, "TTOutput: RTMP key is empty");
        return false;
    }
    
    return true;
}

bool ttoutput_config_validate_file(ttoutput_config_t *config)
{
    if (!config || config->output_type != OUTPUT_TYPE_FILE) {
        return true; // Not applicable
    }
    
    if (strlen(config->file_path) == 0) {
        blog(LOG_ERROR, "TTOutput: File path is empty");
        return false;
    }
    
    if (strlen(config->file_format) == 0) {
        blog(LOG_ERROR, "TTOutput: File format is empty");
        return false;
    }
    
    return true;
}

bool ttoutput_config_validate_encoder(ttoutput_config_t *config)
{
    if (!config) {
        return false;
    }
    
    // Video validation
    if (config->video_bitrate < 500 || config->video_bitrate > 50000) {
        blog(LOG_ERROR, "TTOutput: Invalid video bitrate: %d", config->video_bitrate);
        return false;
    }
    
    if (config->video_width < 320 || config->video_width > 3840) {
        blog(LOG_ERROR, "TTOutput: Invalid video width: %d", config->video_width);
        return false;
    }
    
    if (config->video_height < 240 || config->video_height > 2160) {
        blog(LOG_ERROR, "TTOutput: Invalid video height: %d", config->video_height);
        return false;
    }
    
    if (config->video_fps < 15 || config->video_fps > 120) {
        blog(LOG_ERROR, "TTOutput: Invalid video FPS: %d", config->video_fps);
        return false;
    }
    
    // Audio validation
    if (config->audio_bitrate < 64 || config->audio_bitrate > 320) {
        blog(LOG_ERROR, "TTOutput: Invalid audio bitrate: %d", config->audio_bitrate);
        return false;
    }
    
    if (config->audio_samplerate != 44100 && config->audio_samplerate != 48000) {
        blog(LOG_ERROR, "TTOutput: Invalid audio sample rate: %d", config->audio_samplerate);
        return false;
    }
    
    if (config->audio_channels < 1 || config->audio_channels > 8) {
        blog(LOG_ERROR, "TTOutput: Invalid audio channels: %d", config->audio_channels);
        return false;
    }
    
    return true;
}
