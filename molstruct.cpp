#include "molstruct.h"
#include "element.h"
#include "parsehelpers.h"
#include "molstructgraph.h"

#include <QDebug>
#include <QFile>
#include <QQuaternion>
#include <algorithm>
#include <functional>
#include <cmath>
#include <QRegularExpression>

namespace {
int parseMolfileCharge(int value) {
    switch (value) {
    case 1:
        return 3;
    case 2:
        return 2;
    case 3:
        return 1;
    case 4:
        throw QString("SDF read error, unsupported charge value");
    case 5:
        return -1;
    case 6:
        return -2;
    case 7:
        return -3;
    default:
        return 0;
    }
}
int writeMolfileCharge(int value) {
    switch (value) {
    case 3:
        return 1;
    case 2:
        return 2;
    case 1:
        return 3;
    case -1:
        return 5;
    case -2:
        return 6;
    case -3:
        return 7;
    default:
        return 0;
    }
}
}

MolStruct::MolStruct()
{

}

MolStruct MolStruct::fromSDFData(const QByteArray &source)
{
    /*
     * https://depth-first.com/articles/2020/07/13/the-sdfile-format/
     * https://chem.libretexts.org/Courses/University_of_Arkansas_Little_Rock/ChemInformatics_(2017)%3A_Chem_4399_5399/2.2%3A_Chemical_Representations_on_Computer%3A_Part_II/2.2.2%3A_Anatomy_of_a_MOL_file
     */

    // Since we just want to parse the pubchem data we're pretending only V2000 files exist

    MolStruct result;
    QStringList data = QString(source).split(QRegularExpression("[\r\n]"));
    auto data_iter = data.begin();

    for (int i = 0; i < 3; i++)
    {
        if (data_iter == data.end())
            throw QString("SDF read error, missing header lines");
        data_iter++;
    }

    if (data_iter == data.end())
        throw QString("SDF read error, missing counts line");

    // Line format:
    //aaabbblllfffcccsssxxxrrrpppiiimmmvvvvvv
    QStringList counts_line = splitFixedWidth(*data_iter++, {3, 3});
    int num_atoms = intFromList(counts_line, 0);
    int num_bonds = intFromList(counts_line, 1);

    for (int i = 0; i < num_atoms; i++)
    {
        Atom a;
        // Line format:
        // xxxxx.xxxxyyyyy.yyyyzzzzz.zzzz aaaddcccssshhhbbbvvvHHHrrriiimmmnnneee
        QStringList atom_line = splitFixedWidth(*data_iter++, {11, 10, 10, 4, 2, 3});
        a.x = doubleFromList(atom_line, 0);
        a.y = doubleFromList(atom_line, 1);
        a.z = doubleFromList(atom_line, 2);
        a.element = atOrThrow(atom_line,3).trimmed();
        // skipped: mass difference
        a.charge = parseMolfileCharge(doubleFromList(atom_line, 5));

        result.atoms.push_back(a);
//        qDebug() << "ATOM:" << atom_line;
    }
    for (int i = 0; i < num_bonds; i++)
    {
        Bond b;
        // Line format:
        //111222tttsssxxxrrrccc
        QStringList bond_line = splitFixedWidth(*data_iter++, {3, 3, 3});
        b.from = intFromList(bond_line, 0) - 1;
        b.to = intFromList(bond_line, 1) - 1;
        b.order = intFromList(bond_line, 2);
        if (b.from >= 0 && b.from < result.atoms.size() &&
            b.to >= 0 && b.to < result.atoms.size())
        {
            result.bonds.push_back(b);
        }
        else
        {
            qDebug() << "invalid bond:" << bond_line;
        }
    }
    while (data_iter != data.end())
    {
        QString line = *data_iter++;
        if (line.startsWith("M  END"))
            break;
        else if (line.startsWith("M  CHG"))
        {
            // The actual format is "M  CHGnnn " where n is the number of entires but we ignore it for now
            // The values are fixed width but space separated
            line.remove(0, 9);
            QStringList charge_line = line.split(" ", Qt::SkipEmptyParts);
            if (charge_line.length() % 2 != 0)
                throw QString("Invalid M  CHG line");
            auto charge_iter = charge_line.begin();
            while (charge_iter != charge_line.end())
            {
                int id = toIntOrThrow(*charge_iter++) - 1;
                int charge = toIntOrThrow(*charge_iter++);
                if (id >= result.atoms.length() || id < 0)
                    throw QString("Invalid M  CHG atom id");
                result.atoms[id].charge = charge;
            }
        }
    }

    return result;
}

