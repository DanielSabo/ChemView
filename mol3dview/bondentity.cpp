#include "mol3dview/bondentity.h"


namespace {
    static const float bondWidth = 0.125;
}

BondEntity::BondEntity(Qt3DCore::QEntity *parent) : BillboardEntity(parent) {
    boundingMesh = new Qt3DExtras::QCuboidMesh();
    addComponent(boundingMesh);
    boundingMesh->setXExtent(bondWidth*2);
    boundingMesh->setYExtent(0.1);
    boundingMesh->setZExtent(bondWidth*2);

    setOrder(1);
}

void BondEntity::setOrder(int newOrder) {
    for (auto e: entities)
         delete e;
    entities.clear();
    bonds.clear();

    order = newOrder;
    float length = vector.length();

    //FIXME: The cuboid mesh is strangely slighly shorter than the requested length
    boundingMesh->setYExtent(length);

    auto makeEntity = [&](float radius) {
        auto entity = new Qt3DCore::QEntity(this);
        auto mesh = new Qt3DExtras::QCylinderMesh();
        mesh->setRadius(radius);
        mesh->setLength(length);
        if (material)
            entity->addComponent(material);
        entity->addComponent(mesh);

        bonds.push_back(mesh);
        entities.push_back(entity);
        return entity;
    };

    if (order == 2)
    {
        float width = bondWidth * 0.7f;
        auto transform_a = new Qt3DCore::QTransform();
        auto transform_b = new Qt3DCore::QTransform();
        transform_a->setTranslation(QVector3D(bondWidth, 0.0f, 0.0f));
        transform_b->setTranslation(QVector3D(-bondWidth, 0.0f, 0.0f));
        auto a = makeEntity(width);
        auto b = makeEntity(width);
        a->addComponent(transform_a);
        b->addComponent(transform_b);

        boundingMesh->setXExtent((width+bondWidth)*2);
    }
    else if (order == 3)
    {
        float width = bondWidth * 0.65f;
        float offset = bondWidth * 1.6f;
        auto transform_a = new Qt3DCore::QTransform();
        auto transform_c = new Qt3DCore::QTransform();
        transform_a->setTranslation(QVector3D(offset, 0.0f, 0.0f));
        transform_c->setTranslation(QVector3D(-offset, 0.0f, 0.0f));
        auto a = makeEntity(width);
        makeEntity(width);
        auto c = makeEntity(width);
        a->addComponent(transform_a);
        c->addComponent(transform_c);

        boundingMesh->setXExtent((width+offset)*2);
    }
    else
    {
        makeEntity(bondWidth);

        boundingMesh->setXExtent(bondWidth*2);
    }
}

void BondEntity::setVector(QVector3D from, QVector3D to)
{
    vector = to - from;

    float length = vector.length();
    for (auto c: bonds)
        c->setLength(length);

    QQuaternion cylOrientation(0.707107, 0.707107, 0, 0); // A 90 degree rotation around the x axis
    auto bondOrientation = QQuaternion::fromDirection(vector.normalized(), {});
    auto bondOffset = vector / 2;
    transform->setTranslation(from + bondOffset);
    transform->setRotation(bondOrientation * cylOrientation);

    emit vectorChanged();
}

void BondEntity::setMaterial(Qt3DRender::QMaterial *mat) {
    if (material)
        for (auto e: entities)
            e->removeComponent(material);

    material = mat;

    if (material)
        for (auto e: entities)
            e->addComponent(material);
}

void BondEntity::updateTransform(const QMatrix4x4 &worldMatrix, QVector3D camPos, QVector3D camUp)
{
    (void)camUp;

    QQuaternion parentRotation = QQuaternion::fromRotationMatrix(worldMatrix.toGenericMatrix<3,3>());
    QVector3D bondVector = parentRotation*vector;
    QVector3D atomOrigin = worldMatrix.column(3).toVector3D();
    // Make an axis aligned billboard along the bond vector
    QVector3D bbY = bondVector.normalized();
    QVector3D bbX = QVector3D::crossProduct(camPos - atomOrigin, bondVector).normalized();
    if (bbX.isNull()) // Bond is colinear with the camera vector
    {
        QQuaternion cylOrientation(0.707107, 0.707107, 0, 0); // A 90 degree rotation around the x axis
        transform->setRotation(parentRotation.inverted()*cylOrientation);
    }
    else
    {
        QVector3D bbZ = QVector3D::crossProduct(bbX, bbY);
        QQuaternion cameraFacingRotation = QQuaternion::fromAxes(bbX, bbY, bbZ);
        transform->setRotation(parentRotation.inverted()*cameraFacingRotation);
    }
}
