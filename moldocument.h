#ifndef MOLDOCUMENT_H
#define MOLDOCUMENT_H

#include "molstruct.h"
#include "volumedata.h"

#include <QMap>
#include <QVector>

class MolDocument
{
public:
    MolDocument();
    explicit MolDocument(MolStruct molecule);

public:
    struct MolecularOrbital {
        MolecularOrbital();
        MolecularOrbital(QString id, double occupancy, double energy, QString symmetry);

        QString id;
        double occupancy;
        double energy;
        QString symmetry;
    };

    struct Frequency {
        float wavenum;
        float intensity;
        QVector<QVector3D> eigenvector;
    };

    MolStruct molecule;
    //TODO: Should we separate volumes we know the purpose of (e.g. the orbitals) from ones
    //      that we find as part of a file (e.g. cube files)?
    QMap<QString, VolumeData> volumes;
    QList<MolecularOrbital> orbitals;
    QList<Frequency> frequencies;

    QMap<QString, QString> calculatedProperties;
};

#endif // MOLDOCUMENT_H
