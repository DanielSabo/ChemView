#ifndef PREFERENCESWINDOW_H
#define PREFERENCESWINDOW_H

#include <QWidget>

namespace Ui {
class PreferencesWindow;
}

class PreferencesWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PreferencesWindow(QWidget *parent = nullptr);
    ~PreferencesWindow();

    void saveSettings();
    void discardChanges();

private:
    Ui::PreferencesWindow *ui;
};

#endif // PREFERENCESWINDOW_H
