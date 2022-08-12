#include "mainwindow.h"
#include "propertieswindow.h"
#include "ui_propertieswindow.h"

#include <cmath>
#include <QCloseEvent>
#include <QSettings>
#include <QTextStream>

PropertiesWindow::PropertiesWindow(QWidget *parent) :
    QWidget(parent, Qt::Tool),
    ui(new Ui::PropertiesWindow)
{
    ui->setupUi(this);

    ui->label->setOpenExternalLinks(false);
    ui->label->setWordWrap(true);

    connect(ui->label, &QLabel::linkActivated, this, [this](QString link){
        const auto orbitalPrefix = QStringLiteral("orbital://");
        const auto vibrationPrefix = QStringLiteral("vibration://");
        if (link.startsWith(orbitalPrefix))
        {
            QString surfaceName = link.mid(orbitalPrefix.size());
            if(MainWindow *mainwindow = qobject_cast<MainWindow *>(this->parent()))
                mainwindow->generateNWChemSurface(surfaceName, 1.0E-02);
        }
        else if (link.startsWith(vibrationPrefix))
        {
            int vibrationId = link.mid(vibrationPrefix.size()).toInt();
            if(MainWindow *mainwindow = qobject_cast<MainWindow *>(this->parent()))
                mainwindow->animateFrequency(vibrationId);
        }
    });

    showData({}, false);
}

PropertiesWindow::~PropertiesWindow()
{
    delete ui;
}

void PropertiesWindow::showEvent(QShowEvent *)
{
    QSettings appSettings;

    if (appSettings.contains("PropertiesWindow/geometry"))
        restoreGeometry(appSettings.value("PropertiesWindow/geometry").toByteArray());
}

void PropertiesWindow::hideEvent(QHideEvent *)
{
    QSettings().setValue("PropertiesWindow/geometry", saveGeometry());
}


void PropertiesWindow::showData(const MolDocument &document, bool optimizerAvailable)
{
    QString newLabelText;
    QTextStream stream(&newLabelText);

    QString stylesheet = QStringLiteral(
                "<style>\n"
                "table, th, td {\n"
                "  border: 1px solid;\n"
                "  border-collapse: collapse;\n"
                "  padding: 3px;\n"
                "  font-weight: normal;\n"
                "}\n"
                "</style>\n");
    stream << stylesheet;

    if (!document.calculatedProperties.isEmpty())
    {
        stream << "<b>Properties:</b><br>\n";
        for (auto iter = document.calculatedProperties.begin(); iter != document.calculatedProperties.end(); iter++)
        {
            stream << iter.key() << ": " << iter.value() << "<br>\n";
        }
        stream << "<br>\n";
    }

    /* Based on comparison to the values in this paper ( https://doi.org/10.1021/jp061633o ) the orbital energies
     * appear to be given in Hartrees.
     */

    if (!document.orbitals.isEmpty())
    {
        QString format;
        if (optimizerAvailable)
            format = QStringLiteral("<tr><td><a href='orbital://%1'>%1</a></td><td>%2</td><td>%3</td></tr>");
        else
            format = QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td></tr>");

        stream << "<b>Orbitals:</b>";
        stream << "<table>";
        stream << "<tr><th>ID</th><th>Occupancy</th><th>Energy (au)</th></tr>";

        for (auto iter = document.orbitals.begin(); iter != document.orbitals.end(); iter++)
        {
            if (iter->occupancy <= 0.0001)
                break;
            //FIXME: Ensure proper rounding of values? https://bugreports.qt.io/browse/QTBUG-38171 seems to indicate it may already happen automatically

            stream << format.arg(iter->id).arg(iter->occupancy, 0, 'f', 2).arg(iter->energy, 0, 'E', 6);
        }

        stream << "</table><br>";
    }

    if (!document.frequencies.isEmpty())
    {
        stream << "<br>\n<b>Frequencies:</b>";
        const QString format = QStringLiteral("<tr><td><a href='vibration://%1'>%2</a></td><td>%3</td></tr>");
        stream << "<table>";
        stream << "<tr><th>Wavenumber (cm<sup>-1</sup>)</th><th>Intensity</th></tr>";

        int index = 0;
        for (auto iter = document.frequencies.begin(); iter != document.frequencies.end(); iter++)
            stream << format.arg(index++).arg(iter->wavenum, 0, 'f', 3).arg(iter->intensity, 0, 'f', 3);

        stream << "</table><br>";
    }

    if (newLabelText.isEmpty())
        ui->label->setText("<no data>");
    else
        ui->label->setText(newLabelText);
}
