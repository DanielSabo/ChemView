#ifndef ATOMLABELENTITY_H
#define ATOMLABELENTITY_H

#include "mol3dview/atomentity.h"
#include "mol3dview/billboardentity.h"

#include <QMaterial>
#include <QPlaneMesh>
#include <QPaintedTextureImage>
#include <QPainter>

class AtomLabelEntityTexture : public Qt3DRender::QPaintedTextureImage
{

    Q_OBJECT
public:
    AtomLabelEntityTexture(Qt3DCore::QEntity *parent);

    void paint(QPainter *painter) override;

    void setText(QString newText);
    void setFont(QFont newFont);

    QFont font;
    QString text;
};

class AtomLabelEntity : public BillboardEntity
{
    Q_OBJECT
public:
    AtomLabelEntity(Qt3DCore::QEntity *parent);
    ~AtomLabelEntity() {}

    void setText(QString text);
    void setLineHeight(float lineHeight);

protected:
    void setMaterial(Qt3DRender::QMaterial *mat);
    void updateTransform(QMatrix4x4 const &worldMatrix, QVector3D camPos, QVector3D camUp) override;

public:
    QString text;
    float lineHeight;
    Qt3DRender::QMaterial *material = nullptr;
    Qt3DExtras::QPlaneMesh *mesh = nullptr;
};

#endif // ATOMLABELENTITY_H
