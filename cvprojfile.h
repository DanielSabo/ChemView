#ifndef CVPROJFILE_H
#define CVPROJFILE_H

#include "moldocument.h"

#include <QByteArray>
#include <QString>

class OptimizerNWChem;

namespace CVJSONFile
{
    MolDocument fromPath(QString const &filename);
    MolDocument fromData(QByteArray const &source);
    QByteArray write(MolDocument const &document);
}

namespace CVProjFile
{
    MolDocument fromPath(QString const &filename);
    bool write(QIODevice *file, MolDocument const &document);
    bool write(QIODevice *file, MolDocument const &document, const OptimizerNWChem &nwchem);
}

#endif // CVPROJFILE_H
