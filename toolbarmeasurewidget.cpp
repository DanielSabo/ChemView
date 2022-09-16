#include "toolbarmeasurewidget.h"
#include "mol3dview.h"
#include "molstructgraph.h"

#include <cmath>
#include <QHBoxLayout>
#include <QDoubleValidator>

namespace {

// Calculate the angle between bonds (ab) and (bc)
double calculateBondAngle(Vector3D const &a, Vector3D const &b, Vector3D const &c)
{
    // The angle between two vectors is arccos((a dot b)/(|a|*|b|))
    Vector3D vecA = a - b;
    Vector3D vecB = c - b;
    double dot = Vector3D::dotProduct(vecA, vecB)/(vecA.length()*vecB.length());
    dot = std::max(std::min(dot, 1.0), -1.0);
    return acos(dot) * (180.0 / M_PI);
}

// Calculate the angle between bonds (ab) and (bc)
double calculateBondAngle(MolStruct const &mol, int a, int b, int c)
{
    Vector3D aPos = mol.atoms[a].posToVector3D();
    Vector3D bPos = mol.atoms[b].posToVector3D();
    Vector3D cPos = mol.atoms[c].posToVector3D();
    return calculateBondAngle(aPos, bPos, cPos);
}

// Calculate the dihedral angle between bonds (ab) and (cd)
double calculateDihedralAngle(Vector3D const &a, Vector3D const &b, Vector3D const &c, Vector3D const &d)
{
    Vector3D vecA = a - b;
    Vector3D vecB = d - c;
    Vector3D axis = c - b;

    // The angle between two planes is the angle between their normal vectors
    Vector3D nA = Vector3D::crossProduct(vecA, axis);
    Vector3D nB = Vector3D::crossProduct(vecB, axis);
    double dot = Vector3D::dotProduct(nA, nB)/(nA.length()*nB.length());
    double s = Vector3D::dotProduct(Vector3D::crossProduct(nA, nB), axis);
    dot = std::max(std::min(dot, 1.0), -1.0);
    return std::copysign(acos(dot), s) * (180.0 / M_PI);
}

// Calculate the dihedral angle between bonds (ab) and (cd)
double calculateDihedralAngle(MolStruct const &mol, int a, int b, int c, int d)
{
    Vector3D aPos = mol.atoms[a].posToVector3D();
    Vector3D bPos = mol.atoms[b].posToVector3D();
    Vector3D cPos = mol.atoms[c].posToVector3D();
    Vector3D dPos = mol.atoms[d].posToVector3D();
    return calculateDihedralAngle(aPos, bPos, cPos, dPos);
}

void adjustBondAngle(MolStruct &mol, QList<int> const &targetAtoms, double newAngle)
{
    if (targetAtoms.size() != 3)
    {
        qWarning() << "adjustBondAngle: invalid targetAtoms list";
        return;
    }

    Vector3D aPos = mol.atoms[targetAtoms[0]].posToVector3D();
    Vector3D bPos = mol.atoms[targetAtoms[1]].posToVector3D();
    Vector3D cPos = mol.atoms[targetAtoms[2]].posToVector3D();

    //  qDebug() << aPos.QVec() << bPos.QVec() << cPos.QVec() << newAngle;
    double oldAngle = calculateBondAngle(aPos, bPos, cPos);

    double deltaAngle = newAngle - oldAngle;
//    qDebug() << QString::number(oldAngle,'f',10);
//    qDebug() << QString::number(deltaAngle,'f',10);

    Vector3D vecA = aPos - bPos;
    Vector3D vecB = cPos - bPos;
    Vector3D axis = Vector3D::crossProduct(vecA, vecB);

    //TODO: Validate how small we can safely make this treshold
    if (axis.lengthSquared() < 0.00001)
    {
        // The vectors are effectivly parallel so the axis of rotation is arbitary
        axis = vecA.orthognalVector();
    }
    axis.normalize();

    // TODO: Implement double precision rotation
    QMatrix4x4 transform;
    transform.translate(bPos.QVec());
    transform.rotate(deltaAngle, axis.QVec());
    transform.translate(-bPos.QVec());

    auto atomsToMove = mol.generateGraph().selectBranch(targetAtoms[1], targetAtoms[2]);

    for (auto id: atomsToMove)
        mol.atoms[id].setPos(transform.map(mol.atoms[id].posToVector()));
}

void adjustDihedralAngle(MolStruct &mol, QList<int> const &targetAtoms, double newAngle)
{
    if (targetAtoms.size() != 4)
    {
        qWarning() << "adjustDihedralAngle: invalid targetAtoms list";
        return;
    }

    Vector3D aPos = mol.atoms[targetAtoms[0]].posToVector3D();
    Vector3D bPos = mol.atoms[targetAtoms[1]].posToVector3D();
    Vector3D cPos = mol.atoms[targetAtoms[2]].posToVector3D();
    Vector3D dPos = mol.atoms[targetAtoms[3]].posToVector3D();

    double oldAngle = calculateDihedralAngle(aPos, bPos, cPos, dPos);

    double deltaAngle = newAngle - oldAngle;
//    qDebug() << QString::number(oldAngle,'f',10);
//    qDebug() << QString::number(deltaAngle,'f',10);

    Vector3D axis = cPos - bPos;
    axis.normalize();
//    qDebug() << axis.QVec() << deltaAngle;

    // TODO: Implement double precision rotation
    QMatrix4x4 transform;
    transform.translate(cPos.QVec());
    transform.rotate(deltaAngle, axis.QVec());
    transform.translate(-cPos.QVec());

    auto atomsToMove = mol.generateGraph().selectBranch(targetAtoms[1], targetAtoms[2]);

    for (auto id: atomsToMove)
        mol.atoms[id].setPos(transform.map(mol.atoms[id].posToVector()));

    aPos = mol.atoms[targetAtoms[0]].posToVector3D();
    bPos = mol.atoms[targetAtoms[1]].posToVector3D();
    cPos = mol.atoms[targetAtoms[2]].posToVector3D();
    dPos = mol.atoms[targetAtoms[3]].posToVector3D();
//    double postAngle = calculateDihedralAngle(aPos, bPos, cPos, dPos);
//    qDebug() << oldAngle << newAngle << postAngle;
}

}