MolStruct MolStruct::fromXYZData(const QByteArray &source)
{
    MolStruct result;
    QStringList data = QString(source).split(QRegularExpression("[\r\n]"));
    auto data_iter = data.begin();

    int numAtoms = toIntOrThrow(*data_iter++);
    data_iter++; // Comment line
    for (; data_iter != data.end() && result.atoms.length() != numAtoms; data_iter++)
    {
        // Line format: <element> <x> <y> <z>
        QStringList line = data_iter->split(" ", Qt::SkipEmptyParts);
        if (line.size() == 0)
            continue;
        Atom a;
        a.element = atOrThrow(line, 0).trimmed();
        a.x = doubleFromList(line, 1);
        a.y = doubleFromList(line, 2);
        a.z = doubleFromList(line, 3);

        result.atoms.push_back(a);
    }

    if (result.atoms.length() != numAtoms)
        qDebug() << "XYZ: Expected" << numAtoms << "atoms but found" << result.atoms.length();

    result.percieveBonds();

    return result;
}

MolStruct MolStruct::fromSDF(QString const &filename)
{
    QByteArray source;

    try {source = readFile(filename); }
    catch (QString err) { throw QStringLiteral("SDF: ") + err; }

    return fromSDFData(source);
}

MolStruct MolStruct::fromXYZ(const QString &filename)
{
    QByteArray source;

    try {source = readFile(filename); }
    catch (QString err) { throw QStringLiteral("XYZ: ") + err; }

    return fromXYZData(source);
}

bool MolStruct::isEmpty()
{
    return atoms.isEmpty();
}

MolStruct::Bounds MolStruct::bounds()
{
    if (atoms.isEmpty())
        return {};

    QVector3D minCoord(Q_INFINITY,Q_INFINITY,Q_INFINITY);
    QVector3D maxCoord(-Q_INFINITY,-Q_INFINITY,-Q_INFINITY);
    for (auto const &a: atoms)
    {
        if (a.x < minCoord.x())
            minCoord.setX(a.x);
        if (a.y < minCoord.y())
            minCoord.setY(a.y);
        if (a.z < minCoord.z())
            minCoord.setZ(a.z);
        if (a.x > maxCoord.x())
            maxCoord.setX(a.x);
        if (a.y > maxCoord.y())
            maxCoord.setY(a.y);
        if (a.z > maxCoord.z())
            maxCoord.setZ(a.z);
    }

    return {minCoord, maxCoord-minCoord};
}

int MolStruct::findBondForPair(int from, int to)
{
    for (int i = 0; i < bonds.size(); i++)
    {
        if (bonds[i].to == to && bonds[i].from == from)
            return i;
        else if (bonds[i].to == from && bonds[i].from == to)
            return i;
    }
    return -1;

}

QList<int> MolStruct::findBondsForAtom(int id)
{
    QList<int> result;

    for (int i = 0; i < bonds.size(); i++)
    {
        if (bonds[i].to == id || bonds[i].from == id)
            result.push_back(i);
    }

    return result;
}

bool MolStruct::isLeafAtom(int id)
{
    int bondCount = 0;
    for (auto const &bond: bonds)
    {
        if (bond.to == id || bond.from == id)
            bondCount++;
        if (bondCount > 1)
            return false;
    }
    return true;
}

