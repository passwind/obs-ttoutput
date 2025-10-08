#include "ttoutput-config-tab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>

#include "moc_ttoutput-config-tab.cpp"

TTOutputConfigTab::TTOutputConfigTab(QWidget *parent)
    : QWidget(parent)
    , m_outputTypeCombo(nullptr)
    , m_rtmpGroup(nullptr)
    , m_rtmpUrlEdit(nullptr)
    , m_rtmpKeyEdit(nullptr)
    , m_fileGroup(nullptr)
    , m_filePathEdit(nullptr)
    , m_browseFileButton(nullptr)
    , m_fileFormatCombo(nullptr)
    , m_encoderGroup(nullptr)
    , m_codecCombo(nullptr)
    , m_bitrateSpinBox(nullptr)
    , m_widthSpinBox(nullptr)
    , m_heightSpinBox(nullptr)
    , m_fpsSpinBox(nullptr)
    , m_presetCombo(nullptr)
{
    setupUI();
}

void TTOutputConfigTab::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    
    setupOutputTypeSection();
    setupRTMPSection();
    setupFileSection();
    setupEncoderSection();
    
    layout->addWidget(m_outputTypeCombo->parentWidget());
    layout->addWidget(m_rtmpGroup);
    layout->addWidget(m_fileGroup);
    layout->addWidget(m_encoderGroup);
    layout->addStretch();
    
    // Connect signals
    connect(m_outputTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TTOutputConfigTab::onOutputTypeChanged);
    connect(m_browseFileButton, &QPushButton::clicked,
            this, &TTOutputConfigTab::onBrowseFileClicked);
    
    // Connect all setting change signals
    connect(m_outputTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TTOutputConfigTab::onSettingChanged);
    connect(m_rtmpUrlEdit, &QLineEdit::textChanged, this, &TTOutputConfigTab::onSettingChanged);
    connect(m_rtmpKeyEdit, &QLineEdit::textChanged, this, &TTOutputConfigTab::onSettingChanged);
    connect(m_filePathEdit, &QLineEdit::textChanged, this, &TTOutputConfigTab::onSettingChanged);
    connect(m_fileFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TTOutputConfigTab::onSettingChanged);
    connect(m_codecCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TTOutputConfigTab::onSettingChanged);
    connect(m_bitrateSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TTOutputConfigTab::onSettingChanged);
    connect(m_widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TTOutputConfigTab::onSettingChanged);
    connect(m_heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TTOutputConfigTab::onSettingChanged);
    connect(m_fpsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &TTOutputConfigTab::onSettingChanged);
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TTOutputConfigTab::onSettingChanged);
    
    // Initially hide file group
    updateVisibility();
}

void TTOutputConfigTab::setupOutputTypeSection()
{
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
}

void TTOutputConfigTab::setupRTMPSection()
{
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
}

void TTOutputConfigTab::setupFileSection()
{
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
}

void TTOutputConfigTab::setupEncoderSection()
{
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
}

void TTOutputConfigTab::onOutputTypeChanged(int index)
{
    Q_UNUSED(index)
    updateVisibility();
    emit configurationChanged();
}

void TTOutputConfigTab::onBrowseFileClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Select Output File", QDir::homePath(),
        "Video Files (*.mp4 *.mkv *.flv);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        m_filePathEdit->setText(fileName);
        
        // Auto-detect format from extension
        if (fileName.endsWith(".mp4", Qt::CaseInsensitive)) {
            m_fileFormatCombo->setCurrentText("MP4");
        } else if (fileName.endsWith(".mkv", Qt::CaseInsensitive)) {
            m_fileFormatCombo->setCurrentText("MKV");
        } else if (fileName.endsWith(".flv", Qt::CaseInsensitive)) {
            m_fileFormatCombo->setCurrentText("FLV");
        }
        
        emit browseFileRequested();
    }
}

void TTOutputConfigTab::onSettingChanged()
{
    emit configurationChanged();
}

void TTOutputConfigTab::updateVisibility()
{
    QString type = m_outputTypeCombo->currentData().toString();
    
    if (type == "rtmp") {
        m_rtmpGroup->setVisible(true);
        m_fileGroup->setVisible(false);
    } else if (type == "file") {
        m_rtmpGroup->setVisible(false);
        m_fileGroup->setVisible(true);
    }
}

