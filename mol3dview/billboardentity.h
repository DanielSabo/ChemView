#ifndef BILLBOARDENTITY_H
#define BILLBOARDENTITY_H

#include <QEntity>
#include <QTransform>

class BillboardEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
public:
    BillboardEntity(Qt3DCore::QEntity *parent);
    ~BillboardEntity() {}

    virtual void updatePosition(QVector3D camPos, QVector3D upVec);
    virtual void updateTransform(QMatrix4x4 const &worldMatrix, QVector3D camPos, QVector3D camUp);

    Qt3DCore::QTransform *transform = nullptr;
};

#endif // BILLBOARDENTITY_H
