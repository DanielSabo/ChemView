#include "vector3d.h"
#include <cmath>

QVector3D Vector3D::QVec() {
    return QVector3D(mx, my, mz);
}

double Vector3D::length() const
{
    return sqrt(lengthSquared());
}

double Vector3D::lengthSquared() const
{
    return mx * mx + my * my + mz * mz;
}

Vector3D Vector3D::normalized() const
{
    double l = length();
    return Vector3D(mx / l, my / l, mz / l);
}

void Vector3D::normalize() {
    double l = length();
    mx = mx / l;
    my = my / l;
    mz = mz / l;
}

Vector3D Vector3D::operator-(const Vector3D &b) const
{
    return Vector3D(mx - b.mx, my - b.my, mz - b.mz);
}

Vector3D Vector3D::operator+(const Vector3D &b) const
{
    return Vector3D(mx + b.mx, my + b.my, mz + b.mz);
}

Vector3D Vector3D::operator*(double c) const
{
    return Vector3D(mx * c, my * c, mz * c);
}

Vector3D Vector3D::operator/(double c) const
{
    return Vector3D(mx / c, my / c, mz / c);
}

double Vector3D::dotProduct(Vector3D const &a, Vector3D const &b)
{
    return a.mx * b.mx + a.my * b.my + a.mz * b.mz;
}

Vector3D Vector3D::crossProduct(Vector3D const &a, Vector3D const &b)
{
    Vector3D result;
    result.mx = a.my*b.mz - a.mz*b.my;
    result.my = -(a.mx*b.mz - a.mz*b.mx);
    result.mz = a.mx*b.my - a.my*b.mx;
    return result;
}

Vector3D Vector3D::orthognalVector() const
{
    // https://math.stackexchange.com/a/211195
    Vector3D a = Vector3D(mz, mz, -mx - my);
    Vector3D b = Vector3D(-my - mz, mx, mx);

    if (b.lengthSquared() > a.lengthSquared())
        return b;
    return a;
}
