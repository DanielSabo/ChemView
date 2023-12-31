#include "mol3dview.h"
#include "molstruct.h"
#include "arcball.h"
#include "mol3dview/atomentity.h"
#include "mol3dview/bondentity.h"
#include "mol3dview/atomlabelentity.h"
#include "element.h"
#include "mol3dview/isosurfaceentity.h"
#include "mol3dview/selectionhighlightentity.h"
#include "mol3dview/bondhighlightentity.h"
#include "calculation_util.h"

#include <Qt3DWindow>
#include <QDebug>
#include <QEntity>
#include <QMaterial>
#include <QDiffuseSpecularMaterial>
#include <QSphereMesh>
#include <QTransform>
#include <QCamera>
#include <QCylinderMesh>
#include <QWheelEvent>
#include <QHBoxLayout>
#include <QScreenRayCaster>
#include <QLayer>
#include <QRenderSettings>
#include <QSortPolicy>
#include <QForwardRenderer>
#include <QLayerFilter>
#include <QRenderSurfaceSelector>
#include <QViewport>
#include <QCameraSelector>
#include <QFrustumCulling>
#include <QClearBuffers>
#include <QDebugOverlay>
#include <QFrameAction>
#include <QElapsedTimer>
#include <cmath>
#include <QPointLight>

namespace {
static Qt3DExtras::QDiffuseSpecularMaterial *makeMaterial(Qt3DCore::QNode *parent, float r, float g, float b) {
    auto result = new Qt3DExtras::QDiffuseSpecularMaterial(parent);
    r = r * 0.5 + 0.35;
    g = g * 0.5 + 0.35;
    b = b * 0.5 + 0.35;
    result->setAmbient(QColor::fromRgbF(r, g, b, 1.0f));
    result->setDiffuse(QColor::fromRgbF(r, g, b, 1.0f));
    return result;
}

static QMap<int, Qt3DExtras::QDiffuseSpecularMaterial *> generateMaterials(Qt3DCore::QNode *parent)
{
    // https://en.wikipedia.org/wiki/CPK_coloring

    QMap<int, Qt3DExtras::QDiffuseSpecularMaterial *> result;
    result.insert(0, makeMaterial(parent, 0.8, 0.4, 0.4));
    result.insert(1, makeMaterial(parent, 0.8f, 0.8f, 0.8f));
    result.insert(6, makeMaterial(parent, 0.2f, 0.2f, 0.2f));
    result.insert(7, makeMaterial(parent, 0.2f, 0.2f, 0.8f));
    result.insert(8, makeMaterial(parent, 0.8f, 0.2f, 0.2f));
    result.insert(9, makeMaterial(parent, 0.2f, 0.8f, 0.2f));
    result.insert(15, makeMaterial(parent, 0.8, 0.4, 0.0));
    result.insert(16, makeMaterial(parent, 0.8, 0.8, 0.0));
    result.insert(17, makeMaterial(parent, 0.2f, 0.8f, 0.2f));

    auto nobleGas = makeMaterial(parent, 0.0f, 0.8f, 0.8f);
    result.insert(2, nobleGas);
    result.insert(10, nobleGas);
    result.insert(18, nobleGas);
    result.insert(36, nobleGas);
    result.insert(54, nobleGas);

    auto alkaliMetals = makeMaterial(parent, 0.4, 0.0, 0.8);
    result.insert(3, alkaliMetals);
    result.insert(11, alkaliMetals);
    result.insert(19, alkaliMetals);
    result.insert(37, alkaliMetals);
    result.insert(55, alkaliMetals);
    result.insert(87, alkaliMetals);

    auto alkalineEarth = makeMaterial(parent, 0.1, 0.6, 0.1);
    result.insert(4, alkalineEarth);
    result.insert(12, alkalineEarth);
    result.insert(20, alkalineEarth);
    result.insert(38, alkalineEarth);
    result.insert(56, alkalineEarth);
    result.insert(88, alkalineEarth);

    return result;
}

static Qt3DExtras::QDiffuseSpecularMaterial *materialForElement(QMap<int, Qt3DExtras::QDiffuseSpecularMaterial *> const &mats, QString element)
{
    auto defaultMaterial = mats.value(0);
    int atomicNumber = Element::fromAbbr(element).number;
    return mats.value(atomicNumber, defaultMaterial);
}

// Map a click position to the XY plane of the view
static QVector3D clickTo3DPos(QPoint const &click, Qt3DExtras::Qt3DWindow const *view)
{
    auto camera = view->camera();

    // Calculate the dimensions of the frustum plane through the origin
    // https://stackoverflow.com/questions/13665932/calculating-the-viewing-frustum-in-a-3d-space
    float camDistance = camera->position().length();
    float planeHeight = 2.0f * tan(camera->fieldOfView() * (M_PI)/(180.0f * 2.0f)) * camDistance;
    float planeWidth = planeHeight * camera->aspectRatio();

    QVector3D posVec(planeWidth  * ( float(click.x())/view->width() - 0.5 ),
                     planeHeight * ( 0.5 - float(click.y())/view->height() ),
                     0.0f);

    return camera->transform()->rotation().rotatedVector(posVec);
}

struct AtomListEntry {
    AtomEntity *entity = nullptr;
    bool hovered = false;
    bool selected = false;
    bool highlighted = false;
    SelectionHighlightEntity *highightEntity = nullptr;
    AtomLabelEntity *label = nullptr;
};

struct BondListEntry {
    BondEntity *entity = nullptr;
    bool hovered = false;
    bool selected = false;
    BondHighlightEntity *highightEntity = nullptr;
};
}

