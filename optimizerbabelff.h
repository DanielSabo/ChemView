#ifndef OPTIMIZERBABELFF_H
#define OPTIMIZERBABELFF_H

#include "optimizer.h"

class QProcess;
class OptimizerBabelFF : public Optimizer
{
    Q_OBJECT
public:
    explicit OptimizerBabelFF(QObject *parent = nullptr);

    void setStructure(MolStruct m) override;
    MolStruct getStructure() override;
    MolDocument getResults() override;
    bool run() override;
    void kill() override;
    QString getError() override;

protected:
    QProcess *optimizeProcess;
    MolDocument document;
    QString error;
};

#endif // OPTIMIZERBABELFF_H
