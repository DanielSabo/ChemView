#include "toolbarpropswidget.h"
#include "mol3dview.h"
#include "elementselector.h"
#include "element.h"

#include <QHBoxLayout>

ToolbarPropsWidget::ToolbarPropsWidget(QWidget *parent, Mol3dView *v) : QWidget(parent), view(v)
{
    auto l = new QHBoxLayout(this);
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(4);

    chargeOrderLabel = new QLabel("Charge:", this);
    chargeOrderSpinBox = new QSpinBox(this);
    l->addWidget(chargeOrderLabel);
    l->addWidget(chargeOrderSpinBox);

    selectorLabel = new QLabel("Element:", this);
    selector = new ElementSelector(this);
    l->addWidget(selectorLabel);
    l->addWidget(selector);

    setEnabled(false);
    atomMode(0, "C");

    connect(chargeOrderSpinBox, static_cast<void(QSpinBox::*)(int value)>(&QSpinBox::valueChanged), this, [this](int value) {
        auto newStructure = view->getMolStruct();
        auto selection = view->getSelection();

        if (!selection.atoms.isEmpty())
        {
            bool modified = false;
            for (auto const &a: selection.atoms)
            {
                if (newStructure.atoms[a].charge != value)
                {
                    modified = true;
                    newStructure.atoms[a].charge = value;
                }
            }

            if (modified)
            {
                view->addUndoEvent("Adjust charge");
                view->showMolStruct(newStructure);
                view->setSelection(selection);
            }
        }
        else if (!selection.bonds.isEmpty())
        {
            bool modified = false;
            for (auto const &b: selection.bonds)
            {
                if (newStructure.bonds[b].order != value)
                {
                    modified = true;
                    newStructure.bonds[b].order = value;
                }
            }

            if (modified)
            {
                view->addUndoEvent("Adjust bond order");
                view->showMolStruct(newStructure);
                view->setSelection(selection);
            }
        }
    });

    connect(selector, &ElementSelector::elementChanged, this, [this](QString abbr){
        Element e = Element::fromAbbr(abbr);
        if (e.isValid())
        {
            auto newStructure = view->getMolStruct();
            auto selection = view->getSelection();

            bool modified = false;
            for (auto const &a: selection.atoms)
            {
                if (newStructure.atoms[a].element != e.abbr)
                {
                    modified = true;
                    newStructure.atoms[a].element = e.abbr;
                }
            }

            if (modified)
            {
                view->addUndoEvent("Change element");
                view->showMolStruct(newStructure);
                view->setSelection(selection);
            }
        }
    });

    connect(view, &Mol3dView::selectionChanged, this, [this](Mol3dView::Selection selection) {
        // Don't modifying the atom when we update the display values
        const QSignalBlocker blockerA(chargeOrderSpinBox);
        const QSignalBlocker blockerB(selector);

        if (!selection.atoms.isEmpty())
        {
            setEnabled(true);

            int id = selection.atoms.first();
            auto const &atom = view->getMolStruct().atoms.at(id);

            atomMode(atom.charge, atom.element);
        }
        else if (!selection.bonds.isEmpty())
        {
            setEnabled(true);

            int id = selection.bonds.first();
            auto const &bond = view->getMolStruct().bonds.at(id);
            bondMode(bond.order);
        }
        else
        {
            setEnabled(false);
            atomMode(0, "C");
        }
    });
}

void ToolbarPropsWidget::atomMode(int charge, QString element)
{
    chargeOrderLabel->setText("Charge:");
    chargeOrderSpinBox->setRange(-15,15);
    chargeOrderSpinBox->setValue(charge);
    selectorLabel->setVisible(true);
    selector->setSelectedElement(element);
    selector->setVisible(true);
}

void ToolbarPropsWidget::bondMode(int order)
{
    chargeOrderSpinBox->setRange(1,3);
    chargeOrderSpinBox->setValue(order);
    chargeOrderLabel->setText("Bond Order:");
    selectorLabel->setVisible(false);
    selector->setVisible(false);
}
