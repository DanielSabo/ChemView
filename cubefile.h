#ifndef CUBEFILE_H
#define CUBEFILE_H

#include "moldocument.h"

namespace CubeFile
{
    MolDocument fromCube(QString const &filename);
    MolDocument fromCubeData(QByteArray const &source);
};

#endif // CUBEFILE_H
