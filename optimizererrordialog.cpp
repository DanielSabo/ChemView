#include "optimizererrordialog.h"
#include "ui_optimizererrordialog.h"

#include <QDesktopServices>
#include <QStyle>

OptimizerErrorDialog::OptimizerErrorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptimizerErrorDialog)
{
    ui->setupUi(this);
    QStyle *s = style();
    if (s->styleHint(QStyle::SH_DialogButtonBox_ButtonsHaveIcons, nullptr, this))
    {
        ui->okButton->setIcon(s->standardIcon(QStyle::SP_DialogOkButton, nullptr, ui->okButton));
    }
    ui->okButton->setDefault(true);
    ui->keepGeometryButton->setVisible(false);
    ui->openLogButton->setVisible(false);

    connect(ui->okButton, &QPushButton::clicked, this, [this](){
        reject();
    });
    connect(ui->keepGeometryButton, &QPushButton::clicked, this, [this](){
        accept();
    });
    connect(ui->openLogButton, &QPushButton::clicked, this, [this](){
        QDesktopServices::openUrl(outputURL);
    });
}

OptimizerErrorDialog::~OptimizerErrorDialog()
{
    delete ui;
}

void OptimizerErrorDialog::setText(QString text)
{
    ui->errorMessageLabel->setText(text);
}

void OptimizerErrorDialog::setHasGeometry(bool hasGeom)
{
    ui->keepGeometryButton->setVisible(hasGeom);
}

void OptimizerErrorDialog::setOutputURL(QUrl url)
{
    outputURL = url;
    ui->openLogButton->setVisible(outputURL.isValid());
}
