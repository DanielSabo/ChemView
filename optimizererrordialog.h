#ifndef OPTIMIZERERRORDIALOG_H
#define OPTIMIZERERRORDIALOG_H

#include <QDialog>
#include <QUrl>

namespace Ui {
class OptimizerErrorDialog;
}

class OptimizerErrorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptimizerErrorDialog(QWidget *parent = nullptr);
    ~OptimizerErrorDialog();

    void setText(QString text);
    void setHasGeometry(bool hasGeom);
    void setOutputURL(QUrl url);

private:
    QUrl outputURL;

    Ui::OptimizerErrorDialog *ui;
};

#endif // OPTIMIZERERRORDIALOG_H
