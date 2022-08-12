#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "moldocument.h"
#include "molstruct.h"
#include "volumedata.h"

#include <QObject>

class Optimizer : public QObject
{
    Q_OBJECT
public:
    explicit Optimizer(QObject *parent = nullptr);

    virtual void setStructure(MolStruct m) = 0;
    virtual MolStruct getStructure() = 0;
    virtual bool run() = 0;
    virtual void kill() = 0;
    virtual MolDocument getResults();
    virtual QString getError();
    virtual QString getLogFile();

signals:
    void statusMessage(QString msg);
    void geometryUpdate();
    void finished();

};

#endif // OPTIMIZER_H
