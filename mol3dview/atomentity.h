#ifndef ATOMENTITY_H
#define ATOMENTITY_H

#include <QEntity>
#include <QMaterial>
#include <QSphereMesh>
#include <QTransform>

class AtomEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
public:
    AtomEntity(Qt3DCore::QEntity *parent = nullptr);

    void setRadius(float r);
    void setMaterial(Qt3DRender::QMaterial *mat);

    QString label;
    int id;
    Qt3DRender::QMaterial *material = nullptr;
    Qt3DExtras::QSphereMesh *mesh = nullptr;
    Qt3DCore::QTransform *transform = nullptr;
};

#endif // ATOMENTITY_H
