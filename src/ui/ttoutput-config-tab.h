#pragma once

#include "../ttoutput-plugin.h"
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QGroupBox>

/**
 * Configuration tab for output settings (RTMP/File) and encoder settings
 * Handles the "Output Settings" tab in the main widget
 */
class TTOutputConfigTab : public QWidget
{
    Q_OBJECT

public:
    explicit TTOutputConfigTab(QWidget *parent = nullptr);
    ~TTOutputConfigTab() = default;

    // Configuration interface
    void applyConfig(const ttoutput_config_t *config);
    void fillConfig(ttoutput_config_t *config) const;
    bool validateConfig() const;
    void resetToDefaults();

signals:
    void configurationChanged();
    void browseFileRequested();

private slots:
    void onOutputTypeChanged(int index);
    void onBrowseFileClicked();
    void onSettingChanged();

private:
    void setupUI();
    void setupOutputTypeSection();
    void setupRTMPSection();
    void setupFileSection();
    void setupEncoderSection();
    void updateVisibility();

    // Output Configuration
    QComboBox *m_outputTypeCombo;
    
    // RTMP Settings
    QGroupBox *m_rtmpGroup;
    QLineEdit *m_rtmpUrlEdit;
    QLineEdit *m_rtmpKeyEdit;
    
    // File Settings
    QGroupBox *m_fileGroup;
    QLineEdit *m_filePathEdit;
    QPushButton *m_browseFileButton;
    QComboBox *m_fileFormatCombo;
    
    // Encoder Settings
    QGroupBox *m_encoderGroup;
    QComboBox *m_codecCombo;
    QSpinBox *m_bitrateSpinBox;
    QSpinBox *m_widthSpinBox;
    QSpinBox *m_heightSpinBox;
    QSpinBox *m_fpsSpinBox;
    QComboBox *m_presetCombo;
};