bool MolStruct::isLeafGroup(int id)
{
    int bondCount = 0;
    for (auto const &bond: bonds)
    {
        if (bond.to == id && (atoms[bond.from].element != "H" || !isLeafAtom(bond.from)))
            bondCount++;
        else if (bond.from == id && (atoms[bond.to].element != "H" || !isLeafAtom(bond.to)))
            bondCount++;
        if (bondCount > 1)
            return false;
    }
    return true;
}

namespace {

// Assumes id is a leaf atom
QVector3D bondVectorTo(int id, const MolStruct &mol)
{
    int startId = 0;
    int endId = 0;
    for (auto const &bond: mol.bonds)
    {
        if (bond.to == id)
        {
            startId = bond.from;
            endId = bond.to;
            break;
        }
        else if (bond.from == id)
        {
            startId = bond.to;
            endId = bond.from;
            break;
        }
    }

    auto const &start = mol.atoms.at(startId);
    auto const &end = mol.atoms.at(endId);
    return end.posToVector() - start.posToVector();
}

// Assumes id is a leaf atom
int findBondParter(int id, const MolStruct &mol)
{
    for (auto const &bond: mol.bonds)
    {
        if (bond.to == id)
            return bond.from;
        else if (bond.from == id)
            return bond.to;
    }
    return -1;
}

}

void MolStruct::eraseGroup(int id)
{
    auto atomBonds = findBondsForAtom(id);

    // Given a bond with this atom return the id of the other atom
    auto otherAtom = [&](int bondId, int atomId) -> int {
        if (bonds[bondId].to == atomId)
            return bonds[bondId].from;
        return bonds[bondId].to;
    };

    QList<int> atomsToDelete;
    QList<int> bondsToReplace;

    // Valences will be deleted, other bonds converted back to valences
    for (int b: atomBonds)
    {
        int other = otherAtom(b, id);

        if (atoms[other].element == QStringLiteral("H"))
            atomsToDelete.push_back(other);
        else
            bondsToReplace.push_back(b);
    }

    // Save the original position of this atom to define the bond orientations
    QVector3D atomPos = atoms[id].posToVector();

    auto restoreValence = [&](int from, int to, int bondId) {
        float bondLength = Element(1).estimateBondLength(Element(atoms[from].element));
        QVector3D fromPos = atoms[from].posToVector();
        QVector3D bondVector = atomPos - fromPos;
        atoms[to].setPos(bondVector.normalized() * bondLength + fromPos);

        // Reduce the bond order to 1
        while (bonds[bondId].order > 1)
        {
            bonds[bondId].order -= 1;
            addHydrogenToAtom(from);
        }
    };

    auto iter = bondsToReplace.begin();

    // Were there any non-valence bonds?
    if (iter != bondsToReplace.end())
    {
        int other = otherAtom(*iter, id);

        // If so the first one can remain connected to this id
        atoms[id].element = "H";
        atoms[id].charge = 0;

        restoreValence(other, id, *iter);

        iter++;
    }

    // For the rest we generate new hydrogens
    for (; iter != bondsToReplace.end(); iter++)
    {
        int other = otherAtom(*iter, id);

        Atom a = atoms[id];
        int newId = atoms.size();
        atoms.push_back(a);
        if (bonds[*iter].from == id)
            bonds[*iter].from = newId;
        else
            bonds[*iter].to = newId;

        restoreValence(other, newId, *iter);
    }

    // Delete things last to avoid disrupting the bond list indicies

    // The atom had no non-valence bonds
    if (bondsToReplace.isEmpty())
        atomsToDelete.push_back(id);

    // Sort greatest to least so deleting won't disrupt the atom indicies while iterating
    std::sort(atomsToDelete.begin(), atomsToDelete.end(), std::greater<int>());
    for (int atom: atomsToDelete)
        deleteAtom(atom);
}

