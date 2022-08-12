#ifndef SELECTIONHIGHLIGHTENTITY_H
#define SELECTIONHIGHLIGHTENTITY_H

#include "mol3dview/billboardentity.h"

#include <QMaterial>
#include <QPlaneMesh>

class SelectionHighlightMaterial : public Qt3DRender::QMaterial
{
    Q_OBJECT
public:
    SelectionHighlightMaterial(Qt3DCore::QNode *parent = nullptr);
    void setColor(QColor const &c);
    QColor getColor();

protected:
    Qt3DRender::QParameter *colorParam;
};

class SelectionHighlightEntity : public BillboardEntity
{
    Q_OBJECT
public:
    SelectionHighlightEntity(Qt3DCore::QEntity *parent);
    ~SelectionHighlightEntity() {}

    void updatePosition(QVector3D camPos, QVector3D upVec) override;

    void setColor(QColor const &c);
    QColor getColor();

public:
    SelectionHighlightMaterial *material = nullptr;
    Qt3DExtras::QPlaneMesh *mesh = nullptr;
};

#endif // SELECTIONHIGHLIGHTENTITY_H
