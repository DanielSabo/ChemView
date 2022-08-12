#include "unitslineedit.h"

#include <QDebug>
#include <QFontMetrics>
#include <QPainter>
#include <QStyleOptionFrame>

UnitsLineEdit::UnitsLineEdit(QWidget *parent) : UnitsLineEdit(QString(), QString(), parent)
{
}

UnitsLineEdit::UnitsLineEdit(const QString &text, const QString &units, QWidget *parent) : QLineEdit(text, parent)
{
    setUnits(units);
}

UnitsLineEdit::~UnitsLineEdit()
{
}

void UnitsLineEdit::setUnits(const QString &units)
{
    unitString = units;

    QFontMetrics fm(font());
    int unitsWidth = fm.horizontalAdvance(unitString);
    setTextMargins(0, 0, unitsWidth + 2, 0);
}

QString UnitsLineEdit::units()
{
    return unitString;
}

void UnitsLineEdit::paintEvent(QPaintEvent *event)
{
    if (readOnlyFlag != isReadOnly())
    {
        setStyleSheet("QLineEdit[readOnly=\"true\"] {background-color: " + palette().color(QPalette::Window).name() + ";}");
        readOnlyFlag = isReadOnly();
    }

    QLineEdit::paintEvent(event);

    QStyleOptionFrame panel;
    initStyleOption(&panel);
    QRect contentRect = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);

    QFontMetrics fm(font());
    int unitsWidth = fm.horizontalAdvance(unitString);
    static const int defaultPadding = 4; // Hard coded value from QLineEditPrivate
    contentRect.setLeft(contentRect.x() + contentRect.width() - unitsWidth - defaultPadding);
    contentRect.setWidth(unitsWidth);

    QPainter painter(this);
//    painter.setPen(palette().text().color());
    painter.setPen(palette().placeholderText().color());
    painter.setFont(font());
    painter.drawText(contentRect, Qt::AlignVCenter, unitString);
}
