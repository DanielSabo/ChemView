#ifndef CONFIGURECALCULATIONDIALOG_H
#define CONFIGURECALCULATIONDIALOG_H

#include <QDialog>
#include "nwchemconfiguration.h"

namespace Ui {
class ConfigureCalculationDialog;
}

class ConfigureCalculationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigureCalculationDialog(QWidget *parent = nullptr);
    ~ConfigureCalculationDialog();

    void setConfig(NWChemConfiguration const &conf);
    NWChemConfiguration getConfig();

private:
    Ui::ConfigureCalculationDialog *ui;

    QString currentFunctional;
};

#endif // CONFIGURECALCULATIONDIALOG_H
