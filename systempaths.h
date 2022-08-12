#ifndef SYSTEMPATHS_H
#define SYSTEMPATHS_H

#include <QDir>
#include <QString>
#include <QProcess>

namespace SystemPaths
{
    QDir scriptDir();
    QString script(QString name);

    QString nwchemBin();

    QString nwchemBinDefault();

    void setupEnviroment();

    QProcessEnvironment setupNWChemEnvironment();
}

#endif // SYSTEMPATHS_H
