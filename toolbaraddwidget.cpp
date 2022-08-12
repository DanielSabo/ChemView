#include "toolbaraddwidget.h"
#include "mol3dview.h"
#include "elementselector.h"
#include "element.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>
#include <QLabel>

static const QStringList names = {
    "Default",
    "32  Bent",
    "33  Trigonal planar",
    "41  Single",
    "42  Bent",
    "43  Trigonal pyramidal",
    "44  Tetrahedral",
    "52  Linear",
    "53a T-Shaped (axial)",
    "53r T-Shaped (radial)",
    "54a Seesaw (axial)",
    "54r Seesaw (radial)",
    "55a Trigonal bipyramidal (axial)",
    "55r Trigonal bipyramidal (radial)",
    "64  Square planar",
    "65a Square pyramidal (axial)",
    "65r Square pyramidal (radial)",
    "66  Octahedral",
};

static const QStringList geometries = {
    "",
    ":/structure/32_bent.mol",
    ":/structure/33_trigonal_planar.mol",
    ":/structure/41_single.mol",
    ":/structure/42_bent.mol",
    ":/structure/43_trigonal_pyramidal.mol",
    ":/structure/44_tetrahedral.mol",
    ":/structure/52_linear.mol",
    ":/structure/53_T_shaped_(axial).mol",
    ":/structure/53_T_shaped_(radial).mol",
    ":/structure/54_seesaw_(axial).mol",
    ":/structure/54_seesaw_(radial).mol",
    ":/structure/55_trigonal_bipyramidal_(axial).mol",
    ":/structure/55_trigonal_bipyramidal_(radial).mol",
    ":/structure/64_square_planar.mol",
    ":/structure/65_square_pyramidal_(axial).mol",
    ":/structure/65_square_pyramidal_(radial).mol",
    ":/structure/66_octahedral.mol",
};

// https://en.wikipedia.org/wiki/Organometallic_chemistry
static const QMap<QString, QString> defaultGeometry = {
    {"B", ":/structure/33_trigonal_planar.mol"},
    {"C", ":/structure/44_tetrahedral.mol"},
    {"N", ":/structure/43_trigonal_pyramidal.mol"},
    {"O", ":/structure/42_bent.mol"},

    {"Si", ":/structure/44_tetrahedral.mol"},
    // P being 43 is also reasonable but you're probably trying to make a phosphate
    {"P", ":/structure/55_trigonal_bipyramidal_(axial).mol"},
    {"S", ":/structure/42_bent.mol"},

    {"As", ":/structure/43_trigonal_pyramidal.mol"},
    {"Se", ":/structure/42_bent.mol"},

    {"Sn", ":/structure/44_tetrahedral.mol"},
};

namespace {
    // Adjust the bond lenghts of the geometry's hydrogens to match the center element
    void adjustValenceLength(MolStruct &mol, int center)
    {
        QList<int> bondedHydrogens;
        for (auto &b: mol.bonds)
        {
            if ((b.from == center) && (mol.atoms[b.to].element == "H"))
                bondedHydrogens.append(b.to);
            else if ((b.to == center) && (mol.atoms[b.from].element == "H"))
                bondedHydrogens.append(b.from);
        }

        QVector3D centerPos = mol.atoms.at(center).posToVector();
        float bondLength = Element(1).estimateBondLength(Element(mol.atoms[center].element));

        for (int i: bondedHydrogens)
        {
            QVector3D bondVector = mol.atoms[i].posToVector() - centerPos;
             mol.atoms[i].setPos(bondVector.normalized() * bondLength + centerPos);
        }
    }
}

ToolbarAddWidget::ToolbarAddWidget(QWidget *parent, Mol3dView *v) : QWidget(parent), view(v)
{
    auto l = new QHBoxLayout(this);
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(4);
    selector = new ElementSelector(this);
    geometrySelector = new QComboBox(this);
    l->addWidget(new QLabel("Element:"));
    l->addWidget(selector);
    l->addWidget(new QLabel("Geometry:"));
    l->addWidget(geometrySelector);
    l->insertStretch(-1);

    geometrySelector->addItems(names);
    for (int i = 0; i < names.size(); ++i)
    {
        // TODO: https://stackoverflow.com/questions/59054556/how-to-disable-or-shorten-the-tooltip-delay-in-qtableview
        if (!geometries[i].isEmpty())
        {
            QString iconName = geometries[i];
            iconName.replace(".mol", ".png");
            geometrySelector->setItemData(i, QString("<img src='%1'>").arg(iconName), Qt::ToolTipRole);
        }
    }

    connect(selector, &ElementSelector::elementChanged, this, [this]() {
        QString abbr = selector->getSelectedElement();
        if (abbr == QStringLiteral("H"))
        {
            view->setToolMode(Mol3dView::ToolModeAddValence);
            QSignalBlocker blockerA(geometrySelector);
            geometrySelector->setCurrentIndex(0);
            geometrySelector->setEnabled(false);
        }
        else
        {
            view->setToolMode(Mol3dView::ToolModeAdd);
            QSignalBlocker blockerA(geometrySelector);
            geometrySelector->setCurrentIndex(0);
            geometrySelector->setEnabled(true);
        }

        updateStructure();
    });

    // https://stackoverflow.com/questions/34603778/cannot-connect-qbuttongroup-buttonclicked-to-a-functor
    connect(geometrySelector, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int idx) {
        (void)idx;
        updateStructure();
    });
}

void ToolbarAddWidget::activate()
{
    view->setToolMode(Mol3dView::ToolModeAdd);
    updateStructure();
}

void ToolbarAddWidget::updateStructure()
{
    QString abbr = selector->getSelectedElement();
    int idx = geometrySelector->currentIndex();

    MolStruct newGroup;
    if (idx < 0 || idx >= geometries.size())
        idx = 0;

    QString structurePath;
    if (idx == 0)
    {
        structurePath = defaultGeometry.value(abbr, ":/structure/41_single.mol");
        QSignalBlocker blockerA(geometrySelector);
        auto iter = std::find(geometries.begin(), geometries.end(), structurePath);
        if (iter != geometries.end())
            geometrySelector->setCurrentIndex(iter - geometries.begin());
        else
            geometrySelector->setCurrentIndex(3);
    }
    else
        structurePath = geometries[idx];

    try
    {
        newGroup = MolStruct::fromSDF(structurePath);
    }
    catch (QString err)
    {
        qCritical() << "Failed to open internal structure file!:" << structurePath;
        qCritical() << err;
    }

    QList<int> replacedAtoms = newGroup.replaceElement("R0", abbr);
    for (int i: replacedAtoms)
        adjustValenceLength(newGroup, i);
    view->setAddRGroup(newGroup);
}
