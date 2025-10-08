#include "ttoutput-dock.h"
#include "ttoutput-manager.h"
#include "ttoutput-config.h"

#include <QApplication>
#include <QStyle>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QThread>
#include <QMutexLocker>
#include <chrono>

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

TTOutputDock::TTOutputDock(QWidget *parent)
    : QWidget(parent)
    , m_currentConfig(nullptr)
    , m_statusTimer(new QTimer(this))
    , m_isOutputActive(false)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_isStarting(0)
    , m_isStopping(0)
    , m_pendingConfig(nullptr)
{
    setObjectName("TTOutputDock");
    setWindowTitle("TTOutput Settings");
    setMinimumSize(400, 600);
    
    // Setup worker thread for async operations
    m_workerThread = new QThread(this);
    m_worker = new TTOutputWorker();
    m_worker->moveToThread(m_workerThread);
    
    // Connect worker signals
    connect(m_worker, &TTOutputWorker::outputStarted, this, &TTOutputDock::onOutputStarted);
    connect(m_worker, &TTOutputWorker::outputStopped, this, &TTOutputDock::onOutputStopped);
    connect(m_worker, &TTOutputWorker::progressUpdate, this, &TTOutputDock::onProgressUpdate);
    
    // Connect thread management
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    
    m_workerThread->start();
    
    setupUI();
    loadSettings();
    
    // Setup status update timer
    connect(m_statusTimer, &QTimer::timeout, this, &TTOutputDock::onUpdateStatus);
    m_statusTimer->start(1000); // Update every second
}

TTOutputDock::~TTOutputDock()
{
    saveSettings();
    
    // Clean up worker thread
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(3000); // Wait up to 3 seconds
    }
    
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

void TTOutputDock::setupUI()
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
    
    setupOutputTab();
    setupSourceTab();
    setupControlTab();
    setupConfigTab();
    
    mainLayout->addWidget(m_tabWidget);
    
    // Connect signals
    connect(m_outputTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TTOutputDock::onOutputTypeChanged);
    connect(m_startStopButton, &QPushButton::clicked,
            this, &TTOutputDock::onStartStopClicked);
    connect(m_saveConfigButton, &QPushButton::clicked,
            this, &TTOutputDock::onSaveConfigClicked);
    connect(m_loadConfigButton, &QPushButton::clicked,
            this, &TTOutputDock::onLoadConfigClicked);
    connect(m_browseFileButton, &QPushButton::clicked,
            this, &TTOutputDock::onBrowseFileClicked);
    connect(m_refreshSourcesButton, &QPushButton::clicked,
            this, &TTOutputDock::onRefreshSourcesClicked);
}