class Mol3dViewPrivate
{
public:
    Mol3dViewPrivate(Mol3dView *q) : q_ptr(q) {}

    Mol3dView * const q_ptr;
    Q_DECLARE_PUBLIC(Mol3dView)

    Mol3dView::ToolMode toolMode = Mol3dView::ToolModeNone;
    bool inViewDrag = false;
    ArcBall arcball;
    QQuaternion rotation;
    float camDistance = 20.0f;
    Mol3dView::DrawStyle drawStyle = Mol3dView::DrawStyle::BallAndStick;

    Qt3DExtras::Qt3DWindow *view = nullptr;
    Qt3DCore::QEntity *rootEntity = nullptr;
    Qt3DCore::QEntity *structureEntity = nullptr;
    Qt3DCore::QEntity *lightEntity = nullptr;
    Qt3DCore::QTransform *lightTransform = nullptr;
    Qt3DRender::QLayer *transparentRenderLayer = nullptr;

    Qt3DRender::QScreenRayCaster *rayPicker = nullptr;
    Qt3DRender::QLayer *pickableLayer = nullptr;
    Qt3DCore::QEntity *hoverEntity = nullptr;
    QVector3D hoverIntersectVector;

    QMap<int, Qt3DExtras::QDiffuseSpecularMaterial *> atomMaterials;
    Qt3DExtras::QDiffuseSpecularMaterial *bondMaterial;

    MolStruct currentStructure;

    Qt3DCore::QEntity *currentSurface = nullptr;

    QElapsedTimer animationTimer;
    QVector<QVector3D> animationEigenvector;
    float animationIntensity;

    QList<MolStruct> undoStack;
    QList<MolStruct> redoStack;

    MolStruct addToolRGroup;

    QList<AtomListEntry> atomEntities;
    QList<BondListEntry> bondEntities;

    int atomSelectionLimit = 0;
    int bondSelectionLimit = 0;

    QSet<BillboardEntity *> billboards;

    QList<int> selectionAtoms;
    QList<int> selectionBonds;
    QList<int> highlightAtoms;

    QString statusMessage;

    void initScene();
    void clearScene();

    void rotate(float pitch, float yaw);
    void updateCamera();
    void setAtomSelected(int id, bool selected);
    void setBondSelected(int id, bool selected);
    void syncAtomHighlight(int id);
    void syncBondHighlight(int id);
    void clearSelection();
    void sceneFromMolStruct(MolStruct const &ms);

    void pickerHitUpdate(const Qt3DRender::QAbstractRayCaster::Hits &hits);
    void updateStatusMessage();
    void frameTickCallback(float dt);
};

void Mol3dViewPrivate::initScene()
{
    Q_Q(Mol3dView);

    if (rootEntity)
        return;

    rootEntity = new Qt3DCore::QEntity;
    pickableLayer = new Qt3DRender::QLayer(rootEntity);
    rayPicker = new Qt3DRender::QScreenRayCaster(rootEntity);
    rayPicker->setRunMode(Qt3DRender::QAbstractRayCaster::SingleShot);
    rayPicker->addLayer(pickableLayer);
    rootEntity->addComponent(rayPicker);
    view->renderSettings()->pickingSettings()->setPickMethod(Qt3DRender::QPickingSettings::TrianglePicking);

    view->renderSettings()->setRenderPolicy(Qt3DRender::QRenderSettings::OnDemand);

    QObject::connect(rayPicker, &Qt3DRender::QScreenRayCaster::hitsChanged, q,
            [this](const Qt3DRender::QAbstractRayCaster::Hits &hits) {pickerHitUpdate(hits);});

    bondMaterial = makeMaterial(rootEntity, 0.6f, 0.6f, 0.6f);
    atomMaterials = generateMaterials(rootEntity);

    structureEntity = new Qt3DCore::QEntity(rootEntity);
    lightEntity = new Qt3DCore::QEntity(rootEntity);
    lightTransform = new Qt3DCore::QTransform();
    auto light = new Qt3DRender::QPointLight(rootEntity);
    light->setColor(Qt::white);
    light->setIntensity(0.5);
    lightEntity->addComponent(light);
    lightEntity->addComponent(lightTransform);

    view->setRootEntity(rootEntity);

    /* How to properly render transparent objects: https://stackoverflow.com/questions/55001233/qt3d-draw-transparent-qspheremesh-over-triangles
     *
     * The framegraph is a tree of nodes that are traversed depth first (See QForwardRendererPrivate::init()),
     * rendering occurs whenever a leaf node is encountered.
     */

    transparentRenderLayer = new Qt3DRender::QLayer(rootEntity);

    auto surfaceSelector = new Qt3DRender::QRenderSurfaceSelector();
    auto viewport = new Qt3DRender::QViewport(surfaceSelector);
    auto cameraSelector = new Qt3DRender::QCameraSelector(viewport);
    auto frustumCulling = new Qt3DRender::QFrustumCulling(cameraSelector);
    auto clearBuffer = new Qt3DRender::QClearBuffers(frustumCulling);

    surfaceSelector->setSurface(view);
    viewport->setNormalizedRect(QRectF(0.0, 0.0, 1.0, 1.0));
    cameraSelector->setCamera(view->camera());
    clearBuffer->setClearColor(Qt::white);
    clearBuffer->setBuffers(Qt3DRender::QClearBuffers::ColorDepthBuffer);

    auto opaqueFilter = new Qt3DRender::QLayerFilter(clearBuffer);
    opaqueFilter->addLayer(transparentRenderLayer);
    opaqueFilter->setFilterMode(Qt3DRender::QLayerFilter::DiscardAnyMatchingLayers);
    auto transparentFilter = new Qt3DRender::QLayerFilter(frustumCulling);
    transparentFilter->addLayer(transparentRenderLayer);
    transparentFilter->setFilterMode(Qt3DRender::QLayerFilter::AcceptAnyMatchingLayers);

    Qt3DRender::QSortPolicy *sortPolicy = new Qt3DRender::QSortPolicy(transparentFilter);
    sortPolicy->setSortTypes(QVector<Qt3DRender::QSortPolicy::SortType>{Qt3DRender::QSortPolicy::BackToFront});

    //new Qt3DRender::QDebugOverlay(frustumCulling);

    auto frameAction = new Qt3DLogic::QFrameAction;
    rootEntity->addComponent(frameAction);
    QObject::connect(frameAction, &Qt3DLogic::QFrameAction::triggered, q,
                     [this](float dt) { frameTickCallback(dt); });

    // The view will take owernship of the framegraph
    view->setActiveFrameGraph(surfaceSelector);
}

