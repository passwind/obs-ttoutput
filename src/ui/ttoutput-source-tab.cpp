#include "ttoutput-source-tab.h"
#include "../ttoutput-plugin.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QAbstractItemView>
#include <obs.h>

TTOutputSourceTab::TTOutputSourceTab(QWidget *parent)
    : QWidget(parent)
    , m_sourceList(nullptr)
    , m_refreshSourcesButton(nullptr)
    , m_selectedSourcesLabel(nullptr)
{
    setupUI();
    updateSourceList();
}

void TTOutputSourceTab::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
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
    
    // Connect signals
    connect(m_sourceList, &QListWidget::itemSelectionChanged,
            this, &TTOutputSourceTab::onSourceSelectionChanged);
    connect(m_refreshSourcesButton, &QPushButton::clicked,
            this, &TTOutputSourceTab::onRefreshSourcesClicked);
}

void TTOutputSourceTab::updateSourceList()
{
    m_sourceList->clear();
    
    // Enumerate video sources
    auto enum_video_sources = [](void *param, obs_source_t *source) -> bool {
        TTOutputSourceTab *tab = static_cast<TTOutputSourceTab*>(param);
        
        if (!source) return true;
        
        uint32_t flags = obs_source_get_output_flags(source);
        if ((flags & OBS_SOURCE_VIDEO) != 0) {
            const char *name = obs_source_get_name(source);
            if (name && strlen(name) > 0) {
                QListWidgetItem *item = new QListWidgetItem(QString::fromUtf8(name));
                item->setData(Qt::UserRole, "video");
                item->setData(Qt::UserRole + 1, QVariant::fromValue(reinterpret_cast<quintptr>(source)));
                tab->m_sourceList->addItem(item);
            }
        }
        
        return true;
    };
    
    // Enumerate audio sources
    auto enum_audio_sources = [](void *param, obs_source_t *source) -> bool {
        TTOutputSourceTab *tab = static_cast<TTOutputSourceTab*>(param);
        
        if (!source) return true;
        
        uint32_t flags = obs_source_get_output_flags(source);
        if ((flags & OBS_SOURCE_AUDIO) != 0) {
            const char *name = obs_source_get_name(source);
            if (name && strlen(name) > 0) {
                QListWidgetItem *item = new QListWidgetItem(QString::fromUtf8(name));
                item->setData(Qt::UserRole, "audio");
                item->setData(Qt::UserRole + 1, QVariant::fromValue(reinterpret_cast<quintptr>(source)));
                tab->m_sourceList->addItem(item);
            }
        }
        
        return true;
    };
    
    // Enumerate all sources
    obs_enum_sources(enum_video_sources, this);
    obs_enum_sources(enum_audio_sources, this);
    
    updateSelectedSourcesLabel();
}

void TTOutputSourceTab::onSourceSelectionChanged()
{
    updateSelectedSourcesLabel();
    emit sourceSelectionChanged();
}

void TTOutputSourceTab::onRefreshSourcesClicked()
{
    refreshSources();
}

void TTOutputSourceTab::updateSelectedSourcesLabel()
{
    int selectedCount = m_sourceList->selectedItems().count();
    m_selectedSourcesLabel->setText(QString("Selected: %1 sources").arg(selectedCount));
}

void TTOutputSourceTab::applyConfig(const ttoutput_config_t *config)
{
    if (!config) {
        // Clear all selections
        m_sourceList->clearSelection();
        return;
    }
    
    // Clear current selection
    m_sourceList->clearSelection();
    
    // 依据配置选择来源
    for (int i = 0; i < m_sourceList->count(); ++i) {
        QListWidgetItem *item = m_sourceList->item(i);
        const QString sourceName = item->text();

        for (int s = 0; s < config->source_count; ++s) {
            if (!config->sources[s].enabled) {
                continue;
            }

            if (sourceName == QString::fromUtf8(config->sources[s].name)) {
                item->setSelected(true);
                break;
            }
        }
    }
    
    updateSelectedSourcesLabel();
}

void TTOutputSourceTab::fillConfig(ttoutput_config_t *config) const
{
    if (!config) {
        return;
    }
    
    // Clear existing sources
    config->source_count = 0;
    
    // Get selected sources
    QList<QListWidgetItem*> selectedItems = m_sourceList->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    // Fill sources array
    int sourceIndex = 0;
    for (QListWidgetItem *item : selectedItems) {
        if (sourceIndex >= MAX_SOURCES) {
            break;
        }
        
        obs_source_t *source = reinterpret_cast<obs_source_t*>(
            item->data(Qt::UserRole + 1).value<quintptr>());
        
        if (source) {
            const char *source_name = obs_source_get_name(source);
            strncpy(config->sources[sourceIndex].name, source_name, sizeof(config->sources[sourceIndex].name) - 1);
            config->sources[sourceIndex].name[sizeof(config->sources[sourceIndex].name) - 1] = '\0';
            config->sources[sourceIndex].enabled = true;
            config->sources[sourceIndex].volume = 1.0f;
            sourceIndex++;
        }
    }
    
    config->source_count = sourceIndex;
}

bool TTOutputSourceTab::hasSelectedSources() const
{
    return !m_sourceList->selectedItems().isEmpty();
}

void TTOutputSourceTab::refreshSources()
{
    updateSourceList();
}

#include "moc_ttoutput-source-tab.cpp"
