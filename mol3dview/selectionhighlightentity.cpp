#include "mol3dview/selectionhighlightentity.h"
#include "mol3dview/atomentity.h"

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

SelectionHighlightMaterial::SelectionHighlightMaterial(Qt3DCore::QNode *parent) : Qt3DRender::QMaterial(parent)
{
    auto shaderGL3 = new Qt3DRender::QShaderProgram();
    shaderGL3->setVertexShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/selection_hightlight.vert"))));
    shaderGL3->setFragmentShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/gl3/selection_hightlight.frag"))));

    auto shaderRHI = new Qt3DRender::QShaderProgram();
    shaderRHI->setVertexShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/rhi/selection_hightlight.vert"))));
    shaderRHI->setFragmentShaderCode(Qt3DRender::QShaderProgram::loadSource(QUrl(QStringLiteral("qrc:/shaders/rhi/selection_hightlight.frag"))));

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

    auto noDepthMask = new Qt3DRender::QNoDepthMask();
    auto blendState = new Qt3DRender::QBlendEquationArguments();
    blendState->setSourceRgb(Qt3DRender::QBlendEquationArguments::SourceAlpha);
    blendState->setDestinationRgb(Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);
    auto blendEquation = new Qt3DRender::QBlendEquation();
    blendEquation->setBlendFunction(Qt3DRender::QBlendEquation::Add);

    // Configure GL render pass
    auto renderPassGL3 = new Qt3DRender::QRenderPass();
    renderPassGL3->setShaderProgram(shaderGL3);
    renderPassGL3->addRenderState(noDepthMask);
    renderPassGL3->addRenderState(blendState);
    renderPassGL3->addRenderState(blendEquation);

    // Configure RHI render pass
    auto renderPassRHI = new Qt3DRender::QRenderPass();
    renderPassRHI->setShaderProgram(shaderRHI);
    renderPassRHI->addRenderState(noDepthMask);
    renderPassRHI->addRenderState(blendState);
    renderPassRHI->addRenderState(blendEquation);

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

void SelectionHighlightMaterial::setColor(const QColor &c)
{
    colorParam->setValue(c);
}

QColor SelectionHighlightMaterial::getColor()
{
    return colorParam->value().value<QColor>();
}

SelectionHighlightEntity::SelectionHighlightEntity(Qt3DCore::QEntity *parent) : BillboardEntity(parent)
{
    mesh = new Qt3DExtras::QPlaneMesh();
    mesh->setWidth(.6f);
    mesh->setHeight(.6f);
    material = new SelectionHighlightMaterial(parent);;
    addComponent(mesh);
    addComponent(material);
}

void SelectionHighlightEntity::updatePosition(QVector3D camPos, QVector3D upVec)
{
    AtomEntity *p = qobject_cast<AtomEntity *>(parent());
    if (!p)
        return;

    float diameter = 2.0f*(p->mesh->radius() + 0.1f);
    mesh->setWidth(diameter);
    mesh->setHeight(diameter);

    BillboardEntity::updatePosition(camPos, upVec);
}

void SelectionHighlightEntity::setColor(const QColor &c)
{
    material->setColor(c);
}

QColor SelectionHighlightEntity::getColor()
{
    return material->getColor();
}
