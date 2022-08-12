#include "optimizerbabelff.h"
#include "systempaths.h"

#include <QProcess>
#include <QTemporaryFile>
#include <QDebug>
#include <QCoreApplication>

static QByteArray findSDFSection(QByteArray const &data, QByteArray name)
{
    //FIXME: In a real SDF file we would need to deal with variable amounts of whitespace
    QByteArray header = QByteArrayLiteral("> <") + name + QByteArrayLiteral(">\n");
    int startIndex = data.indexOf(header);
    if (startIndex < 0)
        return {};
    int endIndex = data.indexOf("\n\n", startIndex + header.length());
    if (endIndex < 0)
        return {};
    return data.mid(startIndex + header.size(), endIndex - startIndex);
}

OptimizerBabelFF::OptimizerBabelFF(QObject *parent) : Optimizer(parent)
{

}

void OptimizerBabelFF::setStructure(MolStruct m)
{
    document = {};
    document.molecule = m;
}

MolStruct OptimizerBabelFF::getStructure()
{
    return document.molecule;
}

MolDocument OptimizerBabelFF::getResults()
{
    return document;
}

bool OptimizerBabelFF::run()
{
    if (document.molecule.atoms.isEmpty())
    {
        error = "No molecule";
        return false;
    }

    optimizeProcess = new QProcess(this);
    auto optimizeFile = new QTemporaryFile(optimizeProcess);
    optimizeFile->setFileTemplate(optimizeFile->fileTemplate() + ".mol");
    optimizeFile->open();
    optimizeFile->write(document.molecule.toMolFile());
    optimizeFile->close();

    // https://stackoverflow.com/questions/65454378/qprocess-signals-emitted-after-start-called
    connect(optimizeProcess, &QProcess::errorOccurred,
            this, [this](QProcess::ProcessError e) {
        (void)e;
        error = QStringLiteral("Optimizer error:\n") + optimizeProcess->errorString();
        emit finished();
    });

    connect(optimizeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode != 0 || exitStatus != QProcess::NormalExit)
            error = QStringLiteral("Optimizer error:\n") + QString::fromUtf8(optimizeProcess->readAllStandardError()) + QString::fromUtf8(optimizeProcess->readAllStandardOutput());
        else
        {
            try
            {
                QByteArray optimizeOutput = optimizeProcess->readAllStandardOutput();
                document.molecule = MolStruct::fromSDFData(optimizeOutput);

                QString methodName = QString::fromUtf8(findSDFSection(optimizeOutput, "Method")).trimmed();

                if (methodName.isEmpty())
                    methodName = QString("OpenBabel Force Field");
                document.calculatedProperties["Calculation Method"] = methodName;

                document.molecule.recenter();
            }  catch (QString e) {
                error = e;
            }
        }
        emit finished();
    });

    QString scriptPath = SystemPaths::script("babel_ff.sh");
    optimizeProcess->start("bash", {scriptPath, optimizeFile->fileName()});
    return true;
}

void OptimizerBabelFF::kill()
{
    if (!optimizeProcess)
        return;

    // I am unsure why the disconnect is necessary, however without it the QProcess will
    // emit a error signal before the process is actually dead and then hang in its
    // destructor...
    disconnect(optimizeProcess, 0, 0, 0);

    // In theory QProcess::NotRunning is sufficient not to bother trying to kill things, but
    // this code has suffered from enough race conditions to make me paranoid...

    error = "Optimizer canceled.";
    optimizeProcess->terminate();
    if (!optimizeProcess->waitForFinished(1000))
    {
        if (!(optimizeProcess->state() == QProcess::NotRunning))
            qDebug() << "Optimizer terminate failed...";

        optimizeProcess->kill();
        if(!optimizeProcess->waitForFinished(1000))
        {
            if (!(optimizeProcess->state() == QProcess::NotRunning))
                qWarning() << "Optimizer kill failed...";
        }
    }
    if (!(optimizeProcess->state() == QProcess::NotRunning))
        qDebug() << error;
}

QString OptimizerBabelFF::getError()
{
    return error;
}
