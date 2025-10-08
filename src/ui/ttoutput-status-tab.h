#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

#include <plugin-support.h>
#include "../ttoutput-plugin.h"

class TTOutputStatusTab : public QWidget
{
    Q_OBJECT

public:
    explicit TTOutputStatusTab(QWidget *parent = nullptr);
    
    // Configuration management
    void applyConfig(const ttoutput_config_t *config);
    void fillConfig(ttoutput_config_t *config) const;
    
    // Status updates
    void updateStatus(const QString &status);
    void updateProgress(int percentage);
    void updateBitrate(double bitrate);
    void updateDroppedFrames(int dropped, int total);
    void updateCPUUsage(double usage);
    
    // Control state
    void setOutputActive(bool active);
    bool isOutputActive() const;

signals:
    void startStopClicked();

private slots:
    void onStartStopClicked();

private:
    void setupUI();
    void setupControlSection(QVBoxLayout *layout);
    void setupStatusSection(QVBoxLayout *layout);
    void updateStartStopButton();

    // UI elements
    QPushButton *m_startStopButton;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QLabel *m_bitrateLabel;
    QLabel *m_droppedFramesLabel;
    QLabel *m_cpuUsageLabel;
    
    // State
    bool m_outputActive;
};
