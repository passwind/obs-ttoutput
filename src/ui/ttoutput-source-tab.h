#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include "../ttoutput-plugin.h"

/**
 * Source selection tab for choosing audio/video sources
 * Handles the "Source Selection" tab in the main widget
 */
class TTOutputSourceTab : public QWidget
{
    Q_OBJECT

public:
    explicit TTOutputSourceTab(QWidget *parent = nullptr);
    ~TTOutputSourceTab() = default;

    // Configuration interface
    void applyConfig(const ttoutput_config_t *config);
    void fillConfig(ttoutput_config_t *config) const;
    bool hasSelectedSources() const;
    void refreshSources();

signals:
    void sourceSelectionChanged();

private slots:
    void onSourceSelectionChanged();
    void onRefreshSourcesClicked();

private:
    void setupUI();
    void updateSourceList();
    void updateSelectedSourcesLabel();

    // UI Components
    QListWidget *m_sourceList;
    QPushButton *m_refreshSourcesButton;
    QLabel *m_selectedSourcesLabel;
    
    // Loading state flag to prevent signal emission during config loading
    bool m_isLoading = false;
};