void TTOutputDock::setupOutputTab()
{
    m_outputTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_outputTab);
    layout->setSpacing(15);
    
    // Output type selection
    QGroupBox *typeGroup = new QGroupBox("Output Type");
    typeGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFFFFF; }");
    QVBoxLayout *typeLayout = new QVBoxLayout(typeGroup);
    
    m_outputTypeCombo = new QComboBox();
    m_outputTypeCombo->addItem("RTMP Stream", "rtmp");
    m_outputTypeCombo->addItem("File Recording", "file");
    m_outputTypeCombo->setStyleSheet(
        "QComboBox { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox::down-arrow { image: none; border: none; }"
    );
    typeLayout->addWidget(m_outputTypeCombo);
    layout->addWidget(typeGroup);
    
    // RTMP Settings
    m_rtmpGroup = new QGroupBox("RTMP Settings");
    m_rtmpGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFFFFF; }");
    QFormLayout *rtmpLayout = new QFormLayout(m_rtmpGroup);
    
    m_rtmpUrlEdit = new QLineEdit();
    m_rtmpUrlEdit->setPlaceholderText("rtmp://live.twitch.tv/live/");
    m_rtmpUrlEdit->setStyleSheet(
        "QLineEdit { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    rtmpLayout->addRow("RTMP URL:", m_rtmpUrlEdit);
    
    m_rtmpKeyEdit = new QLineEdit();
    m_rtmpKeyEdit->setPlaceholderText("Your stream key");
    m_rtmpKeyEdit->setEchoMode(QLineEdit::Password);
    m_rtmpKeyEdit->setStyleSheet(
        "QLineEdit { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    rtmpLayout->addRow("Stream Key:", m_rtmpKeyEdit);
    layout->addWidget(m_rtmpGroup);
    
    // File Settings
    m_fileGroup = new QGroupBox("File Settings");
    m_fileGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFFFFF; }");
    QFormLayout *fileLayout = new QFormLayout(m_fileGroup);
    
    QHBoxLayout *filePathLayout = new QHBoxLayout();
    m_filePathEdit = new QLineEdit();
    m_filePathEdit->setPlaceholderText("Select output file path");
    m_filePathEdit->setStyleSheet(
        "QLineEdit { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    m_browseFileButton = new QPushButton("Browse");
    m_browseFileButton->setStyleSheet(
        "QPushButton { background-color: #0078D4; color: #FFFFFF; border: none; padding: 5px 15px; }"
        "QPushButton:hover { background-color: #106EBE; }"
        "QPushButton:pressed { background-color: #005A9E; }"
    );
    filePathLayout->addWidget(m_filePathEdit);
    filePathLayout->addWidget(m_browseFileButton);
    fileLayout->addRow("File Path:", filePathLayout);
    
    m_fileFormatCombo = new QComboBox();
    m_fileFormatCombo->addItem("MP4", "mp4");
    m_fileFormatCombo->addItem("MKV", "mkv");
    m_fileFormatCombo->addItem("FLV", "flv");
    m_fileFormatCombo->setStyleSheet(
        "QComboBox { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    fileLayout->addRow("Format:", m_fileFormatCombo);
    layout->addWidget(m_fileGroup);
    
    // Encoder Settings
    m_encoderGroup = new QGroupBox("Encoder Settings");
    m_encoderGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFFFFF; }");
    QFormLayout *encoderLayout = new QFormLayout(m_encoderGroup);
    
    m_codecCombo = new QComboBox();
    m_codecCombo->addItem("H.264 (x264)", "obs_x264");
    m_codecCombo->addItem("H.265 (x265)", "libx265");
    m_codecCombo->setStyleSheet(
        "QComboBox { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    encoderLayout->addRow("Video Codec:", m_codecCombo);
    
    m_bitrateSpinBox = new QSpinBox();
    m_bitrateSpinBox->setRange(500, 50000);
    m_bitrateSpinBox->setValue(6000);
    m_bitrateSpinBox->setSuffix(" kbps");
    m_bitrateSpinBox->setStyleSheet(
        "QSpinBox { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    encoderLayout->addRow("Bitrate:", m_bitrateSpinBox);
    
    QHBoxLayout *resolutionLayout = new QHBoxLayout();
    m_widthSpinBox = new QSpinBox();
    m_widthSpinBox->setRange(320, 3840);
    m_widthSpinBox->setValue(1920);
    m_widthSpinBox->setStyleSheet(
        "QSpinBox { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(240, 2160);
    m_heightSpinBox->setValue(1080);
    m_heightSpinBox->setStyleSheet(
        "QSpinBox { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    resolutionLayout->addWidget(m_widthSpinBox);
    resolutionLayout->addWidget(new QLabel("x"));
    resolutionLayout->addWidget(m_heightSpinBox);
    encoderLayout->addRow("Resolution:", resolutionLayout);
    
    m_fpsSpinBox = new QSpinBox();
    m_fpsSpinBox->setRange(15, 120);
    m_fpsSpinBox->setValue(60);
    m_fpsSpinBox->setSuffix(" fps");
    m_fpsSpinBox->setStyleSheet(
        "QSpinBox { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    encoderLayout->addRow("Frame Rate:", m_fpsSpinBox);
    
    m_presetCombo = new QComboBox();
    m_presetCombo->addItem("Ultra Fast", "ultrafast");
    m_presetCombo->addItem("Super Fast", "superfast");
    m_presetCombo->addItem("Very Fast", "veryfast");
    m_presetCombo->addItem("Faster", "faster");
    m_presetCombo->addItem("Fast", "fast");
    m_presetCombo->addItem("Medium", "medium");
    m_presetCombo->setCurrentText("Very Fast");
    m_presetCombo->setStyleSheet(
        "QComboBox { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    encoderLayout->addRow("Preset:", m_presetCombo);
    layout->addWidget(m_encoderGroup);
    
    layout->addStretch();
    m_tabWidget->addTab(m_outputTab, "Output Settings");
    
    // Initially hide file group
    m_fileGroup->setVisible(false);
}

void TTOutputDock::setupSourceTab()
{
    m_sourceTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_sourceTab);
    layout->setSpacing(15);
    
    // Source selection header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *headerLabel = new QLabel("Select Audio/Video Sources:");
    headerLabel->setStyleSheet("QLabel { font-weight: bold; color: #FFFFFF; font-size: 14px; }");
    m_refreshSourcesButton = new QPushButton("Refresh");
    m_refreshSourcesButton->setStyleSheet(
        "QPushButton { background-color: #0078D4; color: #FFFFFF; border: none; padding: 5px 15px; }"
        "QPushButton:hover { background-color: #106EBE; }"
        "QPushButton:pressed { background-color: #005A9E; }"
    );
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_refreshSourcesButton);
    layout->addLayout(headerLayout);
    
    // Source list
    m_sourceList = new QListWidget();
    m_sourceList->setSelectionMode(QAbstractItemView::MultiSelection);
    m_sourceList->setStyleSheet(
        "QListWidget { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; }"
        "QListWidget::item { padding: 8px; border-bottom: 1px solid #5A5A5F; }"
        "QListWidget::item:selected { background-color: #0078D4; }"
        "QListWidget::item:hover { background-color: #4A4A4F; }"
    );
    layout->addWidget(m_sourceList);
    
    // Selected sources info
    m_selectedSourcesLabel = new QLabel("Selected: 0 sources");
    m_selectedSourcesLabel->setStyleSheet("QLabel { color: #FFFFFF; font-size: 12px; }");
    layout->addWidget(m_selectedSourcesLabel);
    
    m_tabWidget->addTab(m_sourceTab, "Source Selection");
    
    // Update source list initially
    updateSourceList();
    
    // Connect source selection change
    connect(m_sourceList, &QListWidget::itemSelectionChanged,
            this, &TTOutputDock::onSourceSelectionChanged);
}

void TTOutputDock::setupControlTab()
{
    m_controlTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_controlTab);
    layout->setSpacing(20);
    
    // Control buttons
    QGroupBox *controlGroup = new QGroupBox("Output Control");
    controlGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFFFFF; }");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    
    m_startStopButton = new QPushButton("Start Output");
    m_startStopButton->setMinimumHeight(40);
    m_startStopButton->setStyleSheet(
        "QPushButton { background-color: #107C10; color: #FFFFFF; border: none; padding: 10px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0E6B0E; }"
        "QPushButton:pressed { background-color: #0C5A0C; }"
        "QPushButton:disabled { background-color: #5A5A5F; }"
    );
    controlLayout->addWidget(m_startStopButton);
    layout->addWidget(controlGroup);
    
    // Status display
    QGroupBox *statusGroup = new QGroupBox("Output Status");
    statusGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFFFFF; }");
    QFormLayout *statusLayout = new QFormLayout(statusGroup);
    
    m_statusLabel = new QLabel("Stopped");
    m_statusLabel->setStyleSheet("QLabel { color: #FF6B6B; font-weight: bold; }");
    statusLayout->addRow("Status:", m_statusLabel);
    
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setStyleSheet(
        "QProgressBar { background-color: #3E3E42; border: 1px solid #5A5A5F; text-align: center; }"
        "QProgressBar::chunk { background-color: #0078D4; }"
    );
    statusLayout->addRow("Progress:", m_progressBar);
    
    m_bitrateLabel = new QLabel("0 kbps");
    m_bitrateLabel->setStyleSheet("QLabel { color: #FFFFFF; }");
    statusLayout->addRow("Bitrate:", m_bitrateLabel);
    
    m_droppedFramesLabel = new QLabel("0");
    m_droppedFramesLabel->setStyleSheet("QLabel { color: #FFFFFF; }");
    statusLayout->addRow("Dropped Frames:", m_droppedFramesLabel);
    
    m_cpuUsageLabel = new QLabel("0%");
    m_cpuUsageLabel->setStyleSheet("QLabel { color: #FFFFFF; }");
    statusLayout->addRow("CPU Usage:", m_cpuUsageLabel);
    
    layout->addWidget(statusGroup);
    layout->addStretch();
    
    m_tabWidget->addTab(m_controlTab, "Control & Status");
}

void TTOutputDock::setupConfigTab()
{
    m_configTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_configTab);
    layout->setSpacing(15);
    
    // Save configuration
    QGroupBox *saveGroup = new QGroupBox("Save Configuration");
    saveGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFFFFF; }");
    QVBoxLayout *saveLayout = new QVBoxLayout(saveGroup);
    
    m_configNameEdit = new QLineEdit();
    m_configNameEdit->setPlaceholderText("Enter configuration name");
    m_configNameEdit->setStyleSheet(
        "QLineEdit { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    saveLayout->addWidget(m_configNameEdit);
    
    m_saveConfigButton = new QPushButton("Save Current Configuration");
    m_saveConfigButton->setStyleSheet(
        "QPushButton { background-color: #0078D4; color: #FFFFFF; border: none; padding: 8px; }"
        "QPushButton:hover { background-color: #106EBE; }"
        "QPushButton:pressed { background-color: #005A9E; }"
    );
    saveLayout->addWidget(m_saveConfigButton);
    layout->addWidget(saveGroup);
    
    // Load configuration
    QGroupBox *loadGroup = new QGroupBox("Load Configuration");
    loadGroup->setStyleSheet("QGroupBox { font-weight: bold; color: #FFFFFF; }");
    QVBoxLayout *loadLayout = new QVBoxLayout(loadGroup);
    
    m_configListCombo = new QComboBox();
    m_configListCombo->setStyleSheet(
        "QComboBox { background-color: #3E3E42; color: #FFFFFF; border: 1px solid #5A5A5F; padding: 5px; }"
    );
    loadLayout->addWidget(m_configListCombo);
    
    QHBoxLayout *loadButtonLayout = new QHBoxLayout();
    m_loadConfigButton = new QPushButton("Load Configuration");
    m_loadConfigButton->setStyleSheet(
        "QPushButton { background-color: #107C10; color: #FFFFFF; border: none; padding: 8px; }"
        "QPushButton:hover { background-color: #0E6B0E; }"
        "QPushButton:pressed { background-color: #0C5A0C; }"
    );
    m_deleteConfigButton = new QPushButton("Delete");
    m_deleteConfigButton->setStyleSheet(
        "QPushButton { background-color: #D13438; color: #FFFFFF; border: none; padding: 8px; }"
        "QPushButton:hover { background-color: #B02A2E; }"
        "QPushButton:pressed { background-color: #8E2125; }"
    );
    loadButtonLayout->addWidget(m_loadConfigButton);
    loadButtonLayout->addWidget(m_deleteConfigButton);
    loadLayout->addLayout(loadButtonLayout);
    layout->addWidget(loadGroup);
    
    layout->addStretch();
    m_tabWidget->addTab(m_configTab, "Configuration");
    
    // Update config list
    updateConfigList();
}

// Event handlers
void TTOutputDock::onOutputTypeChanged(int index)
{
    QString type = m_outputTypeCombo->itemData(index).toString();
    
    if (type == "rtmp") {
        m_rtmpGroup->setVisible(true);
        m_fileGroup->setVisible(false);
    } else if (type == "file") {
        m_rtmpGroup->setVisible(false);
        m_fileGroup->setVisible(true);
    }
}

void TTOutputDock::onStartStopClicked()
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
        
        // Update UI to show starting state
        m_startStopButton->setText("Starting...");
        m_startStopButton->setEnabled(false);
        m_statusLabel->setText("Starting...");
        m_statusLabel->setStyleSheet("QLabel { color: #FFA500; font-weight: bold; }");
        m_progressBar->setValue(0);
        
        // Start output asynchronously
        m_worker->startOutput(config);
        
    } else if (m_isOutputActive && m_isStopping.loadRelaxed() == 0) {
        // Set stopping state
        m_isStopping.storeRelaxed(1);
        
        // Update UI to show stopping state
        m_startStopButton->setText("Stopping...");
        m_startStopButton->setEnabled(false);
        m_statusLabel->setText("Stopping...");
        m_statusLabel->setStyleSheet("QLabel { color: #FFA500; font-weight: bold; }");
        
        // Stop output asynchronously
        ttoutput_config_t *configToStop = nullptr;
        {
            QMutexLocker locker(&m_configMutex);
            configToStop = m_currentConfig;
        }
        
        if (configToStop) {
            m_worker->stopOutput(configToStop);
        }
    }
}

