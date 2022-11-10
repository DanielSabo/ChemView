#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QSettings>

#include "systempaths.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("ChemView");
    a.setApplicationDisplayName("ChemView");
    a.setOrganizationDomain("danielsabo.bitbucket.org");

#ifdef Q_OS_MAC
    QSettings::setDefaultFormat(QSettings::NativeFormat);
    //TODO: PlatformHelpers::macDisableAutoTabBar();
#else
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

    QCommandLineParser parser;
    parser.process(a);

    SystemPaths::setupEnviroment();

    MainWindow w;
    w.show();

    for (const auto &filename: parser.positionalArguments())
        w.openFile(filename);

    return a.exec();
}
