#include "ttoutput-main-widget.h"
#include "../ttoutput-plugin.h"
#include "ttoutput-config-tab.h"
#include "ttoutput-source-tab.h"
#include "ttoutput-status-tab.h"
#include "ttoutput-progress-dialog.h"
#include "../ttoutput-manager.h"
#include "../ttoutput-config.h"

#include <QVBoxLayout>
#include <QApplication>
#include <QMessageBox>
#include <QMutexLocker>

#include "moc_ttoutput-main-widget.cpp"

// TTOutputWorker implementation
void TTOutputWorker::startOutput(ttoutput_config_t *config)
{
    if (!config) {
        emit outputStarted(false);
        return;
    }
    
    emit progressUpdate("Initializing output...", 10);
    
    // This is the blocking operation that was causing UI freeze
    bool success = ttoutput_start_output(config);
    
    emit progressUpdate(success ? "Output started successfully" : "Failed to start output", 100);
    emit outputStarted(success);
}

void TTOutputWorker::stopOutput(ttoutput_config_t *config)
{
    if (!config) {
        emit outputStopped();
        return;
    }
    
    emit progressUpdate("Stopping output...", 50);
    
    bool success = ttoutput_stop_output(config);
    
    emit progressUpdate(success ? "Output stopped successfully" : "Failed to stop output", 100);
    emit outputStopped();
}

TTOutputMainWidget::TTOutputMainWidget(QWidget *parent)
    : QWidget(parent)
    , m_tabWidget(nullptr)
    , m_configTab(nullptr)
    , m_sourceTab(nullptr)
    , m_statusTab(nullptr)
    , m_progressDialog(nullptr)
    , m_currentConfig(nullptr)
    , m_statusTimer(new QTimer(this))
    , m_isOutputActive(false)
    , m_isDestroying(false)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_isStarting(0)
    , m_isStopping(0)
    , m_pendingConfig(nullptr)
{
    setObjectName("TTOutputMainWidget");
    setWindowTitle("TTOutput Settings");
    setMinimumSize(400, 600);
    
    setupWorkerThread();
    setupUI();
    loadSettings();
    
    // Setup status update timer
    connect(m_statusTimer, &QTimer::timeout, this, &TTOutputMainWidget::onUpdateStatus);
    m_statusTimer->start(1000); // Update every second
}

TTOutputMainWidget::~TTOutputMainWidget()
{
    m_isDestroying = true;  // Prevent saving settings during destruction
    cleanupWorkerThread();
    
    // Clean up configs
    {
        QMutexLocker locker(&m_configMutex);
        if (m_currentConfig) {
            // Stop output if active
            if (m_currentConfig->output && obs_output_active(m_currentConfig->output)) {
                ttoutput_stop_output(m_currentConfig);
            }
            ttoutput_config_free(m_currentConfig);
            m_currentConfig = nullptr;
        }
        
        if (m_pendingConfig) {
            ttoutput_config_free(m_pendingConfig);
            m_pendingConfig = nullptr;
        }
    }
}

void TTOutputMainWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #3E3E42; background-color: #2D2D30; }"
        "QTabBar::tab { background-color: #3E3E42; color: #FFFFFF; padding: 8px 16px; margin-right: 2px; }"
        "QTabBar::tab:selected { background-color: #0078D4; }"
        "QTabBar::tab:hover { background-color: #4A4A4F; }"
    );
    
    // Create tab components
    m_configTab = new TTOutputConfigTab();
    m_sourceTab = new TTOutputSourceTab();
    m_statusTab = new TTOutputStatusTab();
    
    // Add tabs to widget
    m_tabWidget->addTab(m_configTab, "Output Settings");
    m_tabWidget->addTab(m_sourceTab, "Source Selection");
    m_tabWidget->addTab(m_statusTab, "Control & Status");
    
    mainLayout->addWidget(m_tabWidget);
    
    // Connect tab signals
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &TTOutputMainWidget::onTabChanged);
    
    // Connect configuration tab signals
    connect(m_configTab, &TTOutputConfigTab::configurationChanged, 
            this, &TTOutputMainWidget::configurationChanged);
    connect(m_configTab, &TTOutputConfigTab::browseFileRequested,
            this, [this]() {
                // Handle file browse request
                // This could be moved to a separate method
            });
    
    // Connect source tab signals
    connect(m_sourceTab, &TTOutputSourceTab::sourceSelectionChanged,
            this, &TTOutputMainWidget::configurationChanged);
    
    // Connect status tab signals
    connect(m_statusTab, &TTOutputStatusTab::startStopClicked,
            this, &TTOutputMainWidget::onStartStopClicked);
    
    // Note: Auto-save removed - configuration will be saved when dock is closed
}

