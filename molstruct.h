#ifndef MOLSTRUCT_H
#define MOLSTRUCT_H

#include <QList>
#include <QString>
#include <QVector3D>
#include "vector3d.h"

struct Atom
{
    double x = 0;
    double y = 0;
    double z = 0;
    QString element = "H";
    int charge = 0;

    QVector3D posToVector() const { return QVector3D(x, y, z); }
    Vector3D posToVector3D() const { return Vector3D(x, y, z); }
    void setPos(QVector3D const &v) { x = v.x(); y = v.y(); z = v.z(); }
    void setPos(Vector3D const &v) { x = v.x(); y = v.y(); z = v.z(); }
};

struct Bond
{
    Bond() : from(-1), to(-1), order(1) {}
    Bond(int from, int to, int order = 1) : from(from), to(to), order(order) {}

    int from;
    int to;
    int order;
};

class MolStructGraph;
class MolStruct
{
public:
    MolStruct();

    static MolStruct fromSDF(QString const &filename);
    static MolStruct fromXYZ(QString const &filename);

    static MolStruct fromSDFData(QByteArray const &source);
    static MolStruct fromXYZData(QByteArray const &source);

    QList<Atom> atoms;
    QList<Bond> bonds;

    struct Bounds {
        QVector3D origin;
        QVector3D size;
    };

    bool isEmpty();
    Bounds bounds();
    // Return the bond id for the bond between 'to' and 'from', or -1 if they're not bonded
    int findBondForPair(int from, int to);
    // Return all the bonds attahed to atom 'id'
    QList<int> findBondsForAtom(int id);

    // Returns true if the atom has only one bond
    bool isLeafAtom(int id);
    // Returns true if the atom has only one non-hydrogen bond and no rings
    bool isLeafGroup(int id);
    void deleteAtom(int id);
    void deleteBond(int id);

    void recenter(Vector3D offset = {});
    void recenterOn(int id);

    bool hydrogensToBond(int from, int to);
    // Remove an atom and its valences after breaking any bonds connecting to it
    void eraseGroup(int id);
    bool replaceLeafWithRGroup(int id, MolStruct rGroup);
    // Add another MolStruct to this one as an unconnected fragment
    void addFragment(MolStruct fragment);
    // Add an automatically positioned hydrogen to an atom
    void addHydrogenToAtom(int id);
    bool addHydrogenToAtom(int id, QVector3D orientation);
    QList<int> replaceElement(QString from, QString to);

    bool percieveBonds();

    MolStructGraph generateGraph() const;

    QByteArray toMolFile();
    QByteArray toXYZFile();
};

#endif // MOLSTRUCT_H
