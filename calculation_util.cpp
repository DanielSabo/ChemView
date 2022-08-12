#include "calculation_util.h"
#include "element.h"

#include <QDebug>

int calc_util::overallCharge(MolStruct const &mol)
{
    int charge = 0;
    for (Atom const &a : mol.atoms)
        charge += a.charge;
    return charge;
}

int calc_util::overallSpin(MolStruct const &mol)
{
    QVector<int> bondCounts;
    bondCounts.fill(0, mol.atoms.size());

    for (auto const &b: mol.bonds)
    {
        bondCounts[b.to] += b.order;
        bondCounts[b.from] += b.order;
    }

    int spin = 0;
    bool spinValid = true;

    for (int i = 0; i < mol.atoms.size(); ++i)
    {
        int charge = mol.atoms[i].charge;
        Element element(mol.atoms[i].element);
        if (element.number - charge < bondCounts[i])
        {
            // The atom has more bonds than it has total electrons
            return 0;
            continue;
        }

        int effectiveNumber = element.number + bondCounts[i] - charge;
        if (effectiveNumber == 0) // H+
            continue;

        Element effectiveElement(effectiveNumber);
        if (!effectiveElement.isValid())
        {
            qWarning() << "Could not determine effective element for spin: charge =" << effectiveNumber;
            return 0;
        }
        int effectiveGroup = effectiveElement.group();

        int atomSpin = -1;
        if (effectiveGroup >= 16)
            atomSpin = 18 - effectiveGroup;
        else if (effectiveGroup >= 13)
            atomSpin = effectiveGroup - 12;
        else if (effectiveGroup >= 8)
        {
            spinValid = false;
            atomSpin = 12 - effectiveGroup;
        }
        else if (effectiveGroup >= 3)
        {
            spinValid = false;
            atomSpin = effectiveGroup - 2;
        }
        else if (effectiveGroup == 2)
            atomSpin = 0;
        else if (effectiveGroup == 1)
            atomSpin = 1;
        else if (effectiveGroup < 0 && effectiveGroup >= -7)
        {
            spinValid = false;
            atomSpin = -effectiveGroup;
        }
        else if (effectiveGroup < -7 && effectiveGroup >= -14)
        {
            spinValid = false;
            atomSpin = 14 + effectiveGroup;
        }
        else
        {
            qWarning() << "Could not determine effective group for spin: group =" << effectiveGroup;
            return 0;
        }

        if (atomSpin < 0)
            spinValid = false;
        else
            spin += atomSpin;
    }

    spin = spin + 1;

    if (!spinValid)
        return -spin;
    return spin;
}


QVector<int> calc_util::atomSpins(const MolStruct &mol)
{
    QVector<int> result;
    QVector<int> bondCounts;
    bondCounts.fill(0, mol.atoms.size());
    result.reserve(mol.atoms.size());

    for (auto const &b: mol.bonds)
    {
        bondCounts[b.to] += b.order;
        bondCounts[b.from] += b.order;
    }

    for (int i = 0; i < mol.atoms.size(); ++i)
    {
        int charge = mol.atoms[i].charge;
        Element element(mol.atoms[i].element);
        if (element.number - charge < bondCounts[i])
        {
            // The atom has more bonds than it has total electrons
            result.push_back(0);
            continue;
        }

        int effectiveNumber = element.number + bondCounts[i] - charge;
        if (effectiveNumber == 0) // H+
        {
            result.push_back(1);
            continue;
        }

        Element effectiveElement(effectiveNumber);
        if (!effectiveElement.isValid())
        {
            qWarning() << "Could not determine effective element for spin: charge =" << effectiveNumber;
            result.push_back(0);
            continue;
        }

        int effectiveGroup = effectiveElement.group();

        int atomSpin = -1;
        if (effectiveGroup >= 16)
            atomSpin = 18 - effectiveGroup;
        else if (effectiveGroup >= 13)
            atomSpin = effectiveGroup - 12;
        else if (effectiveGroup >= 8)
        {
//            spinValid = false;
            atomSpin = 12 - effectiveGroup;
        }
        else if (effectiveGroup >= 3)
        {
//            spinValid = false;
            atomSpin = effectiveGroup - 2;
        }
        else if (effectiveGroup == 2)
            atomSpin = 0;
        else if (effectiveGroup == 1)
            atomSpin = 1;
        else if (effectiveGroup < 0 && effectiveGroup >= -7)
        {
//            spinValid = false;
            atomSpin = -effectiveGroup;
        }
        else if (effectiveGroup < -7 && effectiveGroup >= -14)
        {
//            spinValid = false;
            atomSpin = 14 + effectiveGroup;
        }
        else
        {
            qWarning() << "Could not determine effective group for spin: group =" << effectiveGroup;
            atomSpin = 0;
        }

        result.push_back(atomSpin + 1);
    }

    return result;
}