void Mol3dViewPrivate::clearScene()
{
    clearSelection();

    bondEntities = {};
    atomEntities = {};
    billboards = {};
    currentStructure = {};
    hoverEntity = nullptr;
    currentSurface = nullptr;
    animationEigenvector = {};

    delete structureEntity;
    structureEntity = new Qt3DCore::QEntity(rootEntity);
}

void Mol3dViewPrivate::updateCamera()
{
    auto cameraRotation = (arcball.getRotation() * rotation).inverted();
    auto camera = view->camera();

    //FIXME: Aspect ratio?
    camera->lens()->setPerspectiveProjection(45.0f, float(view->width())/float(view->height()), 0.1f, 1000.0f);

    auto camPos = cameraRotation.rotatedVector(QVector3D(0.0f, 0.0f, camDistance));
    auto lightPos = cameraRotation.rotatedVector(QVector3D(camDistance, camDistance, 0.0f));
    lightTransform->setTranslation(lightPos);
    camera->setPosition(camPos);
    camera->setViewCenter(QVector3D(0, 0, 0));
    auto upVec = cameraRotation.rotatedVector(QVector3D(0.0f, 1.0f, 0.0));
    camera->setUpVector(upVec);

    for (auto &b: billboards)
        b->updatePosition(camPos, upVec);
    for (auto &b: bondEntities)
        b.entity->updatePosition(camPos, upVec);
}

void Mol3dViewPrivate::setAtomSelected(int id, bool selected)
{
    if (id < 0 || id >= atomEntities.size())
    {
        qDebug() << "setAtomSelected(): invalid id:" << id;
        return;
    }

    if (selectionAtoms.contains(id) == selected)
        return;

    if (selected)
        selectionAtoms.push_back(id);
    else
        selectionAtoms.removeOne(id);

    atomEntities[id].selected = selected;
    syncAtomHighlight(id);
}

void Mol3dViewPrivate::setBondSelected(int id, bool selected)
{
    if (id < 0 || id >= bondEntities.size())
    {
        qDebug() << "setBondSelected(): invalid id:" << id;
        return;
    }

    if (selectionBonds.contains(id) == selected)
        return;

    if (selected)
        selectionBonds.push_back(id);
    else
        selectionBonds.removeOne(id);

    bondEntities[id].selected = selected;
    syncBondHighlight(id);
}

void Mol3dViewPrivate::syncAtomHighlight(int id)
{
    auto &target = atomEntities[id];

    if (!target.hovered && !target.selected && !target.highlighted)
    {
        if (target.highightEntity)
        {
            billboards.remove(target.highightEntity);
            delete target.highightEntity;
            target.highightEntity = nullptr;
        }
    }
    else
    {
        if (!target.highightEntity)
        {
            target.highightEntity = new SelectionHighlightEntity(target.entity);
            target.highightEntity->addComponent(transparentRenderLayer);
            auto camera = view->camera();
            target.highightEntity->updatePosition(camera->position(), camera->upVector());
            billboards.insert(target.highightEntity);
        }

        if (target.hovered)
            target.highightEntity->setColor(QColor::fromRgb(66, 167, 245));
        else if (target.selected)
            target.highightEntity->setColor(QColor::fromRgb(245, 188, 66));
        else if (target.highlighted)
            target.highightEntity->setColor(QColor::fromRgb(222, 120, 18));
    }
}

void Mol3dViewPrivate::syncBondHighlight(int id)
{
    auto &target = bondEntities[id];

    //if (!target.hovered && !target.selected && !target.highlighted)
    if (!target.hovered && !target.selected)
    {
        if (target.highightEntity)
        {
            delete target.highightEntity;
            target.highightEntity = nullptr;
        }
    }
    else
    {
        if (!target.highightEntity)
        {
            target.highightEntity = new BondHighlightEntity(target.entity);
        }

        if (target.hovered)
            target.highightEntity->setColor(QColor::fromRgb(66, 167, 245));
        else if (target.selected)
            target.highightEntity->setColor(QColor::fromRgb(245, 188, 66));
    }
}

