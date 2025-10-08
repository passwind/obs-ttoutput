#include "ttoutput-status-tab.h"

#include "moc_ttoutput-status-tab.cpp"

TTOutputStatusTab::TTOutputStatusTab(QWidget *parent)
    : QWidget(parent)
    , m_startStopButton(nullptr)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_bitrateLabel(nullptr)
    , m_droppedFramesLabel(nullptr)
    , m_cpuUsageLabel(nullptr)
    , m_outputActive(false)
{
    setupUI();
}

void TTOutputStatusTab::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    
    setupControlSection(layout);
    setupStatusSection(layout);
}

void TTOutputStatusTab::setupControlSection(QVBoxLayout *layout)
{
    QGroupBox *controlGroup = new QGroupBox("Output Control");
    controlGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; color: #FFFFFF; border: 2px solid #5A5A5F; "
        "border-radius: 5px; margin-top: 10px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px 0 5px; }"
    );
    
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
    
    m_startStopButton = new QPushButton("Start Output");
    m_startStopButton->setStyleSheet(
        "QPushButton { background-color: #0078D4; color: #FFFFFF; border: none; "
        "padding: 10px 20px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #106EBE; }"
        "QPushButton:pressed { background-color: #005A9E; }"
        "QPushButton:disabled { background-color: #5A5A5F; color: #AAAAAA; }"
    );
    controlLayout->addWidget(m_startStopButton);
    
    layout->addWidget(controlGroup);
    
    connect(m_startStopButton, &QPushButton::clicked,
            this, &TTOutputStatusTab::onStartStopClicked);
}

void TTOutputStatusTab::setupStatusSection(QVBoxLayout *layout)
{
    QGroupBox *statusGroup = new QGroupBox("Output Status");
    statusGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; color: #FFFFFF; border: 2px solid #5A5A5F; "
        "border-radius: 5px; margin-top: 10px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px 0 5px; }"
    );
    
    QVBoxLayout *statusLayout = new QVBoxLayout(statusGroup);
    
    // Status label
    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("QLabel { color: #FFFFFF; font-size: 12px; }");
    statusLayout->addWidget(m_statusLabel);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setStyleSheet(
        "QProgressBar { border: 2px solid #5A5A5F; border-radius: 5px; text-align: center; }"
        "QProgressBar::chunk { background-color: #0078D4; border-radius: 3px; }"
    );
    m_progressBar->setVisible(false);
    statusLayout->addWidget(m_progressBar);
    
    // Statistics
    m_bitrateLabel = new QLabel("Bitrate: 0 kbps");
    m_bitrateLabel->setStyleSheet("QLabel { color: #FFFFFF; font-size: 11px; }");
    statusLayout->addWidget(m_bitrateLabel);
    
    m_droppedFramesLabel = new QLabel("Dropped Frames: 0 / 0 (0.0%)");
    m_droppedFramesLabel->setStyleSheet("QLabel { color: #FFFFFF; font-size: 11px; }");
    statusLayout->addWidget(m_droppedFramesLabel);
    
    m_cpuUsageLabel = new QLabel("CPU Usage: 0.0%");
    m_cpuUsageLabel->setStyleSheet("QLabel { color: #FFFFFF; font-size: 11px; }");
    statusLayout->addWidget(m_cpuUsageLabel);
    
    layout->addWidget(statusGroup);
}

void TTOutputStatusTab::onStartStopClicked()
{
    emit startStopClicked();
}

void TTOutputStatusTab::updateStartStopButton()
{
    if (m_outputActive) {
        m_startStopButton->setText("Stop Output");
        m_startStopButton->setStyleSheet(
            "QPushButton { background-color: #D13438; color: #FFFFFF; border: none; "
            "padding: 10px 20px; font-size: 14px; font-weight: bold; }"
            "QPushButton:hover { background-color: #B71C1C; }"
            "QPushButton:pressed { background-color: #8E0000; }"
        );
    } else {
        m_startStopButton->setText("Start Output");
        m_startStopButton->setStyleSheet(
            "QPushButton { background-color: #0078D4; color: #FFFFFF; border: none; "
            "padding: 10px 20px; font-size: 14px; font-weight: bold; }"
            "QPushButton:hover { background-color: #106EBE; }"
            "QPushButton:pressed { background-color: #005A9E; }"
        );
    }
}

void TTOutputStatusTab::applyConfig(const ttoutput_config_t *config)
{
    // Status tab doesn't need to apply configuration
    Q_UNUSED(config);
}

void TTOutputStatusTab::fillConfig(ttoutput_config_t *config) const
{
    // Status tab doesn't need to fill configuration
    Q_UNUSED(config);
}

void TTOutputStatusTab::updateStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

void TTOutputStatusTab::updateProgress(int percentage)
{
    if (percentage >= 0 && percentage <= 100) {
        m_progressBar->setValue(percentage);
        m_progressBar->setVisible(true);
    } else {
        m_progressBar->setVisible(false);
    }
}

void TTOutputStatusTab::updateBitrate(double bitrate)
{
    m_bitrateLabel->setText(QString("Bitrate: %1 kbps").arg(bitrate, 0, 'f', 1));
}

void TTOutputStatusTab::updateDroppedFrames(int dropped, int total)
{
    double percentage = total > 0 ? (double)dropped / total * 100.0 : 0.0;
    m_droppedFramesLabel->setText(
        QString("Dropped Frames: %1 / %2 (%3%)")
        .arg(dropped)
        .arg(total)
        .arg(percentage, 0, 'f', 1)
    );
}

void TTOutputStatusTab::updateCPUUsage(double usage)
{
    m_cpuUsageLabel->setText(QString("CPU Usage: %1%").arg(usage, 0, 'f', 1));
}

void TTOutputStatusTab::setOutputActive(bool active)
{
    m_outputActive = active;
    updateStartStopButton();
    
    if (active) {
        m_progressBar->setVisible(true);
    } else {
        m_progressBar->setVisible(false);
        updateStatus("Ready");
        updateProgress(0);
        updateBitrate(0.0);
        updateDroppedFrames(0, 0);
        updateCPUUsage(0.0);
    }
}

bool TTOutputStatusTab::isOutputActive() const
{
    return m_outputActive;
}
