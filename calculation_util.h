#ifndef CALCULATION_UTIL_H
#define CALCULATION_UTIL_H

#include "molstruct.h"

namespace calc_util {
    int overallCharge(MolStruct const &mol);
    int overallSpin(MolStruct const &mol);
    QVector<int> atomSpins(MolStruct const &mol);
}

#endif // CALCULATION_UTIL_H
