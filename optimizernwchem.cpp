#include "optimizernwchem.h"
#include "systempaths.h"
#include "nwchem.h"
#include "linebuffer.h"
#include "cubefile.h"

#include <QProcess>
#include <QTemporaryDir>
#include <QDebug>
#include <QSettings>
#include <cmath>

namespace {
    QString formatSteps(QStringList const &steps)
    {
        QString result;
        if (steps.size() >= 3)
        {
            result = QStringLiteral("<style>\n"
                                    "table, th, td {\n"
                                    "  border: 1px solid;\n"
                                    "  border-collapse: collapse;\n"
                                    "  padding: 3px;\n"
                                    "  font-weight: normal;\n"
                                    "  margin-top: 3px;\n"
                                    "}\n"
                                    "</style>\n"
                                    "<table>\n");

            QString headerStr = steps.first();
            headerStr.remove(0, 2);
            // The header gets split using two spaces to avoid cutting "Delta E" apart.
            QStringList headerLine = headerStr.split("  ", Qt::SkipEmptyParts);
            QString stepStr = steps.last();
            stepStr.remove(0, 2);
            QStringList stepLine = stepStr.split(" ", Qt::SkipEmptyParts);
            result += QStringLiteral("<tr><th>") + headerLine.join("</th><th>") + QStringLiteral("</td></tr>\n");
            result += QStringLiteral("<tr><td>") + stepLine.join("</td><td>") + QStringLiteral("</td></tr>\n");
            result += "</table>\n";
        }
        return result;
    }

    QString frequencyFeedback(LineBuffer const &buffer, int start)
    {
        static const std::regex atom_step_line("( atom: .*)date:.*");
        static const std::regex hessian_line(" HESSIAN: .*");

        std::cmatch match;
        for (int i = buffer.lines.size() - 1; i >= start; i--)
        {
            const QByteArray &line = buffer.lines.at(i);
            if (std::regex_match(line.data(), match, atom_step_line))
            {
                return QString::fromStdString(match.str(1)).trimmed();
            }
            else if (std::regex_match(line.data(), match, hessian_line))
            {
                return QString::fromUtf8(line.data()).simplified();
            }
        }

        return QString();
    }

    QString generateTaskHeader(NWChemConfiguration const &nwchemConfig, QString taskName)
    {
        if (nwchemConfig.method == NWChemConfiguration::MethodDFT)
            return QStringLiteral("<center><b>NWChem DFT %1 (%2/%3)</b></center>").arg(taskName).arg(nwchemConfig.functional).arg(nwchemConfig.basis);
        else if (nwchemConfig.method == NWChemConfiguration::MethodSCF)
            return QStringLiteral("<center><b>NWChem Hartree-Fock %1 (%2)</b></center>").arg(taskName).arg(nwchemConfig.basis);
        return QStringLiteral("<center><b>NWChem %1</b></center>").arg(taskName);
    }

    MolStruct copyChargeAndBonds(MolStruct const &from, MolStruct to)
    {
        // NWChem does not provide bond order or formal charge information, so for now we copy
        // it from the orginial molecule.
        if (to.atoms.size() == from.atoms.size())
        {
            // TODO: Verify connectivity is the same (we're just trying to copy the order if possible)
            to.bonds = from.bonds;

            for (int i = 0; i < to.atoms.size(); ++i)
                to.atoms[i].charge = from.atoms[i].charge;
        }
        else
        {
            qWarning() << "copyChargeAndBonds: Atom lists don't match!";
        }

        return to;
    }
}

OptimizerNWChem::OptimizerNWChem(QObject *parent) : Optimizer(parent)
{

}

OptimizerNWChem::~OptimizerNWChem()
{

}

void OptimizerNWChem::setConfiguration(NWChemConfiguration config)
{
    MolDocument newDocument;
    newDocument.molecule = document.molecule;
    document = newDocument;
    nwchemConfig = config;
    optimized = false;
}

NWChemConfiguration OptimizerNWChem::getConfiguration()
{
    return nwchemConfig;
}

void OptimizerNWChem::requestSurface(QString name)
{
    pendingSurfaces.append(name);
}

void OptimizerNWChem::setStructure(MolStruct m)
{
    document = {};
    document.molecule = m;
    optimized = false;
}

MolStruct OptimizerNWChem::getStructure()
{
    return document.molecule;
}

MolDocument OptimizerNWChem::getResults()
{
    return document;
}