void Mol3dViewPrivate::clearSelection()
{
    Q_Q(Mol3dView);

    for (int id: selectionAtoms)
    {
        atomEntities[id].selected = false;
        syncAtomHighlight(id);
    }
    selectionAtoms = {};

    for (int id: selectionBonds)
    {
        bondEntities[id].selected = false;
        syncBondHighlight(id);
    }
    selectionBonds = {};

    q->setHighlight({});

    emit q->selectionChanged({});
}

void Mol3dViewPrivate::updateStatusMessage()
{
    QString message;

    if (toolMode == Mol3dView::ToolModeAdd)
    {
        if (currentStructure.isEmpty())
            message = QStringLiteral("Double click to add a new atom");
        else
            message = QStringLiteral("Double click a valence hydrogen to attach a new atom");
    }
    else if (toolMode == Mol3dView::ToolModeAddValence)
    {
        message = QStringLiteral("Double click anywhere on an atom to attach a new valence");
    }
    else if (toolMode == Mol3dView::ToolModeDelete)
    {
        message = QStringLiteral("Double click to an atom to delete it or a bond to break it");
    }
    else if (toolMode == Mol3dView::ToolModeBond)
    {
        message = QStringLiteral("Select two valence hydrogens to form a bond or ctrl+click a bond to break it");
    }
    else if (toolMode == Mol3dView::ToolModeSelect)
    {
        if (atomSelectionLimit && bondSelectionLimit)
            message = QStringLiteral("Click on an atom or bond to select it");
        else if (bondSelectionLimit)
            message = QStringLiteral("Click on a bond to select it");
        else if (atomSelectionLimit)
            message = QStringLiteral("Click on an atom to select it");
        else
            message = QString();
    }

    if (message != statusMessage)
    {
        Q_Q(Mol3dView);

        statusMessage = message;
        emit q->hoverInfo(message);
    }
}

void Mol3dViewPrivate::pickerHitUpdate(const Qt3DRender::QAbstractRayCaster::Hits &hits)
{
//    Q_Q(Mol3dView);

    int oldAtom = -1;
    int newAtom = -1;
    int oldBond = -1;
    int newBond = -1;

    if (auto atom = qobject_cast<AtomEntity *>(hoverEntity))
        oldAtom = atom->id;

    if (auto bond = qobject_cast<BondEntity *>(hoverEntity))
        oldBond = bond->id;

    if (hits.length())
    {
        // In triangle picker mode sometimes the hits are out of order
        auto iter = hits.begin();
        auto hit = *iter++;
        while (iter != hits.end())
        {
            if (iter->distance() < hit.distance())
                hit = *iter;
            iter++;
        }

        hoverEntity = hit.entity();
        hoverIntersectVector = {};

        if(auto atom = qobject_cast<AtomEntity *>(hit.entity()))
        {
            hoverIntersectVector = hit.worldIntersection() - atom->transform->translation();
            newAtom = atom->id;
//            emit q->hoverInfo(atom->label);

        }
        else if(auto bond = qobject_cast<BondEntity *>(hit.entity()))
        {
            newBond = bond->id;
//            emit q->hoverInfo(bond->label);
        }
        else
        {
            QString output;
            QDebug(&output) << hit.entity();
//            emit q->hoverInfo(output);
        }
    }
    else
    {
        hoverEntity = nullptr;
//        emit q->hoverInfo("");
    }

    if (toolMode == Mol3dView::ToolModeAdd)
    {
        newBond = -1;
        if ((newAtom != -1) && ((currentStructure.atoms[newAtom].element != "H") || !currentStructure.isLeafAtom(newAtom)))
            newAtom = -1;
    }
    else if (toolMode == Mol3dView::ToolModeBond)
    {
        if ((newAtom != -1) && ((currentStructure.atoms[newAtom].element != "H") || !currentStructure.isLeafAtom(newAtom)))
            newAtom = -1;
    }
    else if (toolMode == Mol3dView::ToolModeSelect)
    {
        if (atomSelectionLimit == 0)
            newAtom = -1;

        if (bondSelectionLimit == 0)
            newBond = -1;
    }

    if (oldAtom != newAtom)
    {
        if (oldAtom != -1)
        {
            atomEntities[oldAtom].hovered = false;
            syncAtomHighlight(oldAtom);
        }

        if (newAtom != -1)
        {
            atomEntities[newAtom].hovered = true;
            syncAtomHighlight(newAtom);
        }
    }

    if (oldBond != newBond)
    {
        if (oldBond != -1)
        {
            bondEntities[oldBond].hovered = false;
            syncBondHighlight(oldBond);
        }

        if (newBond != -1)
        {
            bondEntities[newBond].hovered = true;
            syncBondHighlight(newBond);
        }
    }

    updateStatusMessage();
}

void Mol3dViewPrivate::frameTickCallback(float dt)
{
    (void)dt;

    if (!animationEigenvector.isEmpty())
    {
        float amplitude = sinf(animationTimer.elapsed() * M_PI/500.0f)*animationIntensity;

        QVector<QVector3D> positions;
        positions.reserve(currentStructure.atoms.size());

        for (int i = 0; i < atomEntities.size(); ++i)
        {
            positions.push_back(currentStructure.atoms[i].posToVector() + animationEigenvector[i]*amplitude);
            atomEntities[i].entity->transform->setTranslation(positions[i]);
        }

        auto camera = view->camera();
        QVector3D camPos = camera->position();
        QVector3D camUp = camera->upVector();
        for (int i = 0; i < bondEntities.size(); ++i)
        {
            auto const &bond = currentStructure.bonds[i];
            bondEntities[i].entity->setVector(positions[bond.from], positions[bond.to]);
            bondEntities[i].entity->updatePosition(camPos, camUp);
        }
    }
}

