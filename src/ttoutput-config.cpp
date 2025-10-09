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
    
    blog(LOG_INFO, "TTOutput: Config directory path: %s", configPath.toUtf8().constData());
    
    QDir configDir(configPath);
    if (!configDir.exists()) {
        blog(LOG_INFO, "TTOutput: Config directory does not exist, creating...");
        if (!configDir.mkpath(".")) {
            blog(LOG_ERROR, "TTOutput: Failed to create config directory: %s", 
                 configPath.toUtf8().constData());
            pthread_mutex_destroy(&g_config_data.mutex);
            return false;
        }
        blog(LOG_INFO, "TTOutput: Config directory created successfully");
    } else {
        blog(LOG_INFO, "TTOutput: Config directory already exists");
    }
    
    strncpy(g_config_data.config_dir, configPath.toUtf8().constData(), 
            sizeof(g_config_data.config_dir) - 1);
    
    // Load global configuration
    char global_config_path[512];
    snprintf(global_config_path, sizeof(global_config_path), 
             "%s/global.ini", g_config_data.config_dir);
    
    blog(LOG_INFO, "TTOutput: Initializing config system with path: %s", global_config_path);
    
    g_config_data.global_config = config_create(global_config_path);
    if (!g_config_data.global_config) {
        blog(LOG_ERROR, "TTOutput: Failed to create global config at: %s", global_config_path);
        pthread_mutex_destroy(&g_config_data.mutex);
        return false;
    }
    
    // Load existing config if it exists
    bool file_exists = os_file_exists(global_config_path);
    blog(LOG_INFO, "TTOutput: Config file exists: %s", file_exists ? "YES" : "NO");
    
    if (file_exists) {
        int result = config_open(&g_config_data.global_config, global_config_path, CONFIG_OPEN_EXISTING);
        if (result == CONFIG_SUCCESS) {
            blog(LOG_INFO, "TTOutput: Successfully loaded existing config file");
        } else {
            blog(LOG_WARNING, "TTOutput: Failed to load existing config file (error: %d), will use defaults", result);
        }
    } else {
        blog(LOG_INFO, "TTOutput: No existing config file found, will create new one when saving");
    }
    
    g_config_data.initialized = true;
    
    blog(LOG_INFO, "TTOutput: Configuration system initialized successfully: %s", g_config_data.config_dir);
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
    strncpy(config->video_codec, "obs_x264", sizeof(config->video_codec) - 1);
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
        blog(LOG_ERROR, "TTOutput: Cannot save config - invalid parameters or uninitialized system");
        return false;
    }
    
    // Log the config file path
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/%s.json", g_config_data.config_dir, name);
    blog(LOG_INFO, "TTOutput: Saving JSON configuration to: %s", config_path);
    
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
    
    // Save to file (reuse the path already logged above)
    QFile file(config_path);
    if (!file.open(QIODevice::WriteOnly)) {
        blog(LOG_ERROR, "TTOutput: Failed to open config file for writing: %s", config_path);
        pthread_mutex_unlock(&g_config_data.mutex);
        return false;
    }
    
    QJsonDocument doc(json);
    qint64 bytesWritten = file.write(doc.toJson());
    file.close();
    
    pthread_mutex_unlock(&g_config_data.mutex);
    
    if (bytesWritten > 0) {
        blog(LOG_INFO, "TTOutput: JSON configuration saved successfully to: %s (%lld bytes)", config_path, bytesWritten);
    } else {
        blog(LOG_ERROR, "TTOutput: Failed to write JSON configuration to: %s", config_path);
        return false;
    }
    return true;
}

