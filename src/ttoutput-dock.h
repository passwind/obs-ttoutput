#pragma once

#include <QWidget>

// Forward declarations
class TTOutputMainWidget;

class TTOutputDock : public QWidget
{
    Q_OBJECT

public:
    explicit TTOutputDock(QWidget *parent = nullptr);
    ~TTOutputDock();
    
    // Save current configuration
    void saveConfiguration();

private:
    void setupUI();

    // Main widget that contains all UI functionality
    TTOutputMainWidget *m_mainWidget;
};
