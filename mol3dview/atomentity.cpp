#include "mol3dview/atomentity.h"

AtomEntity::AtomEntity(Qt3DCore::QEntity *parent) : Qt3DCore::QEntity(parent) {
    mesh = new Qt3DExtras::QSphereMesh();
    mesh->setGenerateTangents(true);
    mesh->setRings(24);
    mesh->setSlices(24);
    transform = new Qt3DCore::QTransform();
    addComponent(mesh);
    addComponent(transform);
}

void AtomEntity::setRadius(float r) {
    mesh->setRadius(r);
}

void AtomEntity::setMaterial(Qt3DRender::QMaterial *mat) {
    if (material)
        removeComponent(material);
    material = mat;
    addComponent(material);
}
