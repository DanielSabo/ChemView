#ifndef TOOLBARADDWIDGET_H
#define TOOLBARADDWIDGET_H

#include <QComboBox>
#include <QWidget>

class Mol3dView;
class ElementSelector;
class ToolbarAddWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolbarAddWidget(QWidget *parent, Mol3dView *view);
    void activate();

public slots:
    void updateStructure();

signals:

protected:
    Mol3dView *view;
    ElementSelector *selector;
    QComboBox *geometrySelector;
};

#endif // TOOLBARADDWIDGET_H
