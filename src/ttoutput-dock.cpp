#include "ttoutput-dock.h"
#include "ui/ttoutput-main-widget.h"

#include <QVBoxLayout>

TTOutputDock::TTOutputDock(QWidget *parent)
    : QWidget(parent)
    , m_mainWidget(nullptr)
{
    setObjectName("TTOutputDock");
    setWindowTitle("TTOutput Settings");
    setMinimumSize(400, 600);
    
    setupUI();
}

TTOutputDock::~TTOutputDock()
{
    // Save configuration before destroying the dock
    saveConfiguration();
    
    // Cleanup handled by Qt parent-child relationship
}

void TTOutputDock::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    // Create the main widget that contains all UI functionality
    m_mainWidget = new TTOutputMainWidget(this);
    layout->addWidget(m_mainWidget);
    
    setLayout(layout);
}

void TTOutputDock::saveConfiguration()
{
    if (m_mainWidget) {
        m_mainWidget->saveSettings();
    }
}



#include "moc_ttoutput-dock.cpp"
