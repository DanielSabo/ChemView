#ifndef MOL3DVIEW_H
#define MOL3DVIEW_H

#include "molstruct.h"
#include "volumedata.h"

#include <QWidget>

class Mol3dViewPrivate;
class Mol3dView : public QWidget
{
    Q_OBJECT
public:
    explicit Mol3dView(QWidget *parent = nullptr);
    ~Mol3dView();

    bool eventFilter(QObject *obj, QEvent *event);
    QWindow *getViewWindow();

    void showMolStruct(const MolStruct &ms);
    void showVolumeData(const VolumeData &vol, float threshold = 0.0f);
    void showAnimation(QVector<QVector3D> eigenvector, float intensity);

    MolStruct getMolStruct();
    void rotate(float pitch, float yaw, float roll);

    enum class DrawStyle {
        BallAndStick,
        Stick
    };
    void setDrawStyle(DrawStyle s);
    DrawStyle getDrawStyle();

    enum ToolMode {
        ToolModeNone,
        ToolModeAdd,
        ToolModeAddValence,
        ToolModeDelete,
        ToolModeBond,
        ToolModeSelectOne,
        ToolModeSelect
    };
    void setToolMode(ToolMode m);
    ToolMode getToolMode();
    void setSelectionLimit(int maxSelected);

    struct Selection {
        QList<int> atoms;
        QList<int> bonds;
    };
    void setHighlight(Selection s);

    QList<int> getSelection();
    void setSelection(const QList<int> &selection);
    void modifySelectionCharge(int newCharge);
    void modifySelectionElement(int atomicNumber);
    void setAddRGroup(MolStruct m);

    void addUndoEvent();
    void undo();
    void redo();
    void clearUndoRedo();
    QPair<bool, bool> getUndoRedoState();

signals:
    void moleculeChanged();
    void selectionChanged(QList<int>);
    void hoverInfo(QString info);

private:
    QScopedPointer<Mol3dViewPrivate> const d_ptr;
    Q_DECLARE_PRIVATE(Mol3dView)
};

#endif // MOL3DVIEW_H
