#pragma once

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>

/**
 * Progress dialog for TTOutput operations
 * Shows progress during start/stop operations
 */
class TTOutputProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TTOutputProgressDialog(QWidget *parent = nullptr);
    ~TTOutputProgressDialog();

    void setProgress(int value);
    void setMessage(const QString &message);
    void setCanCancel(bool canCancel);

signals:
    void cancelled();

private slots:
    void onCancelClicked();

private:
    void setupUI();

    QProgressBar *m_progressBar;
    QLabel *m_messageLabel;
    QPushButton *m_cancelButton;
};