ttoutput_config_t* ttoutput_config_load(const char *name)
{
    if (!name || !g_config_data.initialized) {
        blog(LOG_ERROR, "TTOutput: Cannot load config - invalid name or uninitialized system");
        return NULL;
    }
    
    char config_path[512];
    snprintf(config_path, sizeof(config_path), "%s/%s.json", g_config_data.config_dir, name);
    blog(LOG_INFO, "TTOutput: Loading JSON configuration from: %s", config_path);
    
    pthread_mutex_lock(&g_config_data.mutex);
    
    QFile file(config_path);
    if (!file.open(QIODevice::ReadOnly)) {
        blog(LOG_WARNING, "TTOutput: Failed to open JSON config file: %s", config_path);
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
    
    blog(LOG_INFO, "TTOutput: JSON configuration loaded successfully from: %s", config_path);
    blog(LOG_INFO, "TTOutput: Loaded config - Output type: %d, Video: %dx%d@%dfps, Sources: %d", 
         config->output_type, config->video_width, config->video_height, config->video_fps, config->source_count);
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
        blog(LOG_ERROR, "TTOutput: Config system not initialized or global config is NULL");
        return NULL;
    }
    
    pthread_mutex_lock(&g_config_data.mutex);
    
    // Log the config file path
    char global_config_path[512];
    snprintf(global_config_path, sizeof(global_config_path), 
             "%s/global.ini", g_config_data.config_dir);
    
    blog(LOG_INFO, "TTOutput: Loading configuration from: %s", global_config_path);
    
    // Check if config file exists
    bool file_exists = os_file_exists(global_config_path);
    blog(LOG_INFO, "TTOutput: Config file exists: %s", file_exists ? "YES" : "NO");
    
    // Debug: Show actual config file content
    if (file_exists) {
        FILE *debug_file = fopen(global_config_path, "r");
        if (debug_file) {
            blog(LOG_INFO, "TTOutput: === Config file content ===");
            char line[256];
            int line_num = 1;
            while (fgets(line, sizeof(line), debug_file) && line_num <= 20) {
                // Remove newline for cleaner logging
                size_t len = strlen(line);
                if (len > 0 && line[len-1] == '\n') {
                    line[len-1] = '\0';
                }
                blog(LOG_INFO, "TTOutput: Line %d: %s", line_num, line);
                line_num++;
            }
            blog(LOG_INFO, "TTOutput: === End config file content ===");
            fclose(debug_file);
        }
    }
    
    ttoutput_config_t *config = ttoutput_config_create();
    if (!config) {
        blog(LOG_ERROR, "TTOutput: Failed to create config object");
        pthread_mutex_unlock(&g_config_data.mutex);
        return NULL;
    }
    
    // Load from global config - only override defaults if values exist in config
    blog(LOG_INFO, "TTOutput: Checking for config key: [general] output_type");
    if (config_has_user_value(g_config_data.global_config, "general", "output_type")) {
        config->output_type = (output_type_t)config_get_int(g_config_data.global_config, "general", "output_type");
        blog(LOG_INFO, "TTOutput: Loaded output_type from config: %d", config->output_type);
    } else {
        blog(LOG_INFO, "TTOutput: Key [general] output_type not found, using default: %d", config->output_type);
    }
    
    blog(LOG_INFO, "TTOutput: Checking for config key: [rtmp] url");
    if (config_has_user_value(g_config_data.global_config, "rtmp", "url")) {
        const char *rtmp_url = config_get_string(g_config_data.global_config, "rtmp", "url");
        if (rtmp_url) {
            strncpy(config->rtmp_url, rtmp_url, sizeof(config->rtmp_url) - 1);
            blog(LOG_INFO, "TTOutput: Loaded RTMP URL from config: %s", rtmp_url);
        }
    } else {
        blog(LOG_INFO, "TTOutput: Key [rtmp] url not found, using default: %s", config->rtmp_url);
    }
    
    if (config_has_user_value(g_config_data.global_config, "rtmp", "key")) {
        const char *rtmp_key = config_get_string(g_config_data.global_config, "rtmp", "key");
        if (rtmp_key) {
            strncpy(config->rtmp_key, rtmp_key, sizeof(config->rtmp_key) - 1);
            blog(LOG_INFO, "TTOutput: Loaded RTMP key from config (length: %zu)", strlen(rtmp_key));
        }
    } else {
        blog(LOG_INFO, "TTOutput: Using default RTMP key (empty)");
    }
    
    const char *file_path = config_get_string(g_config_data.global_config, "file", "path");
    if (file_path) {
        strncpy(config->file_path, file_path, sizeof(config->file_path) - 1);
        blog(LOG_INFO, "TTOutput: Loaded file path: %s", file_path);
    } else {
        blog(LOG_INFO, "TTOutput: No file path found in config, using default");
    }
    
    const char *file_format = config_get_string(g_config_data.global_config, "file", "format");
    if (file_format) {
        strncpy(config->file_format, file_format, sizeof(config->file_format) - 1);
        blog(LOG_INFO, "TTOutput: Loaded file format: %s", file_format);
    } else {
        blog(LOG_INFO, "TTOutput: No file format found in config, using default");
    }
    
    // Video settings
    const char *video_codec = config_get_string(g_config_data.global_config, "video", "codec");
    if (video_codec) {
        strncpy(config->video_codec, video_codec, sizeof(config->video_codec) - 1);
        blog(LOG_INFO, "TTOutput: Loaded video codec: %s", video_codec);
    } else {
        blog(LOG_INFO, "TTOutput: No video codec found in config, using default");
    }
    
    if (config_has_user_value(g_config_data.global_config, "video", "bitrate")) {
        config->video_bitrate = config_get_int(g_config_data.global_config, "video", "bitrate");
    }
    if (config_has_user_value(g_config_data.global_config, "video", "width")) {
        config->video_width = config_get_int(g_config_data.global_config, "video", "width");
    }
    if (config_has_user_value(g_config_data.global_config, "video", "height")) {
        config->video_height = config_get_int(g_config_data.global_config, "video", "height");
    }
    if (config_has_user_value(g_config_data.global_config, "video", "fps")) {
        config->video_fps = config_get_int(g_config_data.global_config, "video", "fps");
    }
    blog(LOG_INFO, "TTOutput: Video settings - bitrate: %d, resolution: %dx%d, fps: %d", 
         config->video_bitrate, config->video_width, config->video_height, config->video_fps);
    
    const char *video_preset = config_get_string(g_config_data.global_config, "video", "preset");
    if (video_preset) {
        strncpy(config->video_preset, video_preset, sizeof(config->video_preset) - 1);
        blog(LOG_INFO, "TTOutput: Loaded video preset: %s", video_preset);
    } else {
        blog(LOG_INFO, "TTOutput: No video preset found in config, using default");
    }
    
    // Audio settings
    if (config_has_user_value(g_config_data.global_config, "audio", "bitrate")) {
        config->audio_bitrate = config_get_int(g_config_data.global_config, "audio", "bitrate");
    }
    if (config_has_user_value(g_config_data.global_config, "audio", "samplerate")) {
        config->audio_samplerate = config_get_int(g_config_data.global_config, "audio", "samplerate");
    }
    if (config_has_user_value(g_config_data.global_config, "audio", "channels")) {
        config->audio_channels = config_get_int(g_config_data.global_config, "audio", "channels");
    }
    blog(LOG_INFO, "TTOutput: Audio settings - bitrate: %d, samplerate: %d, channels: %d", 
         config->audio_bitrate, config->audio_samplerate, config->audio_channels);
    
    pthread_mutex_unlock(&g_config_data.mutex);
    
    blog(LOG_INFO, "TTOutput: Configuration loaded successfully from: %s", global_config_path);
    return config;
}