Mol3dView::Mol3dView(QWidget *parent) : QWidget(parent),
  d_ptr(new Mol3dViewPrivate(this))
{
    Q_D(Mol3dView);
    auto theLayout = new QHBoxLayout(this);
    theLayout->setContentsMargins(0,0,0,0);

    d->view = new Qt3DExtras::Qt3DWindow();
    QWidget *container = QWidget::createWindowContainer(d->view);
    theLayout->addWidget(container);
    d->view->installEventFilter(this);

    d->initScene();

    setFocusProxy(container);
}

Mol3dView::~Mol3dView()
{
}

bool Mol3dView::eventFilter(QObject *obj, QEvent *event)
{
    Q_D(Mol3dView);

    if (obj != d->view)
        return false;

    if (event->type() == QEvent::KeyPress)
    {
        auto e = (static_cast<QKeyEvent *>(event));

        // TODO: Merge rotate vectors from press & release
        if (e->isAutoRepeat())
        {
            if (Qt::Key_Up == e->key())
                rotate(3,0,0);
            else if (Qt::Key_Down == e->key())
                rotate(-3,0,0);
            else if ((e->modifiers() & Qt::ShiftModifier) && (Qt::Key_Left == e->key()))
                rotate(0,0,3);
            else if ((e->modifiers() & Qt::ShiftModifier) && (Qt::Key_Right == e->key()))
                rotate(0,0,-3);
            else if (Qt::Key_Left == e->key())
                rotate(0,3,0);
            else if (Qt::Key_Right == e->key())
                rotate(0,-3,0);
        }

        return true;
    }
    else if (event->type() == QEvent::KeyRelease)
    {
        auto e = (static_cast<QKeyEvent *>(event));

        if (!e->isAutoRepeat())
        {
            if (Qt::Key_Up == e->key())
                rotate(15,0,0);
            else if (Qt::Key_Down == e->key())
                rotate(-15,0,0);
            else if ((e->modifiers() & Qt::ShiftModifier) && (Qt::Key_Left == e->key()))
                rotate(0,0,15);
            else if ((e->modifiers() & Qt::ShiftModifier) && (Qt::Key_Right == e->key()))
                rotate(0,0,-15);
            else if (Qt::Key_Left == e->key())
                rotate(0,15,0);
            else if (Qt::Key_Right == e->key())
                rotate(0,-15,0);
            else if (Qt::Key_Escape == e->key())
            {
                if ((d->toolMode == ToolModeBond) ||
                    (d->toolMode == ToolModeSelect))
                {
                    d->clearSelection();
                }
            }
        }
        return true;
    }
    else if (event->type() == QEvent::Wheel)
    {
        auto e = static_cast<QWheelEvent *>(event);
        if (e->angleDelta().y() != 0)
        {
            d->camDistance += e->angleDelta().y() / 120.0;
            d->camDistance = qMax(d->camDistance, 1.0f);
            d->updateCamera();
        }
        return true;
    }

    bool refreshPicker = false;
    bool mouseConsumed = false;

    // Handle click events for all tool modes
    if (event->type() == QEvent::MouseButtonPress && static_cast<QMouseEvent *>(event)->button() == 1)
    {
        auto atom = qobject_cast<AtomEntity *>(d->hoverEntity);
        auto bond = qobject_cast<BondEntity *>(d->hoverEntity);
        bool isDoubleClick = static_cast<QMouseEvent *>(event)->flags() == Qt::MouseEventCreatedDoubleClick;

        if (d->toolMode == ToolModeAdd && isDoubleClick)
        {
            if (atom && d->currentStructure.atoms[atom->id].element == "H" && d->currentStructure.isLeafAtom(atom->id))
            {
                auto groupStructure = d->addToolRGroup;
                // TODO: Load methyl in init so addToolRGroup is always set
                if (groupStructure.isEmpty())
                    groupStructure = MolStruct::fromSDF(":structure/methyl.mol");
                auto newStructure = d->currentStructure;
                newStructure.replaceLeafWithRGroup(atom->id, groupStructure);
                addUndoEvent("Add atom");
                showMolStruct(newStructure);

                mouseConsumed = true;
            }
            else if (!atom && !bond)
            {
                auto newFragment = d->addToolRGroup;
                if (newFragment.isEmpty())
                    newFragment = MolStruct::fromSDF(":structure/methyl.mol");
                for (int i = 0; i < newFragment.atoms.size(); ++i)
                    if (newFragment.atoms[i].element == "R1")
                        newFragment.eraseGroup(i);

                QVector3D posVec = clickTo3DPos(static_cast<QMouseEvent *>(event)->pos(), d->view);

                newFragment.recenter(Vector3D(posVec));

                auto newStructure = d->currentStructure;
                newStructure.addFragment(newFragment);
                addUndoEvent("Add fragment");
                showMolStruct(newStructure);

                mouseConsumed = true;
            }

        }
        else if (d->toolMode == ToolModeAddValence && isDoubleClick)
        {
            if (atom)
            {
                auto newStructure = d->currentStructure;
                newStructure.addHydrogenToAtom(atom->id, d->hoverIntersectVector);
                addUndoEvent("Add valence");
                showMolStruct(newStructure);

                mouseConsumed = true;
            }
            else if (!atom && !bond)
            {
                QVector3D posVec = clickTo3DPos(static_cast<QMouseEvent *>(event)->pos(), d->view);

                MolStruct newFragment;
                Atom a;
                a.element = "H";
                a.setPos(posVec);
                newFragment.atoms.push_back(a);

                auto newStructure = d->currentStructure;
                newStructure.addFragment(newFragment);

                addUndoEvent("Add fragment");
                showMolStruct(newStructure);

                mouseConsumed = true;
            }
        }
        else if (d->toolMode == ToolModeDelete && isDoubleClick)
        {
            if (atom)
            {
                if (d->currentStructure.isLeafAtom(atom->id) && d->currentStructure.atoms[atom->id].element == "H")
                {
                    auto newStructure = d->currentStructure;
                    newStructure.deleteAtom(atom->id);
                    addUndoEvent("Delete valence");
                    showMolStruct(newStructure);
                }
                else
                {
                    auto newStructure = d->currentStructure;
                    newStructure.eraseGroup(atom->id);
                    addUndoEvent("Delete atom");
                    showMolStruct(newStructure);
                }

                mouseConsumed = true;
                refreshPicker = true;
            }
            else if (bond)
            {
                auto newStructure = d->currentStructure;
                int from = newStructure.bonds[bond->id].from;
                int to = newStructure.bonds[bond->id].to;

                // When control is pressed delete the bond without restoring valences
                bool ctrlMod = (static_cast<QMouseEvent *>(event)->modifiers() & Qt::ControlModifier);
                bool demoteBond = true;

                // If control is pressed we just delete the bond without considering valences
                if (!ctrlMod)
                {
                    // With the delete tool treat deleting a valence bond the same as deleting the valence
                    if (newStructure.atoms[from].element == "H" && newStructure.isLeafAtom(from))
                    {
                        newStructure.deleteAtom(from);
                        demoteBond = false;
                    }
                    else if (newStructure.atoms[to].element == "H" && newStructure.isLeafAtom(to))
                    {
                        newStructure.deleteAtom(to);
                        demoteBond = false;
                    }
                    else
                    {
                        newStructure.addHydrogenToAtom(from);
                        newStructure.addHydrogenToAtom(to);
                    }
                }

                if (demoteBond)
                {
                    if (newStructure.bonds[bond->id].order <= 1)
                        newStructure.bonds.removeAt(bond->id);
                    else
                        newStructure.bonds[bond->id].order--;
                }

                if (ctrlMod)
                    addUndoEvent("Delete bond");
                else
                    addUndoEvent("Break bond");
                showMolStruct(newStructure);
                mouseConsumed = true;
                refreshPicker = true;
            }
        }
        else if (d->toolMode == ToolModeBond)
        {
            if (atom)
            {
                if (!d->selectionAtoms.empty())
                {
                    if (atom->id == d->selectionAtoms.first())
                    {
                        d->clearSelection();
                    }
                    else
                    {
                        auto newStructure = d->currentStructure;
                        int from = d->selectionAtoms.first();
                        int to = atom->id;
                        //TODO: We need a function to check if this will pass
                        if (newStructure.hydrogensToBond(from, to))
                        {
                            addUndoEvent("Form bond");
                            showMolStruct(newStructure);
                            refreshPicker = true;
                        }
                    }
                    mouseConsumed = true;
                }
                else if (atom && d->currentStructure.isLeafAtom(atom->id) && d->currentStructure.atoms[atom->id].element == "H")
                {
                    d->setAtomSelected(atom->id, true);
                    mouseConsumed = true;
                }
            }
            else if (bond && (static_cast<QMouseEvent *>(event)->modifiers() & Qt::ControlModifier))
            {
                auto newStructure = d->currentStructure;
                int from = newStructure.bonds[bond->id].from;
                int to = newStructure.bonds[bond->id].to;

                // When breaking a bond to a hydrogen just delete the bond
                if (newStructure.atoms[from].element == "H" && newStructure.isLeafAtom(from))
                {
                        newStructure.bonds.removeAt(bond->id);
                        addUndoEvent("Delete bond");
                }
                else if (newStructure.atoms[to].element == "H" && newStructure.isLeafAtom(to))
                {
                        newStructure.bonds.removeAt(bond->id);
                        addUndoEvent("Delete bond");
                }
                else
                {
                    newStructure.addHydrogenToAtom(from);
                    newStructure.addHydrogenToAtom(to);
                    if (newStructure.bonds[bond->id].order <= 1)
                        newStructure.bonds.removeAt(bond->id);
                    else
                        newStructure.bonds[bond->id].order--;
                    addUndoEvent("Break bond");
                }

                showMolStruct(newStructure);
                refreshPicker = true;
                mouseConsumed = true;
            }
        }
        else if (d->toolMode == ToolModeSelect)
        {
            if (atom && (d->atomSelectionLimit != 0))
            {
                for (int b: d->selectionBonds)
                {
                    d->bondEntities[b].selected = false;
                    d->syncBondHighlight(b);
                }
                d->selectionBonds = {};

                if (d->atomEntities[atom->id].selected)
                {
                    d->setAtomSelected(atom->id, false);
                }
                else
                {
                    if (d->atomSelectionLimit > 0)
                        while (d->selectionAtoms.size() >= d->atomSelectionLimit)
                            d->setAtomSelected(d->selectionAtoms.last(), false);

                    d->setAtomSelected(atom->id, true);
                }

                emit selectionChanged(getSelection());
                mouseConsumed = true;
            }
            else if (bond && (d->bondSelectionLimit != 0))
            {
                for (int a: d->selectionAtoms)
                {
                    d->atomEntities[a].selected = false;
                    d->syncAtomHighlight(a);
                }
                d->selectionAtoms = {};

                if (d->bondEntities[bond->id].selected)
                {
                    d->setBondSelected(bond->id, false);
                }
                else
                {
                    if (d->bondSelectionLimit > 0)
                        while (d->selectionBonds.size() >= d->bondSelectionLimit)
                            d->setBondSelected(d->selectionBonds.last(), false);

                    d->setBondSelected(bond->id, true);
                }

                emit selectionChanged(getSelection());
                mouseConsumed = true;
            }
        }

        if (refreshPicker)
            d->rayPicker->trigger(static_cast<QMouseEvent *>(event)->pos());
        if (mouseConsumed)
            return true;
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        auto e = static_cast<QMouseEvent *>(event);

        // If nothing consumed the click then do a view drag
        if (e->button() == 1)
        {
            d->inViewDrag = true;
            auto pos = e->pos();
            float x = 2.0f*float(pos.x()) / d->view->width() - 1.0f;
            float y = -(2.0f*float(pos.y()) / d->view->height() - 1.0f);
            d->arcball.startMotion(x, y);
            return true;
        }

        return true;
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        auto e = static_cast<QMouseEvent *>(event);
        if (e->button() == 1 && d->inViewDrag)
        {
            d->inViewDrag = false;
            auto pos = e->pos();
            float x = 2.0f*float(pos.x()) / d->view->width() - 1.0f;
            float y = -(2.0f*float(pos.y()) / d->view->height() - 1.0f);
            d->arcball.updatePos(x, y);
            d->rotation = d->arcball.getRotation() * d->rotation;
            d->arcball.reset();
            d->updateCamera();
        }

        return true;
    }
    else if (event->type() == QEvent::MouseMove)
    {
        auto e = static_cast<QMouseEvent *>(event);
        if (d->inViewDrag)
        {
            auto pos = e->pos();
            float x = 2.0f*float(pos.x()) / d->view->width() - 1.0f;
            float y = -(2.0f*float(pos.y()) / d->view->height() - 1.0f);
            d->arcball.updatePos(x, y);
            d->updateCamera();
        }
        else
        {
            d->rayPicker->trigger(e->pos());
        }

        return true;
    }
    else if (event->type() == QEvent::Wheel)
    {
        auto e = static_cast<QWheelEvent *>(event);
        if (e->angleDelta().y() != 0)
        {
            d->camDistance += e->angleDelta().y() / 120.0;
            d->camDistance = qMax(d->camDistance, 1.0f);
            d->updateCamera();
        }
        return true;
    }


    return false;
}

