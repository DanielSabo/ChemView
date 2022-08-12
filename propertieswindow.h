#ifndef PROPERTIESWINDOW_H
#define PROPERTIESWINDOW_H

#include "moldocument.h"

#include <QWidget>

namespace Ui {
class PropertiesWindow;
}

class PropertiesWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PropertiesWindow(QWidget *parent = nullptr);
    ~PropertiesWindow();

    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    void showData(MolDocument const &document, bool optimizerAvailable);

private:
    Ui::PropertiesWindow *ui;
};

#endif // PROPERTIESWINDOW_H