void TTOutputDock::onSaveConfigClicked()
{
    QString configName = m_configNameEdit->text().trimmed();
    if (configName.isEmpty()) {
        // Show error message
        return;
    }
    
    ttoutput_config_t *config = createConfigFromUI();
    if (!config) {
        return;
    }
    
    if (ttoutput_config_save(config, configName.toUtf8().constData())) {
        m_configNameEdit->clear();
        updateConfigList();
        // Show success message
    } else {
        // Show error message
    }
    
    ttoutput_config_free(config);
}

void TTOutputDock::onLoadConfigClicked()
{
    QString configName = m_configListCombo->currentText();
    if (configName.isEmpty()) {
        return;
    }
    
    ttoutput_config_t *config = ttoutput_config_load(configName.toUtf8().constData());
    if (config) {
        applyConfigToUI(config);
        ttoutput_config_free(config);
        // Show success message
    } else {
        // Show error message
    }
}

void TTOutputDock::onDeleteConfigClicked()
{
    QString configName = m_configListCombo->currentText();
    if (configName.isEmpty()) {
        return;
    }
    
    // Show confirmation dialog
    if (ttoutput_config_delete(configName.toUtf8().constData())) {
        updateConfigList();
        // Show success message
    } else {
        // Show error message
    }
}

void TTOutputDock::onBrowseFileClicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Select Output File",
        QDir::homePath(),
        "Video Files (*.mp4 *.mkv *.flv);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        m_filePathEdit->setText(fileName);
    }
}

