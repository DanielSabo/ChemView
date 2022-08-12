#ifndef OPTIMIZERNWCHEM_H
#define OPTIMIZERNWCHEM_H

#include "linebuffer.h"
#include "nwchemconfiguration.h"
#include "optimizer.h"
#include "qzipreader.h"
#include "qzipwriter.h"

#include <memory>

class QTemporaryDir;
class QProcess;
namespace NWChem {
    class Parser;
}

class OptimizerNWChem : public Optimizer
{
    Q_OBJECT
public:
    explicit OptimizerNWChem(QObject *parent = nullptr);
    ~OptimizerNWChem();

    void saveToProjFile(QZipWriter &zipfile, QString dirname) const;
    static std::unique_ptr<OptimizerNWChem> fromProjFile(QString const &filename, QString dirname, MolStruct m);

    void setConfiguration(NWChemConfiguration config);
    NWChemConfiguration getConfiguration();
    void requestSurface(QString name);
    void setStructure(MolStruct m) override;
    MolStruct getStructure() override;
    MolDocument getResults() override;
    QString getLogFile() override;
    bool run() override;
    void kill() override;
    QString getError() override;

protected:
    bool initializeOutputDirectory();
    bool initializeLogFile();
    QByteArray readAndLogOutput();

    enum class NWChemTask {
        None,
        GeometryOpt,
        SurfaceGen
    };

    NWChemTask state = NWChemTask::None;
    bool optimized = false;
    std::unique_ptr<QProcess> optimizeProcess;
    std::unique_ptr<QTemporaryDir> dir;
    NWChemConfiguration nwchemConfig;
    MolDocument document;
    QString error;
    QList<QString> pendingSurfaces;

    std::unique_ptr<NWChem::Parser> parser;
    std::unique_ptr<QFile> outputFile;
    LineBuffer outputLineBuffer;
};

#endif // OPTIMIZERNWCHEM_H