bool MolStruct::replaceLeafWithRGroup(int id, MolStruct rGroup)
{
    if (!isLeafAtom(id))
        return false;

    int originAtomId = findBondParter(id, *this);
    if (originAtomId < 0)
    {
        qDebug() << "originAtomId";
        return false;
    }

    int rGroupId = 0;
    for (; rGroupId < rGroup.atoms.size(); ++rGroupId)
    {
        if (rGroup.atoms.at(rGroupId).element == "R1")
            break;
    }
    if (rGroupId >= rGroup.atoms.size())
    {
        qDebug() << "Missing R group";
        return false;
    }

    int groupTargetId = findBondParter(rGroupId, rGroup);
    if (groupTargetId < 0)
    {
        qDebug() << "groupTargetId";
        return false;
    }

    // Find the bond for the atom we're replacing
    QVector3D bondVector = bondVectorTo(id, *this);
    auto bondOrientation = QQuaternion::fromDirection(bondVector.normalized(), {});

    // Find the bond vector for the R1 element (flipped)
    QVector3D groupVector = bondVectorTo(rGroupId, rGroup) * -1.0;
    auto groupOrientation = QQuaternion::fromDirection(groupVector.normalized(), {});
    auto groupTransform = bondOrientation*groupOrientation.inverted();

    float bondLength = Element(atoms[originAtomId].element).estimateBondLength(Element(rGroup.atoms[groupTargetId].element));

    // Center the group on the R1 group's partner atom
    rGroup.recenterOn(groupTargetId);
    QVector3D offset = bondVector.normalized() * bondLength + atoms.at(originAtomId).posToVector();

    // Delete the R1 atom from the R group
    //TODO: Preserve group bond order?
    rGroup.deleteAtom(rGroupId);

    for (auto &atom: rGroup.atoms)
    {
        auto rotated = groupTransform.rotatedVector(atom.posToVector());
        atom.setPos(rotated + offset);
    }

    if (originAtomId > id)
        originAtomId--;
    deleteAtom(id);

    QMap<int, int> atomMapping;
    for (int i = 0; i < rGroup.atoms.size(); ++i)
    {
        atomMapping[i] = atoms.size();
        atoms.append(rGroup.atoms.at(i));
    }

    for (auto &bond: rGroup.bonds)
    {
        Bond newBond;
        newBond.order = bond.order;
        newBond.from = atomMapping.value(bond.from);
        newBond.to = atomMapping.value(bond.to);
        bonds.append(newBond);
    }

    // Join the group to the molecule
    Bond newBond;
    newBond.order = 1;
    newBond.from = originAtomId;
    newBond.to = atomMapping.value(groupTargetId);
    bonds.append(newBond);

    return true;
}

void MolStruct::addFragment(MolStruct fragment)
{
    QMap<int, int> atomMapping;
    for (int i = 0; i < fragment.atoms.size(); ++i)
    {
        atomMapping[i] = atoms.size();
        atoms.append(fragment.atoms.at(i));
    }

    for (auto &bond: fragment.bonds)
    {
        Bond newBond;
        newBond.order = bond.order;
        newBond.from = atomMapping.value(bond.from);
        newBond.to = atomMapping.value(bond.to);
        bonds.append(newBond);
    }
}

