#include "toolbarpropswidget.h"
#include "mol3dview.h"
#include "elementselector.h"
#include "element.h"

#include <QHBoxLayout>
#include <QLabel>

ToolbarPropsWidget::ToolbarPropsWidget(QWidget *parent, Mol3dView *v) : QWidget(parent), view(v)
{
    auto l = new QHBoxLayout(this);
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(4);

    spin = new QSpinBox(this);
    spin->setRange(-15,15);
    l->addWidget(new QLabel("Charge:", this));
    l->addWidget(spin);

    selector = new ElementSelector(this);
    l->addWidget(new QLabel("Element:"));
    l->addWidget(selector);

    connect(spin, static_cast<void(QSpinBox::*)(int value)>(&QSpinBox::valueChanged), this, [this](int charge) {
        auto newStructure = view->getMolStruct();
        auto selection = view->getSelection();

        bool modifided = false;
        for (auto const &s: selection)
        {
            if (newStructure.atoms[s].charge != charge)
            {
                modifided = true;
                newStructure.atoms[s].charge = charge;
            }
        }

        if (modifided)
        {
            view->addUndoEvent("Adjust charge");
            view->showMolStruct(newStructure);
            view->setSelection(selection);
        }
    });


    connect(selector, &ElementSelector::elementChanged, this, [this](QString abbr){
        Element e = Element::fromAbbr(abbr);
        if (e.isValid())
        {
            auto newStructure = view->getMolStruct();
            auto selection = view->getSelection();

            bool modifided = false;
            for (auto const &s: selection)
            {
                if (newStructure.atoms[s].element != e.abbr)
                {
                    modifided = true;
                    newStructure.atoms[s].element = e.abbr;
                }
            }

            if (modifided)
            {
                view->addUndoEvent("Change element");
                view->showMolStruct(newStructure);
                view->setSelection(selection);
            }
        }
    });

    connect(view, &Mol3dView::selectionChanged, this, [this](QList<int> selection) {
        // Don't modifying the atom when we update the display values
        const QSignalBlocker blockerA(spin);
        const QSignalBlocker blockerB(selector);

        if (selection.isEmpty())
        {
            spin->setValue(0);
            setEnabled(false);
            //TODO: Add an "invalid/disabled" mode to the selector widget
            selector->setSelectedElement("C");
        }
        else
        {
            setEnabled(true);
            int id = selection.first();
            auto const &atom = view->getMolStruct().atoms.at(id);

            spin->setValue(atom.charge);
            selector->setSelectedElement(atom.element);
        }
    });

    if (view->getSelection().isEmpty())
        setEnabled(false);
}
