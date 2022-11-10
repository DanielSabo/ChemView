#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAbstractButton>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindowPrivate;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void openFile(QString filename);

    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);

    void syncMenuStates();

public slots:
    void actionQuit();
    void actionNew();
    void actionOpenFile();
    void actionClose();
    void actionSave();
    void actionSaveAs();

    void actionUndo();
    void actionRedo();

    void actionStyleBallandStick();
    void actionStyleStick();
    void actionShowInfo();

    void actionConfigureNWChem();
    void actionNWChemOptimize();
    void actionNWChemSurfaceSpinTotal();
    void actionNWChemSurfaceSpinDensity();
    void actionNWChemOpenLogFile();

    void actionPreferences();

    void toolButtonClicked(QAbstractButton *button);
    void cleanUpButtonClicked();

    void generateNWChemSurface(QString name, float threshold);
    void animateFrequency(int index);

private:
    QScopedPointer<MainWindowPrivate> const d_ptr;
    Q_DECLARE_PRIVATE(MainWindow)

private:
    Ui::MainWindow *ui;

    void connectActions();

    bool doSave(QString filename, bool saveAs = false);
    bool promptSaveCurrentTab();
    bool promptSaveAll();
};
#endif // MAINWINDOW_H
