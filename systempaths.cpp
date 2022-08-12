#include "systempaths.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QSettings>

#if defined(Q_OS_MAC)
#include <sys/sysctl.h>
#include <mach/machine.h>
#endif

QDir SystemPaths::scriptDir()
{
    //return QDir(QCoreApplication::applicationDirPath() + "/scripts/").absolutePath();
#if defined(Q_OS_MAC)
    return QDir(QCoreApplication::applicationDirPath() + "/../Resources/share/scripts/");
#else
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + QString("/../" DATA_PREFIX "/./scripts/"));
#if !defined(DATA_PREFIX)
#error DATA_PREFIX not defined
#endif
#endif
}

QString SystemPaths::script(QString name)
{
    return QDir::toNativeSeparators(scriptDir().filePath(name));
}

QString SystemPaths::nwchemBinDefault()
{
    return QStringLiteral("mpirun nwchem");
}

QString SystemPaths::nwchemBin()
{
    QString command = QSettings().value("NWChemPath").toString();
    if (command.isEmpty())
        return nwchemBinDefault();
    return command;
}

QProcessEnvironment SystemPaths::setupNWChemEnvironment()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

#if defined(Q_OS_MAC)
    env.insert("NWCHEM_BASIS_LIBRARY", QCoreApplication::applicationDirPath() + "/../Resources/share/nwchem/libraries/");
    env.insert("OMP_NUM_THREADS", "1");
#endif

    return env;
}

void SystemPaths::setupEnviroment()
{
#if defined(Q_OS_MAC)
    // Detect ARM/x86
    // https://stackoverflow.com/questions/63214720/determine-cpu-type-architecture-in-macos
    bool isARM = false;
    {
        uint32_t cputype = 0;
        size_t size = sizeof (cputype);
        if(0 == sysctlbyname ("hw.cputype", &cputype, &size, NULL, 0))
        {
            isARM = (cputype & 0xFF) == CPU_TYPE_ARM;
        }
        else
        {
            qWarning() << "Error detecting CPU type, defaulting to x86";
        }

        qDebug() << "CPU type:" << (isARM ? "ARM" : "x86");
    }

    QByteArray path = qgetenv("PATH");
    QDir prefixDir;
    QDir binDir;
    QDir babelLibDir;
    if (isARM)
    {
        prefixDir = QDir(QCoreApplication::applicationDirPath() + "/../Resources/arm/");
        binDir = QDir(QCoreApplication::applicationDirPath() + "/../Resources/arm/bin/");
        babelLibDir =  QDir(QCoreApplication::applicationDirPath() + "/../Resources/arm/lib/openbabel/3.1.0/");
    }
    else
    {
        prefixDir = QDir(QCoreApplication::applicationDirPath() + "/../Resources/x86/");
        binDir = QDir(QCoreApplication::applicationDirPath() + "/../Resources/x86/bin/");
        babelLibDir =  QDir(QCoreApplication::applicationDirPath() + "/../Resources/x86/lib/openbabel/3.1.0/");
    }
    QDir babelDataDir =  QDir(QCoreApplication::applicationDirPath() + "/../Resources/share/openbabel/3.1.0/");

    qputenv("OPAL_PREFIX", QDir::toNativeSeparators(prefixDir.absolutePath()).toUtf8());
    qputenv("BABEL_LIBDIR", QDir::toNativeSeparators(babelLibDir.absolutePath()).toUtf8());
    qputenv("BABEL_DATADIR", QDir::toNativeSeparators(babelDataDir.absolutePath()).toUtf8());
    qputenv("PATH", QDir::toNativeSeparators(binDir.absolutePath()).toUtf8() + ":" + path);
#endif
}