void TTOutputMainWidget::setupWorkerThread()
{
    // Setup worker thread for async operations
    m_workerThread = new QThread(this);
    m_worker = new TTOutputWorker();
    m_worker->moveToThread(m_workerThread);
    
    // Connect worker signals
    connect(m_worker, &TTOutputWorker::outputStarted, this, &TTOutputMainWidget::onOutputStarted);
    connect(m_worker, &TTOutputWorker::outputStopped, this, &TTOutputMainWidget::onOutputStopped);
    connect(m_worker, &TTOutputWorker::progressUpdate, this, &TTOutputMainWidget::onProgressUpdate);
    
    // Connect thread management
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    
    m_workerThread->start();
}

void TTOutputMainWidget::cleanupWorkerThread()
{
    // Clean up worker thread
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(3000); // Wait up to 3 seconds
    }
}

void TTOutputMainWidget::onStartStopClicked()
{
    if (!m_isOutputActive && m_isStarting.loadRelaxed() == 0) {
        // Validate settings before starting
        if (!validateSettings()) {
            return;
        }
        
        // Create configuration from UI
        ttoutput_config_t *config = createConfigFromUI();
        if (!config) {
            return;
        }
        
        // Set starting state
        m_isStarting.storeRelaxed(1);
        
        // Store pending config
        {
            QMutexLocker locker(&m_configMutex);
            if (m_pendingConfig) {
                ttoutput_config_free(m_pendingConfig);
            }
            m_pendingConfig = config;
        }
        
        // Update UI state
        m_statusTab->updateStatus("Starting...");
        
        // Start output asynchronously
        QMetaObject::invokeMethod(m_worker, "startOutput", Qt::QueuedConnection,
                                Q_ARG(ttoutput_config_t*, config));
        
    } else if (m_isOutputActive && m_isStopping.loadRelaxed() == 0) {
        // Set stopping state
        m_isStopping.storeRelaxed(1);
        
        // Update UI state
        m_statusTab->updateStatus("Stopping...");
        
        // Stop output asynchronously
        QMetaObject::invokeMethod(m_worker, "stopOutput", Qt::QueuedConnection,
                                Q_ARG(ttoutput_config_t*, m_currentConfig));
    }
}

void TTOutputMainWidget::onSaveConfigClicked()
{
    // Implementation for saving configuration
    // This would interact with ttoutput-config.cpp
}

void TTOutputMainWidget::onLoadConfigClicked()
{
    // Implementation for loading configuration
    // This would interact with ttoutput-config.cpp
}

void TTOutputMainWidget::onDeleteConfigClicked()
{
    // Implementation for deleting configuration
    // This would interact with ttoutput-config.cpp
}

void TTOutputMainWidget::onUpdateStatus()
{
    if (m_isOutputActive && m_currentConfig) {
        // Update status information
        m_statusTab->updateStatus("Output Active");
    } else {
        m_statusTab->updateStatus("Output Inactive");
    }
}

void TTOutputMainWidget::onOutputStarted(bool success)
{
    m_isStarting.storeRelaxed(0);
    
    if (success) {
        // Move pending config to current config
        {
            QMutexLocker locker(&m_configMutex);
            if (m_currentConfig) {
                ttoutput_config_free(m_currentConfig);
            }
            m_currentConfig = m_pendingConfig;
            m_pendingConfig = nullptr;
        }
        
        m_isOutputActive = true;
        m_statusTab->setOutputActive(true);
        emit outputStatusChanged(true);
    } else {
        // Clean up pending config
        {
            QMutexLocker locker(&m_configMutex);
            if (m_pendingConfig) {
                ttoutput_config_free(m_pendingConfig);
                m_pendingConfig = nullptr;
            }
        }
        
        m_statusTab->updateStatus("Ready");
        QMessageBox::warning(this, "Error", "Failed to start output. Please check your settings.");
    }
}

void TTOutputMainWidget::onOutputStopped()
{
    m_isStopping.storeRelaxed(0);
    m_isOutputActive = false;
    m_statusTab->setOutputActive(false);
    emit outputStatusChanged(false);
}

void TTOutputMainWidget::onProgressUpdate(const QString &message, int progress)
{
    if (!m_progressDialog) {
        m_progressDialog = new TTOutputProgressDialog(this);
    }
    
    m_progressDialog->setMessage(message);
    m_progressDialog->setProgress(progress);
    
    if (progress >= 100) {
        m_progressDialog->hide();
    } else if (!m_progressDialog->isVisible()) {
        m_progressDialog->show();
    }
}

