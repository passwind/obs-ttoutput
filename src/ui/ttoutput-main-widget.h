#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QThread>
#include <QMutex>
#include <QAtomicInt>
#include <QTimer>
#include "../ttoutput-plugin.h"

// Forward declarations
class TTOutputConfigTab;
class TTOutputSourceTab;
class TTOutputStatusTab;
class TTOutputProgressDialog;

// Worker thread for async output operations
class TTOutputWorker : public QObject
{
    Q_OBJECT

public slots:
    void startOutput(ttoutput_config_t *config);
    void stopOutput(ttoutput_config_t *config);

signals:
    void outputStarted(bool success);
    void outputStopped();
    void progressUpdate(const QString &message, int progress);
};

/**
 * Main widget for TTOutput plugin following MVC pattern
 * This class serves as the main container and controller for all UI components
 */
class TTOutputMainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TTOutputMainWidget(QWidget *parent = nullptr);
    ~TTOutputMainWidget();

    // Public interface for external access
    bool isOutputActive() const { return m_isOutputActive; }
    ttoutput_config_t* getCurrentConfig() const { return m_currentConfig; }
    
    // Public method to save current settings
    void saveSettings();

public slots:
    void onStartStopClicked();
    void onSaveConfigClicked();
    void onLoadConfigClicked();
    void onDeleteConfigClicked();
    void onUpdateStatus();
    
    // Async operation slots
    void onOutputStarted(bool success);
    void onOutputStopped();
    void onProgressUpdate(const QString &message, int progress);

signals:
    void outputStatusChanged(bool isActive);
    void configurationChanged();

private slots:
    void onTabChanged(int index);

private:
    void setupUI();
    void setupWorkerThread();
    void cleanupWorkerThread();
    
    void loadSettings();
    
    bool validateSettings();
    ttoutput_config_t* createConfigFromUI();
    void applyConfigToUI(const ttoutput_config_t *config);

    // UI Components
    QTabWidget *m_tabWidget;
    TTOutputConfigTab *m_configTab;
    TTOutputSourceTab *m_sourceTab;
    TTOutputStatusTab *m_statusTab;
    TTOutputProgressDialog *m_progressDialog;
    
    // Data and state management
    ttoutput_config_t *m_currentConfig;
    QTimer *m_statusTimer;
    bool m_isOutputActive;
    bool m_isDestroying;
    
    // Async operation support
    QThread *m_workerThread;
    TTOutputWorker *m_worker;
    QMutex m_configMutex;
    QAtomicInt m_isStarting;
    QAtomicInt m_isStopping;
    ttoutput_config_t *m_pendingConfig;
};
