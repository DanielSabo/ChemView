#include "optimizer.h"

Optimizer::Optimizer(QObject *parent) : QObject(parent)
{

}

MolDocument Optimizer::getResults()
{
    return {};
}

QString Optimizer::getError() {
    return {};
}

QString Optimizer::getLogFile()
{
    return {};
}
