#include "arcball.h"
#include <cmath>

// Based on math found in https://graphicsinterface.org/wp-content/uploads/gi1992-18.pdf

ArcBall::ArcBall()
{

}

void ArcBall::startMotion(float x, float y) {
    startPos = mapToSphere(x, y);
}

void ArcBall::updatePos(float x, float y) {
    QVector3D currentPos = mapToSphere(x, y);

    QVector3D quat_xyz = QVector3D::crossProduct(startPos, currentPos);
    float quat_w = QVector3D::dotProduct(startPos, currentPos);
    rotation = QQuaternion(QVector4D(quat_xyz, quat_w));
}

QQuaternion ArcBall::getRotation()
{
    return rotation;
}

void ArcBall::reset()
{
    rotation = QQuaternion();
}

QVector3D ArcBall::mapToSphere(float x, float y) {
    QVector3D result(x, y, 0.0f);
    float mag = result.lengthSquared();
    if (mag > 1.0f) {
        result /= sqrt(mag);
    } else {
        result.setZ(sqrt(1.0f - mag));
    }

    return result;
}
