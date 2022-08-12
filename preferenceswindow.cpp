#include "preferenceswindow.h"
#include "ui_preferenceswindow.h"
#include "systempaths.h"

#include <QSettings>

PreferencesWindow::PreferencesWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PreferencesWindow)
{
    ui->setupUi(this);

    QSettings appSettings;
    ui->NWChemEntry->setText(appSettings.value("NWChemPath").toString());
    ui->NWChemEntry->setPlaceholderText(SystemPaths::nwchemBinDefault());
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &PreferencesWindow::saveSettings);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &PreferencesWindow::saveSettings);
}

PreferencesWindow::~PreferencesWindow()
{
    delete ui;
}

void PreferencesWindow::saveSettings()
{
    QSettings appSettings;
    appSettings.setValue("NWChemPath", ui->NWChemEntry->text());
    close();
}

void PreferencesWindow::discardChanges()
{
    close();
}