void TTOutputDock::onRefreshSourcesClicked()
{
    updateSourceList();
}

void TTOutputDock::onSourceSelectionChanged()
{
    QList<QListWidgetItem*> selectedItems = m_sourceList->selectedItems();
    m_selectedSourcesLabel->setText(QString("Selected: %1 sources").arg(selectedItems.count()));
}

void TTOutputDock::onUpdateStatus()
{
    if (!m_isOutputActive || !m_currentConfig) {
        return;
    }
    
    // Update status information
    uint64_t bytes_sent = ttoutput_get_bytes_sent(m_currentConfig);
    uint32_t dropped_frames = ttoutput_get_dropped_frames(m_currentConfig);
    double cpu_usage = ttoutput_get_cpu_usage(m_currentConfig);
    
    // Calculate bitrate (bytes per second to kbps)
    static uint64_t last_bytes = 0;
    static auto last_time = std::chrono::steady_clock::now();
    
    auto current_time = std::chrono::steady_clock::now();
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time).count();
    
    if (time_diff >= 1000) { // Update every second
        uint64_t bytes_diff = bytes_sent - last_bytes;
        double bitrate_kbps = (bytes_diff * 8.0) / (time_diff / 1000.0) / 1000.0;
        
        m_bitrateLabel->setText(QString("%1 kbps").arg(QString::number(bitrate_kbps, 'f', 1)));
        
        last_bytes = bytes_sent;
        last_time = current_time;
    }
    
    m_droppedFramesLabel->setText(QString::number(dropped_frames));
    m_cpuUsageLabel->setText(QString("%1%").arg(QString::number(cpu_usage, 'f', 1)));
    
    // Update progress bar based on output status
    if (m_currentConfig->output && obs_output_active(m_currentConfig->output)) {
        m_progressBar->setValue(100);
    } else {
        m_progressBar->setValue(0);
    }
}

