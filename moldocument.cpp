#include "moldocument.h"

MolDocument::MolDocument()
{
}

MolDocument::MolDocument(MolStruct m) : molecule(m)
{
}

MolDocument::MolecularOrbital::MolecularOrbital() : MolecularOrbital(QString(), 0, 0, QString())
{

}

MolDocument::MolecularOrbital::MolecularOrbital(QString id, double occupancy, double energy, QString symmetry)
    : id(id), occupancy(occupancy), energy(energy), symmetry(symmetry)
{

}
