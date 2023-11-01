#include "bondentity.h"
#include "bondhighlightentity.h"

#include <QBlendEquation>
#include <QBlendEquationArguments>
#include <QDiffuseSpecularMaterial>
#include <QEffect>
#include <QGraphicsApiFilter>
#include <QNoDepthMask>
#include <QParameter>
#include <QShaderProgram>
#include <QTechnique>
#include <QUrl>

namespace {
    static const float bondWidth = 0.125;
}

BondHighlightEntity::BondHighlightEntity(BondEntity *parent) : Qt3DCore::QEntity(parent)
{
    int order = 1;
    float length = 0.25f;
    if (parent)
    {
        order = parent->order;
        length = parent->vector.length();

        connect(parent, &BondEntity::vectorChanged, this, [this](){
            mesh->setHeight(qobject_cast<BondEntity *>(this->parent())->vector.length());
        });
    }

    auto transform = new Qt3DCore::QTransform();
    transform->setRotationX(-90.0f);
    transform->setTranslation(QVector3D(0.0f, 0.0f, 0.01f));
    addComponent(transform);

    float width = bondWidth*2.0f;
    if (order == 2)
        width = (bondWidth*1.7f)*2.0f;
    else if (order == 3)
        width = (bondWidth*2.25f)*2.0f;

    mesh = new Qt3DExtras::QPlaneMesh(this);
    mesh->setWidth(width + 0.1f);
    mesh->setHeight(length);
    addComponent(mesh);

    material = new BondHighlightMaterial(parent);
    material->setColor(QColor::fromRgbF(1.0f, 0.0f, 0.0f, 1.0f));
    addComponent(material);
}

void BondHighlightEntity::setColor(const QColor &c)
{
    material->setColor(c);
}

QColor BondHighlightEntity::getColor()
{
    return material->getColor();
}

BondHighlightMaterial::BondHighlightMaterial(Qt3DCore::QNode *parent) : Qt3DRender::QMaterial(parent)
{
    auto shaderGL3 = new Qt3DRender::QShaderProgram();
    shaderGL3->setVertexShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/bond_hightlight.vert"))));
    shaderGL3->setFragmentShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/bond_hightlight.frag"))));

    auto shaderRHI = new Qt3DRender::QShaderProgram();
    shaderRHI->setVertexShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/rhi/bond_hightlight.vert"))));
    shaderRHI->setFragmentShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/rhi/bond_hightlight.frag"))));

    auto techniqueGL3 = new Qt3DRender::QTechnique();
    techniqueGL3->graphicsApiFilter()->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
    techniqueGL3->graphicsApiFilter()->setMajorVersion(3);
    techniqueGL3->graphicsApiFilter()->setMinorVersion(1);
    techniqueGL3->graphicsApiFilter()->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);

    auto techniqueRHI = new Qt3DRender::QTechnique();
    techniqueRHI->graphicsApiFilter()->setApi(Qt3DRender::QGraphicsApiFilter::RHI);
    techniqueRHI->graphicsApiFilter()->setMajorVersion(1);
    techniqueRHI->graphicsApiFilter()->setMinorVersion(0);

    // Shared by both render backeneds
    colorParam = new Qt3DRender::QParameter(QStringLiteral("color"), QColor::fromRgbF(1.0f, 0.0f, 0.0f, 1.0f));

    auto filterKey = new Qt3DRender::QFilterKey(this);
    filterKey->setName(QStringLiteral("renderingStyle"));
    filterKey->setValue(QStringLiteral("forward"));

    // Configure GL render pass
    auto renderPassGL3 = new Qt3DRender::QRenderPass();
    renderPassGL3->setShaderProgram(shaderGL3);

    // Configure RHI render pass
    auto renderPassRHI = new Qt3DRender::QRenderPass();
    renderPassRHI->setShaderProgram(shaderRHI);

    techniqueGL3->addFilterKey(filterKey);
    techniqueGL3->addRenderPass(renderPassGL3);

    techniqueRHI->addFilterKey(filterKey);
    techniqueRHI->addRenderPass(renderPassRHI);

    auto effect = new Qt3DRender::QEffect(this);
    effect->addTechnique(techniqueGL3);
    effect->addTechnique(techniqueRHI);
    effect->addParameter(colorParam);
    setEffect(effect);
}

void BondHighlightMaterial::setColor(const QColor &c)
{
    colorParam->setValue(c);
}

QColor BondHighlightMaterial::getColor()
{
    return colorParam->value().value<QColor>();
}
