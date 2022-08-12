#include "mol3dview/billboardentity.h"

namespace {
    QMatrix4x4 findWorldMatrix(Qt3DCore::QEntity *e) {
        QMatrix4x4 result;
        if (!e)
            return result;

        auto transforms = e->componentsOfType<Qt3DCore::QTransform>();
        if (!transforms.isEmpty())
            result = transforms.first()->matrix();

        if (Qt3DCore::QEntity *p = e->parentEntity())
            result = findWorldMatrix(p)*result;

        return result;
    }
}

BillboardEntity::BillboardEntity(Qt3DCore::QEntity *parent) : Qt3DCore::QEntity(parent) {
    transform = new Qt3DCore::QTransform();
    addComponent(transform);
}

void BillboardEntity::updatePosition(QVector3D camPos, QVector3D camUp) {
    /* Unfortuanely the transform->worldMatrix value is only updated by a redering job
     * and isn't always current. It theory we could listen to the update signal
     * but it's unclear if it's safe to modify our transform inside the update job
     * so instead we traverse back up our parents and calculate it ourselves.
     * TODO: I might be more efficent have an external function to traverse down
     * the tree and update all the billboards recalculating the parent world matrix
     * for each billboard.
     */
    auto worldMatrix = findWorldMatrix(parentEntity());
    updateTransform(worldMatrix, camPos, camUp);
}

void BillboardEntity::updateTransform(const QMatrix4x4 &worldMatrix, QVector3D camPos, QVector3D camUp)
{
    QQuaternion parentRotation = QQuaternion::fromRotationMatrix(worldMatrix.toGenericMatrix<3,3>());
    QVector3D atomOrigin = worldMatrix.column(3).toVector3D();
    QQuaternion cameraFacingRotation = QQuaternion::fromDirection(camPos - atomOrigin, camUp);
    QQuaternion planeUpRotation(0.707107, 0.707107, 0, 0); // A 90 degree rotation around the x axis
    transform->setRotation(parentRotation.inverted()*cameraFacingRotation*planeUpRotation);
}
