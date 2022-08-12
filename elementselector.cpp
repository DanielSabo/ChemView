#include "elementselector.h"
#include "element.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>
#include <QKeyEvent>
#include <QLabel>

class ElementSelectorPrivate
{
public:
    ElementSelectorPrivate(ElementSelector *q) : q_ptr(q) {}

    QString selectedElement;
    QString pendingKeys;
    QLabel *popupWidget = new QLabel();

    void showPopup();

    ElementSelector * const q_ptr;
    Q_DECLARE_PUBLIC(ElementSelector)
};

void ElementSelectorPrivate::showPopup()
{
    Q_Q(ElementSelector);
    QPoint parentPos = q->mapToGlobal(QPoint(0, 0));
    QSize popupSize = popupWidget->sizeHint();
    int originX = parentPos.x() + (q->width() - popupSize.width()) / 2;
    int originY = parentPos.y() + q->height() + 4;
    popupWidget->show();
    popupWidget->move(originX, originY);
}


ElementSelector::ElementSelector(QWidget *parent) : QPushButton(parent),
  d_ptr(new ElementSelectorPrivate(this))
{
    Q_D(ElementSelector);

    d->selectedElement = "C";
    setText(d->selectedElement);
    setFocusPolicy(Qt::ClickFocus);

    d->popupWidget->setText("<center>Enter an element abbreviation, for single<br>"
                            "letters type the abbreviation twice (e.g. CC).</center>");
    d->popupWidget->setFrameStyle(QFrame::Box | QFrame::Plain);
    d->popupWidget->setWindowFlags(Qt::ToolTip);

    connect(this, &QPushButton::clicked, this, [this](){
        this->setFocus();
    });
}

ElementSelector::~ElementSelector()
{
}

QString ElementSelector::getSelectedElement()
{
    Q_D(ElementSelector);

    return d->selectedElement;
}

void ElementSelector::setSelectedElement(QString e)
{
    Q_D(ElementSelector);
    d->selectedElement = e;
    setText(d->selectedElement);
}

void ElementSelector::focusInEvent(QFocusEvent *)
{
    Q_D(ElementSelector);
    d->pendingKeys = "";
    setText("<>");
    d->showPopup();
}

void ElementSelector::focusOutEvent(QFocusEvent *)
{
    Q_D(ElementSelector);
    setText(d->selectedElement);
    d->popupWidget->hide();
}

void ElementSelector::keyPressEvent(QKeyEvent *e)
{
    Q_D(ElementSelector);
    if (e->matches(QKeySequence::Cancel))
    {
        clearFocus();
        return;
    }
    else if (e->key() == Qt::Key_Backspace)
    {
        if (d->pendingKeys.length() > 0)
            d->pendingKeys.resize(d->pendingKeys.length() - 1);

        if (d->pendingKeys.length() == 0)
            setText("<>");
        else
            setText(d->pendingKeys + "...");
    }
    else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)
    {
        if (d->pendingKeys.length() == 1)
        {
            auto element = Element::fromAbbr(d->pendingKeys);
            if (element.isValid())
                d->selectedElement = element.abbr;
            emit elementChanged(d->selectedElement);
        }

        d->pendingKeys = QString();
        clearFocus();
        return;
    }
    else
    {
        //TODO: Ensure there aren't any other e->text() edge cases
        if (!e->text().isEmpty() && e->text()[0].isLetter())
            d->pendingKeys += e->text()[0];

        if (d->pendingKeys.length() >= 2)
        {
            QString abbr = QString(d->pendingKeys[0].toUpper()) + QString(d->pendingKeys[1]);
            QString newElement;
            if (Element::fromAbbr(abbr).isValid())
            {
                newElement = abbr;
            }
            else if (Element::fromAbbr(abbr.left(1)).isValid())
            {
                newElement = abbr.left(1);
            }

            if (!newElement.isEmpty())
            {
                d->selectedElement = newElement;
                emit elementChanged(d->selectedElement);
            }

            d->pendingKeys = QString();

            clearFocus();
            return;
        }
        else if (d->pendingKeys.length() == 1)
        {
            setText(d->pendingKeys + "...");
        }
        return;
    }
}
