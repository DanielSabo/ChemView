#include "cubefile.h"
#include "element.h"
#include "parsehelpers.h"

#include <QDebug>
#include <QRegularExpression>

MolDocument CubeFile::fromCube(const QString &filename)
{
    QByteArray source;

    try {source = readFile(filename); }
    catch (QString err) { throw QStringLiteral("Cube: ") + err; }

    return fromCubeData(source);
}

MolDocument CubeFile::fromCubeData(const QByteArray &source)
{
    /* Reference: http://paulbourke.net/dataformats/cube/ */

    MolDocument result;

    QStringList data = QString(source).split(QRegularExpression("[\r\n]"));
    auto data_iter = data.begin();
    data_iter++; // Comment line
    data_iter++; // Comment line
    QStringList line = (data_iter++)->split(" ", Qt::SkipEmptyParts);
    int numAtoms = intFromList(line, 0);
    QVector3D origin(doubleFromList(line, 1), doubleFromList(line, 2) , doubleFromList(line, 3));

    line = (data_iter++)->split(" ", Qt::SkipEmptyParts);
    int xPoints = intFromList(line, 0);
    double unitScale = 0.529177;
    if (xPoints < 0)
        unitScale = 1.0;
    origin = origin * unitScale;
    QVector3D xStep = QVector3D(doubleFromList(line, 1),
                                doubleFromList(line, 2),
                                doubleFromList(line, 3)) * unitScale;

    line = (data_iter++)->split(" ", Qt::SkipEmptyParts);
    int yPoints = intFromList(line, 0);
    QVector3D yStep = QVector3D(doubleFromList(line, 1),
                                doubleFromList(line, 2),
                                doubleFromList(line, 3)) * unitScale;

    line = (data_iter++)->split(" ", Qt::SkipEmptyParts);
    int zPoints = intFromList(line, 0);
    QVector3D zStep = QVector3D(doubleFromList(line, 1),
                                doubleFromList(line, 2),
                                doubleFromList(line, 3)) * unitScale;

    xPoints = abs(xPoints);
    yPoints = abs(yPoints);
    zPoints = abs(zPoints);

    while (numAtoms--)
    {
        line = (data_iter++)->split(" ", Qt::SkipEmptyParts);
        Atom a;
        Element e = Element::fromAtomicNumber(doubleFromList(line, 0));
        a.element = e.abbr;
        a.charge = doubleFromList(line, 1) - e.number;
        a.x = doubleFromList(line, 2) * unitScale;
        a.y = doubleFromList(line, 3) * unitScale;
        a.z = doubleFromList(line, 4) * unitScale;

        result.molecule.atoms.push_back(a);
    }

    VolumeData volume;

    volume.data.reserve(xPoints * yPoints * zPoints);
    volume.xDim = xPoints;
    volume.yDim = yPoints;
    volume.zDim = zPoints;

    while (data_iter != data.end())
    {
        line = (data_iter++)->split(" ", Qt::SkipEmptyParts);
        for (auto const &s: line)
            volume.data.push_back(toDoubleOrThrow(s));
    }

    if ((int)volume.data.size() != xPoints * yPoints * zPoints)
        throw QString("CubeFile: Expected %1 datapoints but found %2").arg(xPoints * yPoints * zPoints).arg(volume.data.size());

    volume.transform = QMatrix4x4(xStep.x(), yStep.x(), zStep.x(), origin.x(),
                                         xStep.y(), yStep.y(), zStep.y(), origin.y(),
                                         xStep.z(), yStep.z(), zStep.z(), origin.z(),
                                         0, 0, 0, 1);

    result.volumes["default"] = volume;
    result.molecule.percieveBonds();
    return result;
}