void TTOutputMainWidget::onTabChanged(int index)
{
    // Handle tab change if needed
    Q_UNUSED(index)
}

void TTOutputMainWidget::loadSettings()
{
    blog(LOG_INFO, "TTOutput: Starting to load settings...");
    
    // Set loading flag to prevent any save operations during loading
    m_isLoadingSettings = true;
    blog(LOG_INFO, "TTOutput: Loading flag set to true, save operations will be blocked");
    
    // 临时断开配置变更信号，防止在加载期间触发保存
    disconnect(this, &TTOutputMainWidget::configurationChanged,
               this, &TTOutputMainWidget::saveSettings);
    
    // 从全局配置加载默认设置并应用到UI
    ttoutput_config_t *config = ttoutput_config_get_default();
    if (!config) {
        // 如果没有默认配置，保持当前UI默认值
        blog(LOG_WARNING, "TTOutput: No configuration loaded, keeping current UI defaults");
        // Reset loading flag before returning
        m_isLoadingSettings = false;
        blog(LOG_INFO, "TTOutput: Loading flag reset to false");
        // 重新连接信号
        connect(this, &TTOutputMainWidget::configurationChanged,
                this, &TTOutputMainWidget::saveSettings);
        return;
    }

    blog(LOG_INFO, "TTOutput: Configuration loaded successfully, applying to UI...");
    applyConfigToUI(config);

    // 释放临时配置对象
    ttoutput_config_free(config);
    
    // Reset loading flag after successful loading
    m_isLoadingSettings = false;
    blog(LOG_INFO, "TTOutput: Loading flag reset to false after successful loading");
    
    // 重新连接配置变更信号
    connect(this, &TTOutputMainWidget::configurationChanged,
            this, &TTOutputMainWidget::saveSettings);
    
    blog(LOG_INFO, "TTOutput: Settings loaded and applied to UI successfully");
}

void TTOutputMainWidget::saveSettings()
{
    // Skip saving settings during destruction to prevent crashes
    if (m_isDestroying) {
        blog(LOG_INFO, "TTOutput: Skipping save settings during destruction");
        return;
    }
    
    // Skip saving settings during loading to prevent overwriting config
    if (m_isLoadingSettings) {
        blog(LOG_INFO, "TTOutput: Skipping save settings during loading process");
        return;
    }
    
    blog(LOG_INFO, "TTOutput: Starting to save settings...");
    
    // 将当前UI中的设置保存为全局默认配置
    ttoutput_config_t *config = createConfigFromUI();
    if (!config) {
        blog(LOG_ERROR, "TTOutput: Failed to create config from UI");
        return;
    }

    blog(LOG_INFO, "TTOutput: Config created from UI, applying as default...");
    bool success = ttoutput_config_apply_default(config);

    ttoutput_config_free(config);
    
    if (success) {
        blog(LOG_INFO, "TTOutput: Settings saved successfully");
    } else {
        blog(LOG_ERROR, "TTOutput: Failed to save settings");
    }
}

bool TTOutputMainWidget::validateSettings()
{
    if (!m_configTab->validateConfig()) {
        QMessageBox::warning(this, "Invalid Configuration", 
                           "Please check your output configuration settings.");
        m_tabWidget->setCurrentWidget(m_configTab);
        return false;
    }
    
    if (!m_sourceTab->hasSelectedSources()) {
        QMessageBox::warning(this, "No Sources Selected", 
                           "Please select at least one audio or video source.");
        m_tabWidget->setCurrentWidget(m_sourceTab);
        return false;
    }
    
    return true;
}

ttoutput_config_t* TTOutputMainWidget::createConfigFromUI()
{
    ttoutput_config_t *config = ttoutput_config_create();
    if (!config) {
        return nullptr;
    }
    
    // Fill configuration from UI components
    m_configTab->fillConfig(config);
    m_sourceTab->fillConfig(config);
    
    return config;
}

void TTOutputMainWidget::applyConfigToUI(const ttoutput_config_t *config)
{
    if (!config) {
        blog(LOG_WARNING, "TTOutput: Cannot apply NULL config to UI");
        return;
    }
    
    blog(LOG_INFO, "TTOutput: Applying configuration to UI components...");
    blog(LOG_INFO, "TTOutput: Config details - Output type: %d, Video: %dx%d@%dfps %dkbps, Audio: %dkbps", 
         config->output_type, config->video_width, config->video_height, config->video_fps, 
         config->video_bitrate, config->audio_bitrate);
    
    // Apply configuration to UI components
    m_configTab->applyConfig(config);
    m_sourceTab->applyConfig(config);
    
    blog(LOG_INFO, "TTOutput: Configuration applied to UI components successfully");
}
