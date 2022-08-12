#!/usr/bin/env python3

import sys
from rdkit.Chem import rdmolfiles
from rdkit.Chem import ChemicalForceFields
from rdkit.Chem import rdForceFieldHelpers

inputFilename = sys.argv[1]

m = rdmolfiles.MolFromMolFile(inputFilename, removeHs=False, strictParsing=True)
# rdForceFieldHelpers.MMFFOptimizeMolecule(m)

mmffProps = ChemicalForceFields.MMFFGetMoleculeProperties(m)
if mmffProps is None:
  ChemicalForceFields.MMFFGetMoleculeProperties(m, mmffVerbosity=2)
  sys.exit(1)
# print(mmffProps)
mmffField = ChemicalForceFields.MMFFGetMoleculeForceField(m, mmffProps)
# print(mmffField)
mmffField.Minimize()
energy = mmffField.CalcEnergy()

# CalcEnergy
print(rdmolfiles.MolToMolBlock(m))
print("> <Energy>")
print("%f kcal/mol\n" % energy)