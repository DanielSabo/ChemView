#ifndef OPTIMIZERPROGRESSDIALOG_H
#define OPTIMIZERPROGRESSDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

class OptimizerProgressDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OptimizerProgressDialog(QWidget *parent = nullptr);

    void setText(QString text);
    QString getText();

    QLabel *message;
    QPushButton *cancelButton;
    QProgressBar *progressBar;
};

#endif // OPTIMIZERPROGRESSDIALOG_H
