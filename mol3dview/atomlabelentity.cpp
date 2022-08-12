#include "mol3dview/atomlabelentity.h"

#include <QDebug>
#include <QSphereMesh>
#include <QTextureMaterial>
#include <QDiffuseSpecularMaterial>
#include <Qt3DRender/QTexture>

AtomLabelEntityTexture::AtomLabelEntityTexture(Qt3DCore::QEntity *parent) : Qt3DRender::QPaintedTextureImage(parent) {
}

void AtomLabelEntityTexture::paint(QPainter *painter)
{
    auto bounds = QRect(0,0,width(),height());

    //        painter->fillRect(QRect(0,0,width(),height()),QColor(255,0,0,0));
    painter->setCompositionMode(QPainter::CompositionMode_Clear);
    painter->eraseRect(QRect(0,0,width(),height()));
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter->setPen(QColor(0,0,0));
    painter->setFont(font);
    painter->drawText(bounds, Qt::AlignCenter, text);
    //        qDebug() << "Painting '" << text << "' in" << QRect(0,0,width(),height());
}

void AtomLabelEntityTexture::setText(QString newText)
{
    text = newText;
    auto textBounds = QFontMetrics(font).boundingRect(QRect(0,0,0,0), Qt::AlignCenter, text);
    setSize(textBounds.size());
}

void AtomLabelEntityTexture::setFont(QFont newFont)
{
    font = newFont;
    if (!text.isEmpty())
    {
        auto textBounds = QFontMetrics(font).boundingRect(QRect(0,0,0,0), Qt::AlignCenter, text);
        setSize(textBounds.size());
    }
}

AtomLabelEntity::AtomLabelEntity(Qt3DCore::QEntity *parent) : BillboardEntity(parent), lineHeight(0.06f) {
    mesh = new Qt3DExtras::QPlaneMesh();
    mesh->setWidth(.6f);
    mesh->setHeight(.6f);
    addComponent(mesh);

    auto mat = new Qt3DExtras::QDiffuseSpecularMaterial(parent);
    mat->setAmbient(QColor::fromRgbF(0.8f, 0.6f, 0.6f, 1.0f));
    setMaterial(mat);

    setText("?");
}

void AtomLabelEntity::setMaterial(Qt3DRender::QMaterial *mat) {
    if (material)
        removeComponent(material);
    material = mat;
    addComponent(material);
}

void AtomLabelEntity::updateTransform(const QMatrix4x4 &worldMatrix, QVector3D camPos, QVector3D camUp)
{
    BillboardEntity::updateTransform(worldMatrix, camPos, camUp);

    AtomEntity *p = qobject_cast<AtomEntity *>(parent());
    if (!p)
        return;

    float offset = p->mesh->radius();
    // The rotated translation is in the camera facing billboard space,
    // so "right" is now the negative Z axis
    transform->setTranslation(transform->rotation()*(QVector3D(offset*0.7071f+mesh->width()/2.0f, 0.0f, offset*-0.7071f-mesh->height()/2.0f)));
}

void AtomLabelEntity::setText(QString newText) {
    text = newText;
    auto font = QFont("Arial", 30);

    auto textureImage = new AtomLabelEntityTexture(nullptr);
    textureImage->setFont(font);
    textureImage->setText(text);
    textureImage->update();

    auto textBounds = textureImage->size();
    mesh->setHeight(lineHeight);
    mesh->setWidth(lineHeight * float(textBounds.width()) / float(textBounds.height()));

    auto tex2d = new Qt3DRender::QTexture2D(nullptr);
    tex2d->addTextureImage(textureImage);
    tex2d->setMinificationFilter(Qt3DRender::QTexture2D::Linear);
    tex2d->setMagnificationFilter(Qt3DRender::QTexture2D::Linear);
    auto mat = new Qt3DExtras::QTextureMaterial(this);
    mat->setTexture(tex2d);
    mat->setAlphaBlendingEnabled(true);
    setMaterial(mat);

}

void AtomLabelEntity::setLineHeight(float newLineHeight)
{
    lineHeight = newLineHeight;
    setText(text);
}