// Utility functions
void TTOutputDock::updateSourceList()
{
    m_sourceList->clear();
    
    // Get all available sources from OBS
    obs_enum_sources([](void *param, obs_source_t *source) -> bool {
        TTOutputDock *dock = static_cast<TTOutputDock*>(param);
        
        const char *name = obs_source_get_name(source);
        const char *id = obs_source_get_id(source);
        uint32_t flags = obs_source_get_output_flags(source);
        
        // Only add sources that can output video or audio
        if ((flags & OBS_SOURCE_VIDEO) || (flags & OBS_SOURCE_AUDIO)) {
            QListWidgetItem *item = new QListWidgetItem(QString("%1 (%2)").arg(name, id));
            item->setData(Qt::UserRole, QString(name));
            dock->m_sourceList->addItem(item);
        }
        
        return true;
    }, this);
    
    onSourceSelectionChanged();
}

void TTOutputDock::updateConfigList()
{
    m_configListCombo->clear();
    
    size_t count = 0;
    char **config_list = ttoutput_config_get_list(&count);
    if (config_list) {
        for (size_t i = 0; i < count; i++) {
            m_configListCombo->addItem(config_list[i]);
        }
        ttoutput_config_free_list(config_list, count);
    }
}

bool TTOutputDock::validateSettings()
{
    QString outputType = m_outputTypeCombo->currentData().toString();
    
    if (outputType == "rtmp") {
        if (m_rtmpUrlEdit->text().trimmed().isEmpty()) {
            // Show error: RTMP URL is required
            return false;
        }
        if (m_rtmpKeyEdit->text().trimmed().isEmpty()) {
            // Show error: Stream key is required
            return false;
        }
    } else if (outputType == "file") {
        if (m_filePathEdit->text().trimmed().isEmpty()) {
            // Show error: File path is required
            return false;
        }
    }
    
    // Validate encoder settings
    if (m_bitrateSpinBox->value() < 500) {
        // Show error: Bitrate too low
        return false;
    }
    
    if (m_widthSpinBox->value() < 320 || m_heightSpinBox->value() < 240) {
        // Show error: Resolution too low
        return false;
    }
    
    return true;
}