void TTOutputConfigTab::applyConfig(const ttoutput_config_t *config)
{
    if (!config) {
        resetToDefaults();
        return;
    }
    
    // Apply output type
    if (config->output_type == OUTPUT_TYPE_RTMP) {
        m_outputTypeCombo->setCurrentText("RTMP Stream");
        if (config->rtmp_url[0] != '\0') {
            m_rtmpUrlEdit->setText(QString::fromUtf8(config->rtmp_url));
        }
        if (config->rtmp_key[0] != '\0') {
            m_rtmpKeyEdit->setText(QString::fromUtf8(config->rtmp_key));
        }
    } else if (config->output_type == OUTPUT_TYPE_FILE) {
        m_outputTypeCombo->setCurrentText("File Recording");
        if (config->file_path[0] != '\0') {
            m_filePathEdit->setText(QString::fromUtf8(config->file_path));
        }
        if (config->file_format[0] != '\0') {
            QString format = QString::fromUtf8(config->file_format);
            if (format == "mp4") m_fileFormatCombo->setCurrentText("MP4");
            else if (format == "mkv") m_fileFormatCombo->setCurrentText("MKV");
            else if (format == "flv") m_fileFormatCombo->setCurrentText("FLV");
        }
    }
    
    // Apply encoder settings
    if (config->video_codec[0] != '\0') {
        QString codec = QString::fromUtf8(config->video_codec);
        if (codec == "obs_x264") m_codecCombo->setCurrentText("H.264 (x264)");
        else if (codec == "libx265") m_codecCombo->setCurrentText("H.265 (x265)");
    }
    
    m_bitrateSpinBox->setValue(config->video_bitrate);
    m_widthSpinBox->setValue(config->video_width);
    m_heightSpinBox->setValue(config->video_height);
    m_fpsSpinBox->setValue(config->video_fps);
    
    if (config->video_preset[0] != '\0') {
        QString preset = QString::fromUtf8(config->video_preset);
        for (int i = 0; i < m_presetCombo->count(); ++i) {
            if (m_presetCombo->itemData(i).toString() == preset) {
                m_presetCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    
    updateVisibility();
}

void TTOutputConfigTab::fillConfig(ttoutput_config_t *config) const
{
    if (!config) {
        return;
    }
    
    // Set output type
    QString type = m_outputTypeCombo->currentData().toString();
    if (type == "rtmp") {
        config->output_type = OUTPUT_TYPE_RTMP;
        
        QString url = m_rtmpUrlEdit->text();
        if (!url.isEmpty()) {
            strncpy(config->rtmp_url, url.toUtf8().constData(), sizeof(config->rtmp_url) - 1);
            config->rtmp_url[sizeof(config->rtmp_url) - 1] = '\0';
        }
        
        QString key = m_rtmpKeyEdit->text();
        if (!key.isEmpty()) {
            strncpy(config->rtmp_key, key.toUtf8().constData(), sizeof(config->rtmp_key) - 1);
            config->rtmp_key[sizeof(config->rtmp_key) - 1] = '\0';
        }
    } else if (type == "file") {
        config->output_type = OUTPUT_TYPE_FILE;
        
        QString path = m_filePathEdit->text();
        if (!path.isEmpty()) {
            strncpy(config->file_path, path.toUtf8().constData(), sizeof(config->file_path) - 1);
            config->file_path[sizeof(config->file_path) - 1] = '\0';
        }
        
        QString format = m_fileFormatCombo->currentData().toString();
        if (!format.isEmpty()) {
            strncpy(config->file_format, format.toUtf8().constData(), sizeof(config->file_format) - 1);
            config->file_format[sizeof(config->file_format) - 1] = '\0';
        }
    }
    
    // Set encoder settings
    QString codec = m_codecCombo->currentData().toString();
    if (!codec.isEmpty()) {
        strncpy(config->video_codec, codec.toUtf8().constData(), sizeof(config->video_codec) - 1);
        config->video_codec[sizeof(config->video_codec) - 1] = '\0';
    }
    
    config->video_bitrate = m_bitrateSpinBox->value();
    config->video_width = m_widthSpinBox->value();
    config->video_height = m_heightSpinBox->value();
    config->video_fps = m_fpsSpinBox->value();
    
    QString preset = m_presetCombo->currentData().toString();
    if (!preset.isEmpty()) {
        strncpy(config->video_preset, preset.toUtf8().constData(), sizeof(config->video_preset) - 1);
        config->video_preset[sizeof(config->video_preset) - 1] = '\0';
    }
}

bool TTOutputConfigTab::validateConfig() const
{
    QString type = m_outputTypeCombo->currentData().toString();
    
    if (type == "rtmp") {
        if (m_rtmpUrlEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(const_cast<TTOutputConfigTab*>(this), 
                               "Invalid Configuration", "RTMP URL cannot be empty.");
            return false;
        }
        if (m_rtmpKeyEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(const_cast<TTOutputConfigTab*>(this), 
                               "Invalid Configuration", "Stream key cannot be empty.");
            return false;
        }
    } else if (type == "file") {
        if (m_filePathEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(const_cast<TTOutputConfigTab*>(this), 
                               "Invalid Configuration", "Output file path cannot be empty.");
            return false;
        }
    }
    
    // Validate encoder settings
    if (m_bitrateSpinBox->value() < 500) {
        QMessageBox::warning(const_cast<TTOutputConfigTab*>(this), 
                           "Invalid Configuration", "Bitrate must be at least 500 kbps.");
        return false;
    }
    
    if (m_widthSpinBox->value() < 320 || m_heightSpinBox->value() < 240) {
        QMessageBox::warning(const_cast<TTOutputConfigTab*>(this), 
                           "Invalid Configuration", "Resolution must be at least 320x240.");
        return false;
    }
    
    return true;
}

void TTOutputConfigTab::resetToDefaults()
{
    m_outputTypeCombo->setCurrentIndex(0);
    m_rtmpUrlEdit->clear();
    m_rtmpKeyEdit->clear();
    m_filePathEdit->clear();
    m_fileFormatCombo->setCurrentIndex(0);
    m_codecCombo->setCurrentIndex(0);
    m_bitrateSpinBox->setValue(6000);
    m_widthSpinBox->setValue(1920);
    m_heightSpinBox->setValue(1080);
    m_fpsSpinBox->setValue(60);
    m_presetCombo->setCurrentText("Very Fast");
    updateVisibility();
}
