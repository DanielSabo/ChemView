#ifndef TOOLBARMEASUREWIDGET_H
#define TOOLBARMEASUREWIDGET_H

#include "unitslineedit.h"

#include <QLabel>
#include <QWidget>

class Mol3dView;
class ToolbarMeasureWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolbarMeasureWidget(QWidget *parent, Mol3dView *view);

signals:

protected:
    Mol3dView *view;

    QLabel *label1;
    QLabel *label2;
    UnitsLineEdit *edit1;
    UnitsLineEdit *edit2;
    QList<int> targetAtoms;
};

#endif // TOOLBARMEASUREWIDGET_H
