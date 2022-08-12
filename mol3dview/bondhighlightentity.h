#ifndef BONDHIGHLIGHTENTITY_H
#define BONDHIGHLIGHTENTITY_H

#include "bondentity.h"

#include <QEntity>
#include <QPlaneMesh>

class BondHighlightMaterial : public Qt3DRender::QMaterial
{
    Q_OBJECT
public:
    BondHighlightMaterial(Qt3DCore::QNode *parent = nullptr);
    void setColor(QColor const &c);
    QColor getColor();

protected:
    Qt3DRender::QParameter *colorParam;
};

class BondHighlightEntity : public Qt3DCore::QEntity
{
public:
    BondHighlightEntity(BondEntity *parent);
    ~BondHighlightEntity() = default;

    void setColor(QColor const &c);
    QColor getColor();

public:
    BondHighlightMaterial *material = nullptr;
    Qt3DExtras::QPlaneMesh *mesh = nullptr;
};

#endif // BONDHIGHLIGHTENTITY_H
