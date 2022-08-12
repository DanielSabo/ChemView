#ifndef BONDENTITY_H
#define BONDENTITY_H

#include "mol3dview/billboardentity.h"

#include <QMaterial>
#include <QCylinderMesh>
#include <QCuboidMesh>

class BondEntity : public BillboardEntity
{
    Q_OBJECT
public:
    BondEntity(Qt3DCore::QEntity *parent = nullptr);
    ~BondEntity() = default;

    void setOrder(int order);
    void setVector(QVector3D from, QVector3D to);
    void setMaterial(Qt3DRender::QMaterial *mat);

    void updateTransform(QMatrix4x4 const &worldMatrix, QVector3D camPos, QVector3D camUp) override;

    QString label;
    int id;
    int order = 0;
    QVector3D vector = QVector3D(1.0f, 0.0f, 0.0f);
    Qt3DRender::QMaterial *material = nullptr;

    Qt3DExtras::QCuboidMesh *boundingMesh;
    QList<Qt3DExtras::QCylinderMesh *> bonds;
    QList<Qt3DCore::QEntity *> entities;

signals:
    void vectorChanged();
};

#endif // BONDENTITY_H