ttoutput_config_t* TTOutputDock::createConfigFromUI()
{
    ttoutput_config_t *config = ttoutput_config_create();
    if (!config) {
        return nullptr;
    }
    
    // Basic settings
    QString outputType = m_outputTypeCombo->currentData().toString();
    config->output_type = (outputType == "rtmp") ? OUTPUT_TYPE_RTMP : OUTPUT_TYPE_FILE;
    
    // RTMP settings
    if (config->output_type == OUTPUT_TYPE_RTMP) {
        strncpy(config->rtmp_url, m_rtmpUrlEdit->text().toUtf8().constData(), sizeof(config->rtmp_url) - 1);
        strncpy(config->rtmp_key, m_rtmpKeyEdit->text().toUtf8().constData(), sizeof(config->rtmp_key) - 1);
    }
    
    // File settings
    if (config->output_type == OUTPUT_TYPE_FILE) {
        strncpy(config->file_path, m_filePathEdit->text().toUtf8().constData(), sizeof(config->file_path) - 1);
        strncpy(config->file_format, m_fileFormatCombo->currentData().toString().toUtf8().constData(), sizeof(config->file_format) - 1);
    }
    
    // Encoder settings
    strncpy(config->video_codec, m_codecCombo->currentData().toString().toUtf8().constData(), sizeof(config->video_codec) - 1);
    config->video_bitrate = m_bitrateSpinBox->value();
    config->video_width = m_widthSpinBox->value();
    config->video_height = m_heightSpinBox->value();
    config->video_fps = m_fpsSpinBox->value();
    strncpy(config->video_preset, m_presetCombo->currentData().toString().toUtf8().constData(), sizeof(config->video_preset) - 1);
    
    // Audio settings (default values)
    config->audio_bitrate = 128;
    config->audio_samplerate = 44100;
    config->audio_channels = 2;
    
    // Selected sources
    QList<QListWidgetItem*> selectedItems = m_sourceList->selectedItems();
    config->source_count = qMin(selectedItems.count(), MAX_SOURCES);
    
    for (int i = 0; i < config->source_count; i++) {
        QString sourceName = selectedItems[i]->data(Qt::UserRole).toString();
        strncpy(config->sources[i].name, sourceName.toUtf8().constData(), sizeof(config->sources[i].name) - 1);
        config->sources[i].enabled = true;
        config->sources[i].volume = 1.0f;
    }
    
    return config;
}

