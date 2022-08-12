#ifndef ARCBALL_H
#define ARCBALL_H

#include <QQuaternion>
#include <QVector3D>

class ArcBall
{
public:
    ArcBall();

    void startMotion(float x, float y);
    void updatePos(float x, float y);
    QQuaternion getRotation();
    void reset();

private:
    QVector3D mapToSphere(float x, float y);

    // Mapped start position
    QVector3D startPos;

    QQuaternion rotation;
};

#endif // ARCBALL_H
