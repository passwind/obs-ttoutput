#pragma once

#include "ttoutput-plugin.h"

class TTOutputDock : public QWidget
{
    Q_OBJECT

public:
    explicit TTOutputDock(QWidget *parent = nullptr);
    ~TTOutputDock();

private slots:
    void onOutputTypeChanged(int index);
    void onStartStopClicked();
    void onSaveConfigClicked();
    void onLoadConfigClicked();
    void onSourceSelectionChanged();
    void onUpdateStatus();
    void onBrowseFileClicked();
    void onRefreshSourcesClicked();
    void onDeleteConfigClicked();

private:
    void setupUI();
    void setupOutputTab();
    void setupSourceTab();
    void setupControlTab();
    void setupConfigTab();
    
    void updateSourceList();
    void updateOutputStatus();
    void updateControlButtons();
    void updateConfigList();
    void loadSettings();
    void saveSettings();
    
    bool validateSettings();
    obs_data_t *getSettingsData();
    void setSettingsData(obs_data_t *data);
    
    ttoutput_config_t* createConfigFromUI();
    void applyConfigToUI(const ttoutput_config_t *config);

    // UI Components
    QTabWidget *m_tabWidget;
    
    // Output Configuration Tab
    QWidget *m_outputTab;
    QComboBox *m_outputTypeCombo;
    QGroupBox *m_rtmpGroup;
    QLineEdit *m_rtmpUrlEdit;
    QLineEdit *m_rtmpKeyEdit;
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
    
    // Source Selection Tab
    QWidget *m_sourceTab;
    QListWidget *m_sourceList;
    QPushButton *m_refreshSourcesButton;
    QLabel *m_selectedSourcesLabel;
    
    // Control Tab
    QWidget *m_controlTab;
    QPushButton *m_startStopButton;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QLabel *m_bitrateLabel;
    QLabel *m_droppedFramesLabel;
    QLabel *m_cpuUsageLabel;
    
    // Configuration Tab
    QWidget *m_configTab;
    QLineEdit *m_configNameEdit;
    QPushButton *m_saveConfigButton;
    QComboBox *m_configListCombo;
    QPushButton *m_loadConfigButton;
    QPushButton *m_deleteConfigButton;
    
    // Data
    ttoutput_config_t *m_currentConfig;
    QTimer *m_statusTimer;
    bool m_isOutputActive;
};