QString OptimizerNWChem::getLogFile()
{
    if (outputFile)
    {
        if (outputFile->isOpen())
            outputFile->flush();
        return outputFile->fileName();
    }
    return {};
}

void OptimizerNWChem::saveToProjFile(QZipWriter &writer, QString dirname) const
{
    if(!dir->isValid())
        return;
    QDir directory(dir->path());
    writer.addDirectory(dirname);
    directory.setNameFilters({QStringLiteral("molecule.*")});
    for (auto filePath: directory.entryList())
    {
//        qDebug() << filePath;
        QFile file(directory.filePath(filePath));
        file.open(QIODevice::ReadOnly);
        writer.addFile(QDir::toNativeSeparators(dirname+"/"+filePath), &file);
        file.close();
    }
    writer.addFile(QDir::toNativeSeparators(dirname+"/config.json"), nwchemConfig.serialize());
    writer.addFile(QDir::toNativeSeparators(dirname+"/output.log"), outputLineBuffer.joined());
}

std::unique_ptr<OptimizerNWChem> OptimizerNWChem::fromProjFile(const QString &filename, QString dirname, MolStruct m)
{
    QZipReader zipfile(filename);
    std::unique_ptr<OptimizerNWChem> result(new OptimizerNWChem);

    result->setStructure(m);
    if(!result->initializeOutputDirectory())
        throw QString("Failed to create temporary directory");
    result->optimized = true;

    /* QZipReader reader can't tell us if a file or directory exists except by
     * manually interating everything so here we track if we found the relevant
     * files.
     */
    bool optimizerDirectoryFound = false;
    bool configOK = false;
    bool logfileOK = false;

    for (auto zipInfo: zipfile.fileInfoList())
    {
        if (!zipInfo.isFile)
            continue;

        QFileInfo fileInfo(zipInfo.filePath);
        if (fileInfo.dir() == dirname)
        {
//            qDebug() << " z:" << zipInfo.filePath;
            optimizerDirectoryFound = true;
            if (fileInfo.fileName() == "config.json")
            {
                result->nwchemConfig = NWChemConfiguration(zipfile.fileData(zipInfo.filePath));
                configOK = true;
            }
            else if (fileInfo.fileName() == "output.log")
            {
                QByteArray logData = zipfile.fileData(zipInfo.filePath);
                if (result->initializeLogFile())
                    result->outputFile->write(logData);
                result->outputLineBuffer.append(logData);
                logfileOK = true;
            }
            else
            {
                QFile file(result->dir->filePath(fileInfo.fileName()));
                file.open(QIODevice::WriteOnly);
                file.write(zipfile.fileData(zipInfo.filePath));
                file.close();
            }
        }
    }

    if (!optimizerDirectoryFound)
        return {};

    if (!configOK)
        throw QString("No optimizer config found");
    if (!logfileOK)
        throw QString("No optimizer output found");

    qDebug() << "Loaded optimizer:" << result->dir->path();

    return result;
}