void MolStruct::addHydrogenToAtom(int id)
{
    // Pretty sure there's a smarter way to generate candidate points
    // from the bond vectors but for now we just brute force it using
    // a fibonacci sphere.
    // https://stackoverflow.com/questions/9600801/evenly-distributing-n-points-on-a-sphere

    QVector3D originVec = atoms[id].posToVector();
    std::vector<QVector3D> bondNormals;
    for (auto &b: bonds)
    {
        if (b.to == id)
            bondNormals.push_back((atoms[b.from].posToVector() - originVec).normalized());
        else if (b.from == id)
            bondNormals.push_back((atoms[b.to].posToVector() - originVec).normalized());
    }

    if (bondNormals.size() == 0)
    {
        addHydrogenToAtom(id, QVector3D(0.0f, 0.0f, 1.0f));
        return;
    }
    if (bondNormals.size() == 1)
    {
        addHydrogenToAtom(id, -bondNormals.front());
        return;
    }

    float farthestDist = 0;
    QVector3D farthestVec(0.0f, 0.0f, 1.0f);

    // Generate a set of set points on the surface of the unit sphere
    const float phi = 2.399963229728653f; // Golden angle
    const int numTestPoints = 1000;
    for (int i = 0; i < numTestPoints; ++i)
    {
        float y = 1 - (i / float(numTestPoints - 1)) * 2; // y goes from 1 to -1
        float radius = sqrt(1 - y * y); // radius of the x/z plane circle at y
        float theta = phi * i; // golden angle increment
        float x = cosf(theta) * radius;
        float z = sinf(theta) * radius;

        QVector3D testVec(x, y, z);

        // Determine the distance of this point from the closest bond vector
        float distance = 10; // In radians, so the true max is 2*pi
        for (auto const &bvec: bondNormals)
            distance = std::min(distance, acosf(QVector3D::dotProduct(testVec, bvec)));
        if (distance > farthestDist)
        {
            farthestDist = distance;
            farthestVec = testVec;
        }
    }

    addHydrogenToAtom(id, farthestVec);
}

bool MolStruct::addHydrogenToAtom(int id, QVector3D orientation)
{
    float bondLength = Element(atoms[id].element).estimateBondLength(Element(1));
    Atom a;
    a.element = "H";
    a.setPos(orientation.normalized() * bondLength + atoms[id].posToVector());
    atoms.append(a);
    Bond b;
    b.order = 1;
    b.from = id;
    b.to = atoms.size() - 1;
    bonds.append(b);

    return true;
}

QList<int> MolStruct::replaceElement(QString from, QString to)
{
    QList<int> result;
    for (int i = 0; i < atoms.size(); ++i)
    {
        if (atoms[i].element == from)
        {
            atoms[i].element = to;
            result.append(i);
        }
    }
    return result;
}

bool MolStruct::percieveBonds()
{
    /* Per https://en.wikipedia.org/wiki/XYZ_file_format two atoms are considered bonded if the distance
     * between them is less than their covalent radii. Avogadro also seems to also exclude H-H bonds from
     * consideration but I'm not sure how I feel about that.
     */

    // Copying this totally arbitrary value from Avogadro because they've seen more molecules than I have.
    float fudgeFactor = 0.45;

    bonds.clear();

    for (int i = 0; i < atoms.length(); ++i)
        for (int j = i + 1; j < atoms.length(); ++j)
        {
            float distSqr = (atoms[j].posToVector() - atoms[i].posToVector()).lengthSquared();
            float bondDist = Element::fromAbbr(atoms[i].element).covalentRadius()
                    + Element::fromAbbr(atoms[j].element).covalentRadius()
                    + fudgeFactor;

            if (distSqr <= bondDist*bondDist)
                bonds.push_back(Bond(i, j));
        }

    return true;
}

MolStructGraph MolStruct::generateGraph() const
{
    return MolStructGraph(*this);
}

QByteArray MolStruct::toMolFile()
{
    // Note: The mol file format uses fixed width fields
    QString result;
    result += "\n\n\n";
    result += QString("%1%2").arg(atoms.size(),3).arg(bonds.size(),3) + QStringLiteral("  0  0  0  0  0  0  0  0999 V2000\n");
    for (auto const &atom: atoms)
    {
        result += QString("%1%2%3%4%5%6").arg(atom.x,10,'f',4).arg(atom.y,10,'f',4).arg(atom.z,10,'f',4).arg(atom.element,4)
                                         .arg(0,2).arg(writeMolfileCharge(atom.charge),3) + QStringLiteral("  0  0  0  0  0  0  0  0  0  0\n");
    }

    for (auto const &bond: bonds)
    {
        result += QString("%1%2%3").arg(bond.from + 1,3).arg(bond.to + 1,3).arg(bond.order,3) + QStringLiteral("  0  0  0  0\n");
    }

    { // Write charge
        QString currentRow;
        int currentCount = 0;
        for (int i = 0; i < atoms.size(); i++)
        {
            if (atoms[i].charge != 0)
            {
                currentRow += QString(" %1 %2").arg(i+1,3).arg(atoms[i].charge,3);
                currentCount++;

                if (currentCount == 8)
                {
                    result += QString("M  CHG  8")+currentRow+QString("\n");
                    currentRow = QString();
                    currentCount = 0;
                }
            }
        }
        if (!currentRow.isEmpty())
            result += QString("M  CHG%1").arg(currentCount,3)+currentRow+QString("\n");
    }

    result += QStringLiteral("M  END\n");
    return result.toUtf8();
}