QWindow *Mol3dView::getViewWindow()
{
    Q_D(Mol3dView);
    return d->view;
}

void Mol3dView::showMolStruct(const MolStruct &ms)
{
    Q_D(Mol3dView);

    d->clearScene();

    d->currentStructure = ms;

    auto atomSpins = calc_util::atomSpins(ms);

    d->atomEntities.reserve(ms.atoms.size());
    for (int i = 0; i < ms.atoms.size(); ++i)
    {
        Atom const &a = ms.atoms[i];
        auto element = Element::fromAbbr(a.element);

        AtomListEntry listEntry;

        AtomEntity *atomEntity = new AtomEntity(d->structureEntity);
        listEntry.entity = atomEntity;

        atomEntity->id = i;
        if (a.charge == 0)
            atomEntity->label = QString("%1-%2").arg(i).arg(element.abbr);
        else if (a.charge > 0)
            atomEntity->label = QString("%1-%2 Charge(+%3)").arg(i).arg(element.abbr).arg(a.charge);
        else if (a.charge < 0)
            atomEntity->label = QString("%1-%2 Charge(%3)").arg(i).arg(element.abbr).arg(a.charge);
        atomEntity->addComponent(d->pickableLayer);

        atomEntity->transform->setTranslation(a.posToVector());

        float radius;
        if (d->drawStyle == DrawStyle::BallAndStick)
        {
            radius = 0.8 * element.empiricalRadius();
            if (radius <= 0)
                radius = 0.8 * Element::fromAtomicNumber(6).empiricalRadius();
        }
        else // DrawStyle::Stick
        {
            radius = 0.125;
        }

        atomEntity->setRadius(radius);
        atomEntity->setMaterial(materialForElement(d->atomMaterials, a.element));

        if (a.charge != 0 || atomSpins[i] != 1)
        {
            QString labelText;
            if (a.charge == 1)
                labelText = "+";
            else if (a.charge == -1)
                labelText = "-";
            else if (a.charge > 1)
                labelText = QString("%1+").arg(a.charge);
            else if (a.charge < -1)
                labelText = QString("%1-").arg(abs(a.charge));

            if (atomSpins[i] == 0)
                labelText += "?";
            else if (atomSpins[i] != 1)
                labelText += u"\u25CF"; // u"\u2022";

            auto label = new AtomLabelEntity(atomEntity);
            listEntry.label = label;

            label->setText(labelText);
            label->setLineHeight(0.9f);
            label->addComponent(d->transparentRenderLayer);

            d->billboards.insert(label);
        }

        d->atomEntities.push_back(listEntry);
    }

    d->bondEntities.reserve(ms.bonds.size());
    for (int i = 0; i < ms.bonds.size(); ++i)
    {
        Bond const &b = ms.bonds[i];
        Atom start = ms.atoms[b.from];
        Atom end = ms.atoms[b.to];

        BondEntity *bondEntity = new BondEntity(d->structureEntity);
        d->bondEntities.push_back({bondEntity});
        bondEntity->id = i;
        bondEntity->label = QString("%1-%2-%3 (%4)")
                .arg(i)
                .arg(start.element)
                .arg(end.element)
                .arg(b.order);
        bondEntity->addComponent(d->pickableLayer);

        bondEntity->setOrder(b.order);
        bondEntity->setMaterial(d->bondMaterial);
        bondEntity->setVector(start.posToVector(), end.posToVector());
    }

    d->updateCamera();

    emit moleculeChanged();
}

