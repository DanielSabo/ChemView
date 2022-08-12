#include "volumedata.h"
#include <QDebug>

VolumeData::VolumeData() : xDim(0), yDim(0), zDim(0)
{

}

VolumeData::VolumeData(int x, int y, int z) : xDim(x), yDim(y), zDim(z)
{
    data.resize(xDim*yDim*zDim);
}
