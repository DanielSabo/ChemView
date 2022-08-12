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

    colorParam = new Qt3DRender::QParameter(QStringLiteral("color"), QColor::fromRgbF(1.0f, 0.0f, 0.0f, 1.0f));

    auto techniqueGL3 = new Qt3DRender::QTechnique();
    auto apiFilterGL3 = techniqueGL3->graphicsApiFilter();
    apiFilterGL3->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
    apiFilterGL3->setMajorVersion(3);
    apiFilterGL3->setMinorVersion(1);
    apiFilterGL3->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);

    auto filterKeyGL3 = new Qt3DRender::QFilterKey(this);
    filterKeyGL3->setName(QStringLiteral("renderingStyle"));
    filterKeyGL3->setValue(QStringLiteral("forward"));
    techniqueGL3->addFilterKey(filterKeyGL3);

    auto renderPassGL3 = new Qt3DRender::QRenderPass();
    renderPassGL3->setShaderProgram(shaderGL3);

    // Enable alpha blending
    auto noDepthMask = new Qt3DRender::QNoDepthMask();
    renderPassGL3->addRenderState(noDepthMask);
    auto blendState = new Qt3DRender::QBlendEquationArguments();
    blendState->setSourceRgb(Qt3DRender::QBlendEquationArguments::SourceAlpha);
    blendState->setDestinationRgb(Qt3DRender::QBlendEquationArguments::OneMinusSourceAlpha);
    renderPassGL3->addRenderState(blendState);
    auto blendEquation = new Qt3DRender::QBlendEquation();
    blendEquation->setBlendFunction(Qt3DRender::QBlendEquation::Add);
    renderPassGL3->addRenderState(blendEquation);

    techniqueGL3->addRenderPass(renderPassGL3);

    auto effect = new Qt3DRender::QEffect(this);
    effect->addTechnique(techniqueGL3);
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
