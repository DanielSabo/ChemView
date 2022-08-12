#ifndef ELEMENTSELECTOR_H
#define ELEMENTSELECTOR_H

#include <QPushButton>

class ElementSelectorPrivate;
class ElementSelector : public QPushButton
{
    Q_OBJECT
public:
    explicit ElementSelector(QWidget *parent = nullptr);
    ~ElementSelector();

    QString getSelectedElement();
    void setSelectedElement(QString e);

    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);
    void keyPressEvent(QKeyEvent *e);

private:
    QScopedPointer<ElementSelectorPrivate> const d_ptr;
    Q_DECLARE_PRIVATE(ElementSelector)

signals:
    void elementChanged(QString abbr);

};

#endif // ELEMENTSELECTOR_H
