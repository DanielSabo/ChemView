#ifndef VOLUMEDATA_H
#define VOLUMEDATA_H

#include <QVector>
#include <QMatrix4x4>
#include <QVector3D>

class VolumeData {
public:
    VolumeData();
    VolumeData(int x, int y, int z);

    int size() const
    {
        return xDim*yDim*zDim;
    }
    float getAt(int x, int y, int z) const
    {
        return data[x*yDim*zDim + y*zDim + z];
    }

    int xDim;
    int yDim;
    int zDim;
    QVector<float> data;
    QMatrix4x4 transform;
};

#endif // VOLUMEDATA_H