void Mol3dView::showVolumeData(VolumeData const &vol, float threshold)
{
    Q_D(Mol3dView);
    if (d->currentSurface)
    {
        delete d->currentSurface;
        d->currentSurface = nullptr;
    }

    if (!vol.data.isEmpty())
    {
        d->currentSurface = IsosurfaceEntity::fromData(vol, QColor::fromRgbF(0.3f, 0.3f, 0.7f, 0.5f), threshold);
        d->currentSurface->addComponent(d->transparentRenderLayer);
        d->currentSurface->setParent(d->structureEntity);
    }

    d->updateCamera();
}

void Mol3dView::showAnimation(QVector<QVector3D> eigenvector, float intensity)
{
    Q_D(Mol3dView);
    d->animationTimer.start();
    d->animationEigenvector = eigenvector;
    d->animationIntensity = intensity;

    auto const &mol = d->currentStructure;

    // Reset positions
    for (int i = 0; i < d->atomEntities.size(); ++i)
        d->atomEntities[i].entity->transform->setTranslation(mol.atoms[i].posToVector());

    auto camera = d->view->camera();
    QVector3D camPos = camera->position();
    QVector3D camUp = camera->upVector();
    for (int i = 0; i < d->bondEntities.size(); ++i)
    {
        auto const &bond = mol.bonds[i];
        d->bondEntities[i].entity->setVector(mol.atoms[bond.from].posToVector(),  mol.atoms[bond.to].posToVector());
        d->bondEntities[i].entity->updatePosition(camPos, camUp);
    }
}