ToolbarMeasureWidget::ToolbarMeasureWidget(QWidget *parent, Mol3dView *v) : QWidget(parent), view(v)
{
    auto l = new QHBoxLayout(this);
    l->setContentsMargins(0,0,0,0);

    label1 = new QLabel(this);
    label2 = new QLabel(this);
    edit1 = new UnitsLineEdit(this);
    edit2 = new UnitsLineEdit(this);


    // Symbols: Anstrom: \u212B Degreee : \u00B0

    label1->setVisible(true);
    label2->setVisible(false);
    edit1->setVisible(false);
    edit1->setAlignment(Qt::AlignRight);
    edit1->setUnits(QStringLiteral(u"\u212B"));
    edit1->setValidator(new QDoubleValidator(edit1));
    edit2->setVisible(false);
    edit2->setAlignment(Qt::AlignRight);
    edit2->setUnits(QStringLiteral(u"\u00B0"));
    edit2->setValidator(new QDoubleValidator(edit2));

    // The sizing of QLineEdit is arcane, but we can find the padding by
    // subtracting the width of the default string it sizes to (17 x's)
    QFontMetrics fontMetrics(edit1->font());
    int editHorizontalPad = edit1->sizeHint().width() - fontMetrics.horizontalAdvance(u'x') * 17;
    edit1->setFixedWidth(editHorizontalPad + fontMetrics.horizontalAdvance(QStringLiteral("000.000000")));
    edit2->setFixedWidth(editHorizontalPad + fontMetrics.horizontalAdvance(QStringLiteral("-000.0000")));

    l->addWidget(label1);
    l->addWidget(edit1);
    l->addWidget(label2);
    l->addWidget(edit2);
    l->addStretch();

    label1->setText("Select atoms to measure.");

    connect(edit1, &QLineEdit::returnPressed, this, [this]() {
        if (targetAtoms.size() != 2)
            return;

        auto mol = view->getMolStruct();

        QVector3D aPos = mol.atoms[targetAtoms[0]].posToVector();
        QVector3D bPos = mol.atoms[targetAtoms[1]].posToVector();
        QVector3D bondVector = bPos - aPos;
        float oldLength = bondVector.length();

        bool valid = false;
        float newLength = edit1->text().toDouble(&valid);

        // For sanity sake we also reject bond lengths less than the emperical radius of hydrogen
        if (!valid || newLength < 0.25)
        {
            edit1->setText(QString::number(oldLength));
            return;
        }
        else
        {
            auto atomsToMove = mol.generateGraph().selectBranch(targetAtoms[0], targetAtoms[1]);
            QVector3D delta = bondVector.normalized()*(newLength - oldLength);

            //TODO: Detect rings

            for (auto id: atomsToMove)
                mol.atoms[id].setPos(mol.atoms[id].posToVector() + delta);

            auto selection = view->getSelection();
            view->addUndoEvent("Adjust bond length");
            view->showMolStruct(mol);
            view->setSelection(selection);
        }
    });

    connect(edit2, &QLineEdit::returnPressed, this, [this]() {

        bool valid = false;
        float newAngle = edit2->text().toDouble(&valid);

        if (targetAtoms.size() == 3)
        {
            auto mol = view->getMolStruct();

            if (!valid)
            {
                double oldAngle = calculateBondAngle(mol, targetAtoms[0], targetAtoms[1], targetAtoms[2]);
                edit2->setText(QString::number(oldAngle));
                return;
            }

            adjustBondAngle(mol, targetAtoms, newAngle);

            auto selection = view->getSelection();
            view->addUndoEvent("Adjust bond angle");
            view->showMolStruct(mol);
            view->setSelection(selection);
        }
        else if (targetAtoms.size() == 4)
        {

            auto mol = view->getMolStruct();

            if (!valid)
            {
                double oldAngle = calculateDihedralAngle(mol, targetAtoms[0], targetAtoms[1], targetAtoms[2], targetAtoms[3]);
                edit2->setText(QString::number(oldAngle));
                return;
            }

            adjustDihedralAngle(mol, targetAtoms, newAngle);

            auto selection = view->getSelection();
            view->addUndoEvent("Adjust dihedral angle");
            view->showMolStruct(mol);
            view->setSelection(selection);
        }
    });

    connect(view, &Mol3dView::selectionChanged, this, [this](Mol3dView::Selection selection) {
                Mol3dView::Selection highlight;

                targetAtoms = {};

                if (selection.atoms.length() < 2)
                {
                    label1->setText("Select atoms to measure.");
                    label1->setVisible(true);
                    label2->setVisible(false);
                    edit1->setVisible(false);
                    edit2->setVisible(false);
                }
                else if (selection.atoms.length() == 2)
                {
                    auto mol = this->view->getMolStruct();
                    QVector<int> path = mol.generateGraph().findPath(selection.atoms[0], selection.atoms[1], 2);

                    auto const &a = mol.atoms[selection.atoms[0]];
                    auto const &b = mol.atoms[selection.atoms[1]];
                    float length = (a.posToVector() - b.posToVector()).length();

                    label1->setText(QStringLiteral(u"Distance:"));
                    if (path.size() == 2)
                    {
                        label1->setText(QStringLiteral(u"Bond length:"));
                        targetAtoms = {selection.atoms[0], selection.atoms[1]};
                    }
                    edit1->setText(QString::number(length));
                    label1->setVisible(true);
                    edit1->setVisible(true);

                    QString angleDescription;
                    QString angleValue;

                    if (path.size() == 3) // Do we have an angle? (common bond partner)
                    {
                        int common = path[1];
                        auto const &c = mol.atoms[common];
                        highlight.atoms.push_back(common);

                        double angle = calculateBondAngle(a.posToVector3D(), c.posToVector3D(), b.posToVector3D());
                        angleDescription = QStringLiteral(u"Angle:");
                        angleValue = QString::number(angle, 'f', 4);

                        targetAtoms = {selection.atoms[0], common, selection.atoms[1]};
                    }
                    else if (path.size() == 4) // Or do we have a dihedral
                    {
                        int common1 = path[1];
                        int common2 = path[2];
                        highlight.atoms.push_back(common1);
                        highlight.atoms.push_back(common2);

                        auto const &c1 = mol.atoms[common1];
                        auto const &c2 = mol.atoms[common2];

                        float angle = calculateDihedralAngle(a.posToVector3D(), c1.posToVector3D(), c2.posToVector3D(), b.posToVector3D());;
                        angleDescription = QStringLiteral(u"Dihedral:");
                        angleValue = QString::number(angle, 'f', 4);

                        if (!std::isnan(angle))
                            targetAtoms = {selection.atoms[0], common1, common2, selection.atoms[1]};
                        else
                            angleValue = "N/A";
                    }

                    if (angleDescription.isEmpty())
                    {
                        label2->setVisible(false);
                        edit2->setVisible(false);

                        if (path.size() == 2)
                            edit1->setReadOnly(false);
                        else
                            edit1->setReadOnly(true);
                    }
                    else
                    {
                        label2->setVisible(true);
                        label2->setText(angleDescription);
                        edit2->setVisible(true);
                        edit2->setText(angleValue);

                        edit1->setReadOnly(true);
                        if (targetAtoms.isEmpty()) // A NaN angle
                            edit2->setReadOnly(true);
                        else
                            edit2->setReadOnly(false);
                    }
                }
                else
                {
                    label1->setText("FIXME: max selection = 2");
                    label2->setVisible(false);
                    edit1->setVisible(false);
                    edit2->setVisible(false);
                }

                view->setHighlight(highlight);
            });
}
