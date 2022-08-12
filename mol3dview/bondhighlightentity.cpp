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

#if 0
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
#endif

    techniqueGL3->addRenderPass(renderPassGL3);

    auto effect = new Qt3DRender::QEffect(this);
    effect->addTechnique(techniqueGL3);
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