MolStruct Mol3dView::getMolStruct()
{
    Q_D(Mol3dView);
    return d->currentStructure;
}

void Mol3dView::rotate(float pitch, float yaw, float roll)
{
    Q_D(Mol3dView);
    d->rotation = d->rotation * QQuaternion::fromEulerAngles(pitch, yaw, roll);
    d->updateCamera();
}

void Mol3dView::setDrawStyle(DrawStyle s)
{
    Q_D(Mol3dView);

    if (d->drawStyle == s)
        return;

    d->drawStyle = s;

    for (int i = 0; i < d->atomEntities.size(); ++i)
    {
        float radius;
        if (d->drawStyle == DrawStyle::BallAndStick)
        {
            Element element(d->currentStructure.atoms[i].element);
            radius = 0.8 * element.empiricalRadius();
            if (radius <= 0)
                radius = 0.8 * Element::fromAtomicNumber(6).empiricalRadius();
        }
        else // DrawStyle::Stick
        {
            radius = 0.125;
        }

        d->atomEntities[i].entity->setRadius(radius);
    }

    d->updateCamera();
}

Mol3dView::DrawStyle Mol3dView::getDrawStyle()
{
    Q_D(Mol3dView);
    return d->drawStyle;
}

void Mol3dView::setToolMode(ToolMode m)
{
    Q_D(Mol3dView);
    d->toolMode = m;
    if (m == ToolModeSelect)
        d->atomSelectionLimit = -1;
    d->clearSelection();

    d->updateStatusMessage();
}

Mol3dView::ToolMode Mol3dView::getToolMode()
{
    Q_D(Mol3dView);
    return d->toolMode;
}

void Mol3dView::setSelectionLimit(int maxAtoms, int maxBonds)
{
    Q_D(Mol3dView);
    d->atomSelectionLimit = maxAtoms;
    d->bondSelectionLimit = maxBonds;

    d->updateStatusMessage();
}

void Mol3dView::setHighlight(Selection s)
{
    Q_D(Mol3dView);

    QSet<int> updateItems;
    for (auto id: d->highlightAtoms)
    {
        d->atomEntities[id].highlighted = false;
        updateItems.insert(id);
    }
    d->highlightAtoms = {};

    for (auto id: s.atoms)
    {
        d->atomEntities[id].highlighted = true;
        updateItems.insert(id);
    }
    d->highlightAtoms = s.atoms;


    for (auto id: updateItems)
        d->syncAtomHighlight(id);

    //TODO: Implement bond highlights
}

Mol3dView::Selection Mol3dView::getSelection()
{
    Q_D(Mol3dView);
    return Selection{d->selectionAtoms, d->selectionBonds};
}

void Mol3dView::setSelection(const Selection &selection)
{
    Q_D(Mol3dView);
    for (auto const &a: selection.atoms)
        d->setAtomSelected(a, true);

    emit selectionChanged(getSelection());
}

void Mol3dView::setAddRGroup(MolStruct m)
{
    Q_D(Mol3dView);
    bool hasRGroup = false;
    for (auto const &a: m.atoms)
    {
        if (a.element == QStringLiteral("R1"))
        {
            hasRGroup = true;
            break;
        }
    }

    if (hasRGroup)
        d->addToolRGroup = m;
}