QByteArray MolStruct::toXYZFile()
{
    QStringList result = {QStringLiteral("%1\n").arg(atoms.length())};
    for (auto &a: atoms)
        result.append(QStringLiteral("%1 %2 %3 %4").arg(a.element).arg(a.x,0,'f').arg(a.y,0,'f').arg(a.z,0,'f'));

    return result.join('\n').toUtf8();
}

void MolStruct::deleteAtom(int id)
{
    auto iter = bonds.begin();
    while (iter != bonds.end())
    {
        if (iter->from == id || iter->to == id)
        {
            iter = bonds.erase(iter);
        }
        else
        {
            if (iter->from > id)
                iter->from -= 1;
            if (iter->to > id)
                iter->to -= 1;
            iter++;
        }
    }
    atoms.removeAt(id);
}

void MolStruct::deleteBond(int id)
{
    // Nothing references bond IDs so we can just delete it
    bonds.removeAt(id);
}

void MolStruct::recenter(Vector3D offset)
{
    if (atoms.empty())
        return;

    double min_x = atoms[0].x;
    double min_y = atoms[0].y;
    double min_z = atoms[0].z;
    double max_x = atoms[0].x;
    double max_y = atoms[0].y;
    double max_z = atoms[0].z;

    for (auto &atom: atoms)
    {
        min_x = std::min(min_x, atom.x);
        min_y = std::min(min_y, atom.y);
        min_z = std::min(min_z, atom.z);
        max_x = std::max(max_x, atom.x);
        max_y = std::max(max_y, atom.y);
        max_z = std::max(max_z, atom.z);
    }

    double center_x = (max_x - min_x)/2.0 + min_x;
    double center_y = (max_y - min_y)/2.0 + min_y;
    double center_z = (max_z - min_z)/2.0 + min_z;

    center_x -= offset.x();
    center_y -= offset.y();
    center_z -= offset.z();

    for (auto &atom: atoms)
    {
        atom.x -= center_x;
        atom.y -= center_y;
        atom.z -= center_z;
    }
}

void MolStruct::recenterOn(int id)
{
    Atom center = atoms.at(id);
    for (auto &atom: atoms)
    {
        atom.x -= center.x;
        atom.y -= center.y;
        atom.z -= center.z;
    }
}

bool MolStruct::hydrogensToBond(int from, int to)
{
    if (to == from)
        return false;
    if (!(isLeafAtom(to) && atoms[to].element == "H"))
        return false;
    if (!(isLeafAtom(from) && atoms[from].element == "H"))
        return false;

    int toOrigin = findBondParter(to, *this);
    int fromOrigin = findBondParter(from, *this);

    if ((toOrigin == -1) || (fromOrigin == -1))
        return false;
    if (toOrigin == fromOrigin)
        return false;

    int existingBond = findBondForPair(toOrigin, fromOrigin);
    if (existingBond == -1)
        bonds.append(Bond(fromOrigin, toOrigin));
    else if (bonds[existingBond].order < 3)
        bonds[existingBond].order++;
    else
        return false;

    deleteAtom(from);
    if (from < to)
        to--;
    deleteAtom(to);
    return true;
}