bool OptimizerNWChem::run()
{
    error = QString();

    if (document.molecule.atoms.isEmpty())
    {
        error = QStringLiteral("No molecule");
        return false;
    }

    // If we haven't already optimized ensure we're working in a clean directory
    if (!optimized)
    {
        if(!initializeOutputDirectory())
        {
            error = QStringLiteral("Failed to create temporary directory");
            return false;
        }

        outputLineBuffer.lines.clear();
    }
    else if (pendingSurfaces.empty())
    {
        error = QStringLiteral("Nothing to do");
        return false;
    }

    // Set up a new process
    optimizeProcess.reset(new QProcess);
    optimizeProcess->setWorkingDirectory(dir->path());
    parser.reset(new NWChem::Parser(outputLineBuffer));
    parser->parse();

    // https://stackoverflow.com/questions/65454378/qprocess-signals-emitted-after-start-called
    connect(optimizeProcess.get(), &QProcess::errorOccurred,
            this, [this](QProcess::ProcessError e) {
        (void)e;
        error = QStringLiteral("Error starting NWChem:\n") + optimizeProcess->errorString();
        state = NWChemTask::None;
        emit finished();
    });

    connect(optimizeProcess.get(), &QProcess::readyReadStandardOutput,
            this, [this]() {

        bool newSectionParsed = false;
        int priorLastLine = parser->buffer.lines.size() - 2;
        int priorTaskCount = parser->tasks.size();
        if (priorTaskCount && parser->tasks.back().type == NWChem::Parser::SectionType::Invalid)
            priorTaskCount -= 1;

        try {
             newSectionParsed = parser->parseIncremental(readAndLogOutput());
        }  catch (QString e) {
            qWarning() << e;
        }

        if (NWChemTask::GeometryOpt == state)
        {
            if (newSectionParsed)
            {
                if (!parser->document.molecule.isEmpty())
                {
                    auto newMolecule = parser->document.molecule;
                    document.molecule = copyChargeAndBonds(document.molecule, newMolecule);
                    emit geometryUpdate();
                }

                if (parser->tasks.back().type == NWChem::Parser::SectionType::GeometryOpt)
                {
                    QString newStatus = generateTaskHeader(nwchemConfig, QStringLiteral("Optmization"));
                    QString stepInfo = formatSteps(parser->optimizationSteps);
                    if (stepInfo.isEmpty())
                        newStatus += QStringLiteral("<center>Initializing...</center>");
                    else
                        newStatus += stepInfo;

                    emit statusMessage(newStatus);
                }
                else if (parser->tasks.back().type == NWChem::Parser::SectionType::Frequency)
                {
                    if (priorTaskCount != int(parser->tasks.size()))
                    {
                        QString newStatus = generateTaskHeader(nwchemConfig, QStringLiteral("Frequency"));
                        newStatus += QStringLiteral("<center>Initializing...</center>");
                        emit statusMessage(newStatus);
                    }
                }
                else if ((parser->tasks.back().type == NWChem::Parser::SectionType::ModuleDFT) ||
                         (parser->tasks.back().type == NWChem::Parser::SectionType::ModuleSCF))
                {
                    QString newStatus = generateTaskHeader(nwchemConfig, QStringLiteral("Energy"));
                    newStatus += QStringLiteral("<center>Calculating single-point energy...</center>");
                    emit statusMessage(newStatus);
                }
            }
            else if (!parser->tasks.empty() && parser->tasks.back().type == NWChem::Parser::SectionType::Frequency)
            {
                // Frequency sections take a very long time to run so we scan for some additional progress info

                priorLastLine = std::max(parser->sections[parser->tasks.back().section].start, priorLastLine);
                QString frequencyStatusMessage = frequencyFeedback(parser->buffer, priorLastLine);

                if (!frequencyStatusMessage.isEmpty())
                {
                    QString newStatus = generateTaskHeader(nwchemConfig, QStringLiteral("Frequency"));
                    newStatus += QStringLiteral("\n<center>%1</center>").arg(frequencyStatusMessage);
                    emit statusMessage(newStatus);
                }
            }
        }
        else if (NWChemTask::SurfaceGen == state)
        {
            QString newStatus = generateTaskHeader(nwchemConfig, QStringLiteral("DPlot"));
            newStatus += QStringLiteral("<center>Generating density plot...</center>");
            emit statusMessage(newStatus);
        }
    });

    connect(optimizeProcess.get(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitCode != 0 || exitStatus != QProcess::NormalExit)
        {
            readAndLogOutput();
            error = QString::fromUtf8(optimizeProcess->readAllStandardError());
        }
        else
        {
            try {
                parser->parseIncremental(readAndLogOutput());

                auto result = parser->document;
                result.molecule = copyChargeAndBonds(document.molecule, result.molecule);

                // Transfer existing volumes to the new result
                std::swap(document.volumes, result.volumes);
                document = result;

                for (const auto &name: pendingSurfaces)
                {
                    QString cubePath = dir->filePath(QString("molecule_%1.cube").arg(name));
                    if (QFile::exists(cubePath))
                    {
                        auto cube = CubeFile::fromCube(cubePath);
                        document.volumes[name] = cube.volumes.first();
                    }
                }
                pendingSurfaces.clear();

                optimized = true;
            }  catch (QString e) {
                error = e;
            }
        }
        state = NWChemTask::None;
        emit finished();
    });

    QString inputFilename = "molecule.nw";
    QFile file(dir->filePath(inputFilename));
    file.open(QIODevice::WriteOnly); // WriteOnly will truncate the file if it exists

    //TODO: It would be better if we could do all this in one pass, but we can't generate
    //      the dplot bounds until we have a geometry to work from
    if (!optimized)
    {
        state = NWChemTask::GeometryOpt;
        file.write(NWChem::molToOptimize(document.molecule, "molecule", nwchemConfig));
    }
    else if (!pendingSurfaces.empty())
    {
        state = NWChemTask::SurfaceGen;
        for (auto const &surfaceName: pendingSurfaces)
        {
            QString orbitalsBlock;
            if (surfaceName == QStringLiteral("total"))
            {
                orbitalsBlock = QStringLiteral("  spin total;\n");
            }
            else if (surfaceName == QStringLiteral("spindensity"))
            {
                orbitalsBlock = QStringLiteral("  spin spindens;\n");
            }
            else
            {
                QString spin = QStringLiteral("total");
                QString orbitalIdStr;
                if (surfaceName.endsWith("a"))
                {
                    spin = QStringLiteral("alpha");
                    orbitalIdStr = surfaceName.chopped(1);
                }
                else if (surfaceName.endsWith("b"))
                {
                    spin = QStringLiteral("beta");
                    orbitalIdStr = surfaceName.chopped(1);
                }
                else
                {
                    orbitalIdStr = surfaceName;
                }

                bool ok;
                int orbitalId = orbitalIdStr.toInt(&ok);
                if (!ok)
                {
                    error = QStringLiteral("Invailid orbital id for dplot");
                    return false;
                }

                orbitalsBlock = QStringLiteral("  orbitals; 1; %1\n  spin %2\n").arg(orbitalId).arg(spin);
            }

            auto makeBoundsLine = [](float min, float max) {
                float start = std::floor(min*10 - 15);
                float end = std::ceil(max*10 + 15);
                int steps = end - start;
                start /= 10.0;
                end /= 10.0;
                return QStringLiteral("  %1 %2 %3\n").arg(start, 4,'f', 1).arg(end, 4,'f', 1).arg(steps);
            };

            auto bounds = document.molecule.bounds();
            QString boundsBlock;
            boundsBlock += makeBoundsLine(bounds.origin.x(),bounds.origin.x()+bounds.size.x());
            boundsBlock += makeBoundsLine(bounds.origin.y(),bounds.origin.y()+bounds.size.y());
            boundsBlock += makeBoundsLine(bounds.origin.z(),bounds.origin.z()+bounds.size.z());

            // NWChem writes the absolute path of the basis files to its database, so we need to
            // force it to refresh the paths in case the file was last run on another computer.
            QString basisBlock = nwchemConfig.generateBasisSection();

            QString dplotCommand = QStringLiteral(
                        "restart %1\n"
                        "echo\n"
                        "\n"
                        "%2"
                        "\n"
                        "dplot\n"
                        "  title %3\n"
                        "  vectors %1.movecs\n"
                        "  LimitXYZ\n"
                        "%4"
                        "  gaussian\n"
                        "%5"
                        "  output %1_%3.cube\n"
                        "end\n"
                        "\n"
                        "task dplot\n").arg("molecule").arg(basisBlock).arg(surfaceName).arg(boundsBlock).arg(orbitalsBlock);
            file.write(dplotCommand.toUtf8());
        }
    }
    file.close();

    QString command = SystemPaths::nwchemBin();
    QStringList arguments = QProcess::splitCommand(command);
    command = arguments.takeFirst();
    arguments.push_back(inputFilename);

    qDebug() << dir->filePath(inputFilename);
    //    dir->setAutoRemove(false);

    optimizeProcess->setProcessEnvironment(SystemPaths::setupNWChemEnvironment());
    optimizeProcess->start(command, arguments);
    return true;
}

void OptimizerNWChem::kill()
{
    if (!optimizeProcess)
        return;

    // I am unsure why the disconnect is necessary, however without it the QProcess will
    // emit a error signal before the process is actually dead and then hang in its
    // destructor...
    disconnect(optimizeProcess.get(), 0, 0, 0);

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

QString OptimizerNWChem::getError()
{
    return error;
}

bool OptimizerNWChem::initializeOutputDirectory()
{
    if (outputFile && outputFile->isOpen())
        outputFile->close();

    dir.reset(new QTemporaryDir);
    return dir->isValid();
}

bool OptimizerNWChem::initializeLogFile()
{
    if (dir && dir->isValid())
    {
        QString outputPath = dir->filePath("output.txt");
        outputFile.reset(new QFile(outputPath));
        return outputFile->open(QIODevice::ReadWrite | QIODevice::Append);
    }
    return false;
}

QByteArray OptimizerNWChem::readAndLogOutput()
{
    if (!optimizeProcess)
        return {};

    QByteArray output = optimizeProcess->readAllStandardOutput();
    bool outputOk = outputFile && outputFile->isOpen();

    if (!outputOk)
        outputOk = initializeLogFile();

    if (outputOk)
        outputFile->write(output);

    return output;
}
