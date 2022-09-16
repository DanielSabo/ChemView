#ifndef TOOLBARPROPSWIDGET_H
#define TOOLBARPROPSWIDGET_H

#include <QWidget>
#include <QSpinBox>
#include <QLabel>

class Mol3dView;
class ElementSelector;
class ToolbarPropsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolbarPropsWidget(QWidget *parent, Mol3dView *view);

protected:
    QLabel *chargeOrderLabel;
    QSpinBox *chargeOrderSpinBox;
    QLabel *selectorLabel;
    ElementSelector *selector;
    Mol3dView *view;

    void atomMode(int charge, QString element);
    void bondMode(int order);
};

#endif // TOOLBARPROPSWIDGET_H