bool ttoutput_config_apply_default(ttoutput_config_t *config)
{
    if (!config || !g_config_data.initialized || !g_config_data.global_config) {
        blog(LOG_ERROR, "TTOutput: Cannot apply config - invalid parameters or uninitialized system");
        return false;
    }
    
    // Log the config file path
    char global_config_path[512];
    snprintf(global_config_path, sizeof(global_config_path), 
             "%s/global.ini", g_config_data.config_dir);
    
    blog(LOG_INFO, "TTOutput: Saving configuration to: %s", global_config_path);
    
    pthread_mutex_lock(&g_config_data.mutex);
    
    // Save to global config
    config_set_int(g_config_data.global_config, "general", "output_type", config->output_type);
    blog(LOG_INFO, "TTOutput: Saving output_type: %d", config->output_type);
    
    config_set_string(g_config_data.global_config, "rtmp", "url", config->rtmp_url);
    config_set_string(g_config_data.global_config, "rtmp", "key", config->rtmp_key);
    blog(LOG_INFO, "TTOutput: Saving RTMP settings - URL: %s, Key length: %zu", 
         config->rtmp_url, strlen(config->rtmp_key));
    
    config_set_string(g_config_data.global_config, "file", "path", config->file_path);
    config_set_string(g_config_data.global_config, "file", "format", config->file_format);
    blog(LOG_INFO, "TTOutput: Saving file settings - Path: %s, Format: %s", 
         config->file_path, config->file_format);
    
    config_set_string(g_config_data.global_config, "video", "codec", config->video_codec);
    config_set_int(g_config_data.global_config, "video", "bitrate", config->video_bitrate);
    config_set_int(g_config_data.global_config, "video", "width", config->video_width);
    config_set_int(g_config_data.global_config, "video", "height", config->video_height);
    config_set_int(g_config_data.global_config, "video", "fps", config->video_fps);
    config_set_string(g_config_data.global_config, "video", "preset", config->video_preset);
    blog(LOG_INFO, "TTOutput: Saving video settings - Codec: %s, Bitrate: %d, Resolution: %dx%d, FPS: %d, Preset: %s", 
         config->video_codec, config->video_bitrate, config->video_width, config->video_height, 
         config->video_fps, config->video_preset);
    
    config_set_int(g_config_data.global_config, "audio", "bitrate", config->audio_bitrate);
    config_set_int(g_config_data.global_config, "audio", "samplerate", config->audio_samplerate);
    config_set_int(g_config_data.global_config, "audio", "channels", config->audio_channels);
    blog(LOG_INFO, "TTOutput: Saving audio settings - Bitrate: %d, Samplerate: %d, Channels: %d", 
         config->audio_bitrate, config->audio_samplerate, config->audio_channels);
    
    // Save to file
    blog(LOG_INFO, "TTOutput: Attempting to save config file...");
    
    // Check if directory is writable
    QFileInfo dirInfo(g_config_data.config_dir);
    if (!dirInfo.isWritable()) {
        blog(LOG_ERROR, "TTOutput: Config directory is not writable: %s", g_config_data.config_dir);
        pthread_mutex_unlock(&g_config_data.mutex);
        return false;
    }
    
    // Check if config file exists and is writable
    QFileInfo fileInfo(global_config_path);
    if (fileInfo.exists() && !fileInfo.isWritable()) {
        blog(LOG_ERROR, "TTOutput: Config file exists but is not writable: %s", global_config_path);
        pthread_mutex_unlock(&g_config_data.mutex);
        return false;
    }
    
    int result = config_save_safe(g_config_data.global_config, "tmp", NULL);
    bool success = (result == CONFIG_SUCCESS);
    
    pthread_mutex_unlock(&g_config_data.mutex);
    
    if (success) {
        blog(LOG_INFO, "TTOutput: Configuration saved successfully to: %s", global_config_path);
        
        // Verify the file was actually created/updated
        QFileInfo verifyInfo(global_config_path);
        if (verifyInfo.exists()) {
            blog(LOG_INFO, "TTOutput: Config file verified - Size: %lld bytes, Last modified: %s", 
                 verifyInfo.size(), verifyInfo.lastModified().toString().toUtf8().constData());
        } else {
            blog(LOG_WARNING, "TTOutput: Config file was not created despite successful save operation");
        }
    } else {
        blog(LOG_ERROR, "TTOutput: Failed to save configuration to: %s (error code: %d)", global_config_path, result);
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
