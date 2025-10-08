#include "ttoutput-progress-dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

#include "moc_ttoutput-progress-dialog.cpp"

TTOutputProgressDialog::TTOutputProgressDialog(QWidget *parent)
    : QDialog(parent)
    , m_progressBar(nullptr)
    , m_messageLabel(nullptr)
    , m_cancelButton(nullptr)
{
    setWindowTitle("TTOutput Operation");
    setModal(true);
    setFixedSize(400, 150);
    
    setupUI();
}

TTOutputProgressDialog::~TTOutputProgressDialog()
{
    // Cleanup handled by Qt parent-child relationship
}

void TTOutputProgressDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // Message label
    m_messageLabel = new QLabel("Initializing...");
    m_messageLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_messageLabel);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    layout->addWidget(m_progressBar);
    
    // Cancel button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton("Cancel");
    m_cancelButton->setEnabled(false); // Disabled by default
    connect(m_cancelButton, &QPushButton::clicked, this, &TTOutputProgressDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);
    
    layout->addLayout(buttonLayout);
}

void TTOutputProgressDialog::setProgress(int value)
{
    if (m_progressBar) {
        m_progressBar->setValue(value);
    }
}

void TTOutputProgressDialog::setMessage(const QString &message)
{
    if (m_messageLabel) {
        m_messageLabel->setText(message);
    }
}

void TTOutputProgressDialog::setCanCancel(bool canCancel)
{
    if (m_cancelButton) {
        m_cancelButton->setEnabled(canCancel);
    }
}

void TTOutputProgressDialog::onCancelClicked()
{
    emit cancelled();
    reject();
}
