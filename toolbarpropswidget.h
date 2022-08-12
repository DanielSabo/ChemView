#ifndef TOOLBARPROPSWIDGET_H
#define TOOLBARPROPSWIDGET_H

#include <QWidget>
#include <QSpinBox>

class Mol3dView;
class QSpinBox;
class ElementSelector;
class ToolbarPropsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolbarPropsWidget(QWidget *parent, Mol3dView *view);

protected:
    QSpinBox *spin;
    ElementSelector *selector;
    Mol3dView *view;
};

#endif // TOOLBARPROPSWIDGET_H
