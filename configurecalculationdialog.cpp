#include "configurecalculationdialog.h"
#include "ui_configurecalculationdialog.h"

namespace {
    static const QStringList methodNames = {"Hartree-Fock", "DFT"};
    static const QList<int> methodValues = {NWChemConfiguration::MethodSCF, NWChemConfiguration::MethodDFT};
}

ConfigureCalculationDialog::ConfigureCalculationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigureCalculationDialog)
{
    ui->setupUi(this);

    ui->methodComboBox->addItems(methodNames);
    ui->methodComboBox->setCurrentIndex(1);

    QStringList basisSets = {"6-31G**", "6-311G**"};
    ui->basisComboBox->addItems(basisSets);
    ui->basisComboBox->setCurrentIndex(0);

    QStringList functionals = {"B3LYP"};
    ui->functionalComboBox->addItems(functionals);
    ui->functionalComboBox->setCurrentIndex(0);
    currentFunctional = ui->functionalComboBox->currentText();

    connect(ui->functionalComboBox, &QComboBox::currentTextChanged, this, [this](QString text) {
        if (!text.isEmpty())
            currentFunctional = text;
    });

    // https://stackoverflow.com/questions/34603778/cannot-connect-qbuttongroup-buttonclicked-to-a-functor
    connect(ui->methodComboBox, &QComboBox::currentTextChanged, this, [this](QString text) {
        if (text == "Hartree-Fock")
        {
            ui->functionalComboBox->setCurrentText("");
            ui->functionalComboBox->setEnabled(false);
        }
        else
        {
            ui->functionalComboBox->setCurrentText(currentFunctional);
            ui->functionalComboBox->setEnabled(true);
        }
    });
}

ConfigureCalculationDialog::~ConfigureCalculationDialog()
{
    delete ui;
}

void ConfigureCalculationDialog::setConfig(const NWChemConfiguration &conf)
{
    ui->basisComboBox->setCurrentText(conf.basis);
    ui->functionalComboBox->setCurrentText(conf.functional);

    // Set the method last so the combo box event can clear/enable/disable the functional as needed
    int methodIdx = methodValues.indexOf(conf.method);
    if (methodIdx < 0)
        methodIdx = 0;
    ui->methodComboBox->setCurrentIndex(methodIdx);

    ui->taskOptCheckBox->setChecked(conf.taskOpt);
    ui->taskFreqCheckBox->setChecked(conf.taskFreq);
}

NWChemConfiguration ConfigureCalculationDialog::getConfig()
{
    NWChemConfiguration result;
    result.driverMaxIter = 100;
    result.method = NWChemConfiguration::Method(methodValues[ui->methodComboBox->currentIndex()]);
    result.basis = ui->basisComboBox->currentText();
    if (result.method == NWChemConfiguration::MethodDFT)
        result.functional = ui->functionalComboBox->currentText();
    else
        result.functional = QString();

    result.taskOpt = ui->taskOptCheckBox->isChecked();
    result.taskFreq = ui->taskFreqCheckBox->isChecked();

    return result;
}
