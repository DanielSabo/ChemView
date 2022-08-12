#ifndef UNITSLINEEDIT_H
#define UNITSLINEEDIT_H

#include <QLineEdit>

class UnitsLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit UnitsLineEdit(QWidget *parent = nullptr);
    explicit UnitsLineEdit(const QString &text, const QString &units, QWidget *parent = nullptr);
    ~UnitsLineEdit();

    void setUnits(const QString &units);
    QString units();

    void paintEvent(QPaintEvent *) override;

    QString unitString;
protected:
    bool readOnlyFlag = false;
};

#endif // UNITSLINEEDIT_H
