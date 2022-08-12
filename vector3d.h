#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <QVector3D>

class Vector3D {
public:
    Vector3D() = default;
    Vector3D(double x, double y, double z) : mx(x), my(y), mz(z) {}
    explicit Vector3D(QVector3D const &v) : mx(v.x()), my(v.y()), mz(v.z()) {}

    double mx = 0.0;
    double my = 0.0;
    double mz = 0.0;

    inline double x() const { return mx; }
    inline double y() const { return my; }
    inline double z() const { return mz; }

    QVector3D QVec();

    double length() const;
    double lengthSquared() const;
    Vector3D normalized() const;
    void normalize();

    Vector3D operator-(Vector3D const &b) const;
    Vector3D operator+(Vector3D const &b) const;
    Vector3D operator*(double c) const;
    Vector3D operator/(double c) const;

    static double dotProduct(Vector3D const &a, Vector3D const &b);
    static Vector3D crossProduct(Vector3D const &a, Vector3D const &b);

    // Generate an arbitrary perpendicular vector
    Vector3D orthognalVector() const;
};

#endif // VECTOR3D_H
