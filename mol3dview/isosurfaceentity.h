#ifndef ISOSURFACEENTITY_H
#define ISOSURFACEENTITY_H

#include "volumedata.h"

#include <QBuffer>
#include <QColor>
#include <QEntity>
#include <QGeometryRenderer>
#include <QDiffuseSpecularMaterial>
#include <QTransform>

namespace Qt3DCompat {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using namespace Qt3DCore;
#else
using namespace Qt3DRender;
#endif
}

class IsosurfaceEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
public:
    IsosurfaceEntity(Qt3DCore::QEntity *parent = nullptr);

    static IsosurfaceEntity* fromData(VolumeData const &volume, QColor color, float threshold = 0.0f);

    Qt3DExtras::QDiffuseSpecularMaterial *material = nullptr;
    Qt3DCore::QTransform *transform = nullptr;

    Qt3DCompat::QGeometry *geom = nullptr;
    Qt3DRender::QGeometryRenderer *geomRender = nullptr;

    Qt3DCompat::QBuffer *vertexBuffer = nullptr;
    Qt3DCompat::QAttribute *vertexAttr = nullptr;
    Qt3DCompat::QAttribute *normalAttr = nullptr;
    Qt3DCompat::QBuffer *indexBuffer = nullptr;
    Qt3DCompat::QAttribute *indexAttr = nullptr;
};

#endif // ISOSURFACEENTITY_H