void TTOutputDock::applyConfigToUI(const ttoutput_config_t *config)
{
    if (!config) {
        return;
    }
    
    // Output type
    if (config->output_type == OUTPUT_TYPE_RTMP) {
        m_outputTypeCombo->setCurrentIndex(0);
        m_rtmpUrlEdit->setText(config->rtmp_url);
        m_rtmpKeyEdit->setText(config->rtmp_key);
    } else {
        m_outputTypeCombo->setCurrentIndex(1);
        m_filePathEdit->setText(config->file_path);
        
        // Find and set file format
        int formatIndex = m_fileFormatCombo->findData(config->file_format);
        if (formatIndex >= 0) {
            m_fileFormatCombo->setCurrentIndex(formatIndex);
        }
    }
    
    // Encoder settings
    int codecIndex = m_codecCombo->findData(config->video_codec);
    if (codecIndex >= 0) {
        m_codecCombo->setCurrentIndex(codecIndex);
    }
    
    m_bitrateSpinBox->setValue(config->video_bitrate);
    m_widthSpinBox->setValue(config->video_width);
    m_heightSpinBox->setValue(config->video_height);
    m_fpsSpinBox->setValue(config->video_fps);
    
    int presetIndex = m_presetCombo->findData(config->video_preset);
    if (presetIndex >= 0) {
        m_presetCombo->setCurrentIndex(presetIndex);
    }
    
    // Update UI visibility
    onOutputTypeChanged(m_outputTypeCombo->currentIndex());
    
    // Select sources
    m_sourceList->clearSelection();
    for (int i = 0; i < config->source_count; i++) {
        for (int j = 0; j < m_sourceList->count(); j++) {
            QListWidgetItem *item = m_sourceList->item(j);
            if (item->data(Qt::UserRole).toString() == config->sources[i].name) {
                item->setSelected(true);
                break;
            }
        }
    }
    
    onSourceSelectionChanged();
}

void TTOutputDock::loadSettings()
{
    // Load default configuration if available
    ttoutput_config_t *config = ttoutput_config_get_default();
    if (config) {
        applyConfigToUI(config);
        ttoutput_config_free(config);
    }
}

void TTOutputDock::saveSettings()
{
    // Save current settings as default
    ttoutput_config_t *config = createConfigFromUI();
    if (config) {
        ttoutput_config_apply_default(config);
        ttoutput_config_free(config);
    }
}

// Async operation callbacks
void TTOutputDock::onOutputStarted(bool success)
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
        m_startStopButton->setText("Stop Output");
        m_startStopButton->setStyleSheet(
            "QPushButton { background-color: #D13438; color: #FFFFFF; border: none; padding: 10px; font-size: 14px; font-weight: bold; }"
            "QPushButton:hover { background-color: #B02A2E; }"
            "QPushButton:pressed { background-color: #8E2125; }"
        );
        m_startStopButton->setEnabled(true);
        m_statusLabel->setText("Running");
        m_statusLabel->setStyleSheet("QLabel { color: #107C10; font-weight: bold; }");
    } else {
        // Clean up pending config on failure
        {
            QMutexLocker locker(&m_configMutex);
            if (m_pendingConfig) {
                ttoutput_config_free(m_pendingConfig);
                m_pendingConfig = nullptr;
            }
        }
        
        m_startStopButton->setText("Start Output");
        m_startStopButton->setStyleSheet(
            "QPushButton { background-color: #107C10; color: #FFFFFF; border: none; padding: 10px; font-size: 14px; font-weight: bold; }"
            "QPushButton:hover { background-color: #0E6B0E; }"
            "QPushButton:pressed { background-color: #0C5A0C; }"
        );
        m_startStopButton->setEnabled(true);
        m_statusLabel->setText("Failed to start");
        m_statusLabel->setStyleSheet("QLabel { color: #FF6B6B; font-weight: bold; }");
    }
}

void TTOutputDock::onOutputStopped()
{
    m_isStopping.storeRelaxed(0);
    
    // Clean up current config
    {
        QMutexLocker locker(&m_configMutex);
        if (m_currentConfig) {
            ttoutput_config_free(m_currentConfig);
            m_currentConfig = nullptr;
        }
    }
    
    m_isOutputActive = false;
    m_startStopButton->setText("Start Output");
    m_startStopButton->setStyleSheet(
        "QPushButton { background-color: #107C10; color: #FFFFFF; border: none; padding: 10px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #0E6B0E; }"
        "QPushButton:pressed { background-color: #0C5A0C; }"
    );
    m_startStopButton->setEnabled(true);
    m_statusLabel->setText("Stopped");
    m_statusLabel->setStyleSheet("QLabel { color: #FF6B6B; font-weight: bold; }");
    m_progressBar->setValue(0);
    m_bitrateLabel->setText("0 kbps");
    m_droppedFramesLabel->setText("0");
    m_cpuUsageLabel->setText("0%");
}

void TTOutputDock::onProgressUpdate(const QString &message, int progress)
{
    m_statusLabel->setText(message);
    if (progress >= 0 && progress <= 100) {
        m_progressBar->setValue(progress);
    }
}
