#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "molstruct.h"
#include "mol3dview.h"
#include "optimizerbabelff.h"
#include "optimizerprogressdialog.h"
#include "preferenceswindow.h"
#include "toolbarpropswidget.h"
#include "nwchem.h"
#include "optimizernwchem.h"
#include "toolbaraddwidget.h"
#include "toolbarmeasurewidget.h"
#include "cubefile.h"
#include "propertieswindow.h"
#include "configurecalculationdialog.h"
#include "filehandlers.h"
#include "cvprojfile.h"
#include "calculation_util.h"
#include "optimizererrordialog.h"

#include <QSettings>
#include <QCloseEvent>
#include <QDebug>
#include <QWheelEvent>
#include <QFileDialog>
#include <QButtonGroup>
#include <QProcess>
#include <QTemporaryFile>
#include <QMessageBox>
#include <QProgressDialog>
#include <QMimeData>
#include <QWindow>
#include <QSaveFile>
#include <QDesktopServices>
#include <QActionGroup>

class MainWindowPrivate
{
public:
    MainWindowPrivate(MainWindow *q) : q_ptr(q) {};

    MainWindow * const q_ptr;
    Q_DECLARE_PUBLIC(MainWindow)

    Mol3dView *mol3dView = nullptr;
    QButtonGroup *toolbarButtonGroup = nullptr;
    QActionGroup *drawStyleActionGroup = nullptr;

    QLabel *statusBarRight = nullptr;

    PropertiesWindow *propertiesWindow = nullptr;

    NWChemConfiguration currentNWChemConfig;

    std::unique_ptr<Optimizer> optimizer;
    std::unique_ptr<OptimizerProgressDialog> optimizerDialog;
    bool optimizerChangedGeometry;

    std::unique_ptr<Optimizer> savedOptimizer;
    int activeAnimation;
    QString activeSurface;
    float activeSurfaceThreshold;
    MolDocument currentDocument;

    void replaceToolWidget(QWidget *w);
    void updatePropertiesWindow(bool ifHidden = false);
    void moleculeChanged();
    void runOptimizer(std::unique_ptr<Optimizer> optimizer , QString title, bool saveOptimizer);
};

void MainWindowPrivate::replaceToolWidget(QWidget *w)
{
    Q_Q(MainWindow);
    q->ui->horizontalLayout->replaceWidget(q->ui->toolWidgetPlaceholder, w);
    delete q->ui->toolWidgetPlaceholder;
    q->ui->toolWidgetPlaceholder = w;
}

void MainWindowPrivate::updatePropertiesWindow(bool ifHidden)
{
    if (propertiesWindow->isVisible() || ifHidden)
    {
        bool optimizerAvailable = (qobject_cast<OptimizerNWChem *>(savedOptimizer.get()) ||
                                   qobject_cast<OptimizerNWChem *>(optimizer.get()));
        propertiesWindow->showData(currentDocument, optimizerAvailable);
    }
}

void MainWindowPrivate::moleculeChanged()
{
    Q_Q(MainWindow);
    savedOptimizer.reset();
    currentDocument = MolDocument(mol3dView->getMolStruct());
    activeAnimation = -1;
    q->setWindowModified(true);
    updatePropertiesWindow();
    q->syncMenuStates();

    QString rightBarMessage;
    if (!currentDocument.molecule.isEmpty())
    {
        int charge = calc_util::overallCharge(currentDocument.molecule);
        int spin = calc_util::overallSpin(currentDocument.molecule);

        if (charge > 0)
            rightBarMessage = QStringLiteral("Charge: %1+").arg(charge);
        else if (charge < 0)
            rightBarMessage = QStringLiteral("Charge: %1-").arg(-charge);

        if (!rightBarMessage.isEmpty())
            rightBarMessage += " | ";

        if (spin > 0)
            rightBarMessage += QStringLiteral("Spin: %1").arg(spin);
        else if (spin < 0)
            rightBarMessage += QStringLiteral("Spin: <b>%1?</b>").arg(-spin);
        else
            rightBarMessage += QStringLiteral("Spin: <b>Error</b>");
    }

    statusBarRight->setText(rightBarMessage);
}

void MainWindowPrivate::runOptimizer(std::unique_ptr<Optimizer> opt, QString title, bool saveOptimizer)
{
    Q_Q(MainWindow);
    optimizer = std::move(opt);
    optimizerChangedGeometry = false;

    optimizerDialog.reset(new OptimizerProgressDialog());
    optimizerDialog->setWindowModality(Qt::ApplicationModal);
    optimizerDialog->setText(title);

    q->connect(optimizerDialog.get(), &QDialog::finished, q, [this](int){
        // Result is ignored because the dialog will only close if rejected
        optimizer->kill();

        optimizerDialog.reset();
        optimizer.reset();
    });

    q->connect(optimizer.get(), &Optimizer::statusMessage, q, [this](QString msg){
        optimizerDialog->setText(msg);
    });

    q->connect(optimizer.get(), &Optimizer::geometryUpdate, q, [this](){
        mol3dView->showMolStruct(optimizer->getStructure());
        optimizerChangedGeometry = true;
    });

    q->connect(optimizer.get(), &Optimizer::finished, q, [this, saveOptimizer, q](){
        optimizerDialog.reset();
        auto error = optimizer->getError();
        if (!error.isEmpty())
        {
            qDebug() << "Optimize failed:";
            qDebug() << qPrintable(error);

            OptimizerErrorDialog errorDialog;
            errorDialog.setText(error);
            errorDialog.setHasGeometry(optimizerChangedGeometry);
            errorDialog.setOutputURL(optimizer->getLogFile());
            if (!errorDialog.exec())
            {
                mol3dView->undo(); // Reset the geometry
            }
        }
        else
        {
            MolDocument result = optimizer->getResults();
            mol3dView->showMolStruct(result.molecule);
            if (result.volumes.contains(activeSurface))
            {
                mol3dView->showVolumeData(result.volumes.value(activeSurface), activeSurfaceThreshold);
            }
            currentDocument = result;
            q->setWindowModified(true);
            updatePropertiesWindow();
        }

        optimizer->kill();

        if (saveOptimizer && error.isEmpty())
        {
            savedOptimizer = std::move(optimizer);
            QObject::disconnect(savedOptimizer.get(), 0, 0, 0);
        }
        else
        {
            optimizer.release()->deleteLater();
        }

        q->syncMenuStates();
    });

    mol3dView->addUndoEvent();

    if (!optimizer->run())
    {
        optimizerDialog.reset();
        auto error = optimizer->getError();
        optimizer->kill();
        optimizer.release()->deleteLater();

        mol3dView->undo();

        qDebug() << "Optimize failed:";
        qDebug() << qPrintable(error);

        OptimizerErrorDialog errorDialog;
        errorDialog.setText(error);
        errorDialog.exec();
    }

    if (optimizerDialog)
        optimizerDialog->exec();
}


/* --------------------------------------------------------------------- */

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      d_ptr(new MainWindowPrivate(this)),
      ui(new Ui::MainWindow)
{
    Q_D(MainWindow);

    ui->setupUi(this);
    connectActions();

    QSettings appSettings;

    if (appSettings.contains("MainWindow/geometry"))
        restoreGeometry(appSettings.value("MainWindow/geometry").toByteArray());

    if (appSettings.contains("NWChemConfig"))
    {
        d->currentNWChemConfig = NWChemConfiguration(appSettings.value("NWChemConfig").toByteArray());
    }
    else
    {
        d->currentNWChemConfig.method = NWChemConfiguration::MethodDFT;
        d->currentNWChemConfig.basis = "6-311G**";
        d->currentNWChemConfig.functional = "B3LYP";
        d->currentNWChemConfig.driverMaxIter = 100;
        d->currentNWChemConfig.taskOpt = true;
        d->currentNWChemConfig.taskFreq = true;
    }


#if defined(Q_OS_MAC)
    {
        ui->line->setHidden(true);
    }
#endif

    // Set up toolbar
    d->toolbarButtonGroup = new QButtonGroup(this);
    d->toolbarButtonGroup->addButton(ui->toolAddButton);
    d->toolbarButtonGroup->addButton(ui->toolDeleteButton);
    d->toolbarButtonGroup->addButton(ui->toolBondButton);
    d->toolbarButtonGroup->addButton(ui->toolPropsButton);
    d->toolbarButtonGroup->addButton(ui->toolMeasureButton);
    // https://stackoverflow.com/questions/34603778/cannot-connect-qbuttongroup-buttonclicked-to-a-functor
    connect(d->toolbarButtonGroup, static_cast<void(QButtonGroup::*)(QAbstractButton *button)>(&QButtonGroup::buttonClicked), this, &MainWindow::toolButtonClicked);
    connect(ui->toolCleanUpButton, &QPushButton::clicked, this, &MainWindow::cleanUpButtonClicked);

    // Set up status bar
    d->statusBarRight = new QLabel("");
    ui->statusbar->addPermanentWidget(d->statusBarRight);

    // Draw style group
    d->drawStyleActionGroup = new QActionGroup(this);
    d->drawStyleActionGroup->addAction(ui->actionStyle_Ball_and_Stick);
    d->drawStyleActionGroup->addAction(ui->actionStyle_Stick);

    // Set up 3d view
    d->mol3dView = new Mol3dView;
    d->mol3dView->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    ui->centralLayout->addWidget(d->mol3dView);

    d->mol3dView->getViewWindow()->installEventFilter(this);

    connect(d->mol3dView, &Mol3dView::hoverInfo, this,
            [this](QString info) {
                ui->statusbar->showMessage(info);
            });

    connect(d->mol3dView, &Mol3dView::moleculeChanged, this,
            [d]() {
                d->moleculeChanged();
            });

    setAcceptDrops(true);

    syncMenuStates();

    // Sync 3d view state to the active tool
    MainWindow::toolButtonClicked(d->toolbarButtonGroup->checkedButton());

    //FIXME: With MacOS + Qt6 focusing the view here prevents the menus from initializing
    //       until it loses focus somehow.
//    d->mol3dView->setFocus();

    setWindowTitle("Untitled[*]");
    setWindowModified(false);
    setWindowFilePath({});

    d->propertiesWindow = new PropertiesWindow(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectActions()
{
    // File
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::actionNew);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::actionOpenFile);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::actionSave);
    connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::actionSaveAs);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::actionQuit);

    // Edit
    connect(ui->actionUndo, &QAction::triggered, this, &MainWindow::actionUndo);
    connect(ui->actionRedo, &QAction::triggered, this, &MainWindow::actionRedo);

    // View
    connect(ui->actionStyle_Ball_and_Stick, &QAction::triggered, this, &MainWindow::actionStyleBallandStick);
    connect(ui->actionStyle_Stick, &QAction::triggered, this, &MainWindow::actionStyleStick);
    connect(ui->actionShow_Info, &QAction::triggered, this, &MainWindow::actionShowInfo);

    // Calculate
    connect(ui->actionConfigure_NWChem, &QAction::triggered, this, &MainWindow::actionConfigureNWChem);
    connect(ui->actionNWChemOptimize, &QAction::triggered, this, &MainWindow::actionNWChemOptimize);
    connect(ui->actionNWChemSurfaceSpinTotal, &QAction::triggered, this, &MainWindow::actionNWChemSurfaceSpinTotal);
    connect(ui->actionNWChemSurfaceSpinDensity, &QAction::triggered, this, &MainWindow::actionNWChemSurfaceSpinDensity);
    connect(ui->actionView_Orbitals_Vibrations, &QAction::triggered, this, &MainWindow::actionShowInfo);
    connect(ui->actionNWChemOpenLogFile, &QAction::triggered, this, &MainWindow::actionNWChemOpenLogFile);

    // Settings
    connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::actionPreferences);
}

bool MainWindow::doSave(QString filename)
{
    Q_D(MainWindow);

    // TODO: This should probably get even smarter and consider data loss
    if (filename.isEmpty() || !FileHandlers::canSavePath(filename)) /* Save As... */
    {
        QString savePath;

        if (filename.isEmpty())
        {
            savePath = QSettings().value("MainWindow/openSavePath", QDir::homePath()).toString();
        }
        else
        {
            // If we can't save this filetype default to a file with the same basename
            int suffixLen = QFileInfo(filename).completeSuffix().length();
            savePath = filename.chopped(suffixLen) + ".mol";
        }

        filename = QFileDialog::getSaveFileName(this, "Save As...", savePath, FileHandlers::getSaveFilters());

        if (filename.isEmpty())
            return false;
    }

    QSettings().setValue("MainWindow/openSavePath", QFileInfo(filename).absoluteDir().absolutePath());

    QSaveFile file(filename);

    try
    {
        if(!file.open(QIODevice::WriteOnly))
            throw file.errorString();
        if (filename.endsWith(".cvproj"))
        {
            if (OptimizerNWChem *nwchemOpt = qobject_cast<OptimizerNWChem *>(d->savedOptimizer.get()))
                CVProjFile::write(&file, d->currentDocument, *nwchemOpt);
            else
                CVProjFile::write(&file, d->currentDocument);
        }
        else
        {
            QByteArray data;
            if (filename.endsWith(".nw"))
            {
                data = NWChem::molToOptimize(d->mol3dView->getMolStruct(),
                                             QStringLiteral("comp_") + QFileInfo(filename).baseName(),
                                             d->currentNWChemConfig);
            }
            else if (filename.endsWith(".xyz"))
            {
                data = d->mol3dView->getMolStruct().toXYZFile();
            }
            else if (filename.endsWith(".sdf") || filename.endsWith(".mol"))
            {
                data = d->mol3dView->getMolStruct().toMolFile();
            }
            else
            {
                throw QString("Uknown file extension");
            }
            file.write(data);
        }

        if (!file.commit())
            throw file.errorString();

    } catch (QString err) {
        QMessageBox errorDialog;
        errorDialog.setWindowTitle("Error while saving file");
        errorDialog.setText(QStringLiteral("Error while saving file:\n") + err);
        errorDialog.setIcon(QMessageBox::Critical);
        errorDialog.exec();
        return false;
    }

    setWindowTitle(QFileInfo(filename).fileName() + "[*]");
    setWindowModified(false);
    setWindowFilePath(filename);

    return true;
}

/* Prompt the user to save before continuing, return true if
 * the operation should continue (they saved or discarded).
 */
bool MainWindow::promptSave()
{
    if (isWindowModified())
    {
        QMessageBox savePrompt(this);
        savePrompt.setIcon(QMessageBox::Warning);
        if (windowFilePath().isEmpty())
            savePrompt.setText(tr("There are unsaved changes, are you sure you want to discard this structure?"));
        else
        {
            QString promptText = tr("There are unsaved changes to \"%1\". Are you sure you want to discard this structure?")
                                    .arg(QFileInfo(windowFilePath()).fileName());
            savePrompt.setText(promptText);
        }
        savePrompt.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        savePrompt.setDefaultButton(QMessageBox::Save);

        int result = savePrompt.exec();
        if (result == QMessageBox::Save)
        {
            if (!doSave(windowFilePath()))
                return false;
        }
        else if (result != QMessageBox::Discard)
        {
            return false;
        }
    }

    return true;
}

void MainWindow::actionQuit()
{
    close();
}

void MainWindow::actionNew()
{
    Q_D(MainWindow);

    if (!promptSave())
        return;

    d->mol3dView->clearUndoRedo();
    d->mol3dView->showMolStruct({});

    setWindowTitle("Untitled[*]");
    setWindowModified(false);
    setWindowFilePath({});
}

void MainWindow::actionOpenFile()
{
    QString defaultDir = QSettings().value("MainWindow/openSavePath", QDir::homePath()).toString();
    auto filename = QFileDialog::getOpenFileName(this, "Open File", defaultDir, FileHandlers::getOpenFilters());
    if (!filename.isEmpty())
    {
        QSettings().setValue("MainWindow/openSavePath", QFileInfo(filename).absoluteDir().absolutePath());
        openFile(filename);
    }
}

void MainWindow::actionSave()
{
    doSave(windowFilePath());
}

void MainWindow::actionSaveAs()
{
    doSave({});
}

void MainWindow::actionUndo()
{
    Q_D(MainWindow);
    d->mol3dView->undo();
}

void MainWindow::actionRedo()
{
    Q_D(MainWindow);
    d->mol3dView->redo();
}

void MainWindow::actionStyleBallandStick()
{
    Q_D(MainWindow);
    d->mol3dView->setDrawStyle(Mol3dView::DrawStyle::BallAndStick);
}

void MainWindow::actionStyleStick()
{
    Q_D(MainWindow);
    d->mol3dView->setDrawStyle(Mol3dView::DrawStyle::Stick);
}

void MainWindow::actionShowInfo()
{
    Q_D(MainWindow);
    d->updatePropertiesWindow(true);
    d->propertiesWindow->show();
    d->propertiesWindow->raise();
}

void MainWindow::actionConfigureNWChem()
{
    Q_D(MainWindow);
    ConfigureCalculationDialog dialog;
    dialog.setConfig(d->currentNWChemConfig);
    if(dialog.exec() == QDialog::Accepted)
    {
        d->currentNWChemConfig = dialog.getConfig();
        QSettings().setValue("NWChemConfig", d->currentNWChemConfig.serialize());
        syncMenuStates();
    }
}

void MainWindow::actionNWChemOptimize()
{
    Q_D(MainWindow);
    if (d->mol3dView->getMolStruct().atoms.empty())
        return;

    std::unique_ptr<OptimizerNWChem> optimizer(new OptimizerNWChem(this));
    optimizer->setStructure(d->mol3dView->getMolStruct());
    NWChemConfiguration &conf = d->currentNWChemConfig;
    optimizer->setConfiguration(conf);
    qDebug() << "NWChem Config:" << conf.generateConfig();

    d->runOptimizer(std::move(optimizer), QStringLiteral("Initializing NWChem Calculation..."), true);
}

void MainWindow::actionNWChemSurfaceSpinTotal()
{
    generateNWChemSurface("total", 1.0E-02);
}

void MainWindow::actionNWChemSurfaceSpinDensity()
{
    generateNWChemSurface("spindensity", 1.0E-02);
}

void MainWindow::actionNWChemOpenLogFile()
{
    Q_D(MainWindow);

    if (qobject_cast<OptimizerNWChem *>(d->savedOptimizer.get()) == nullptr)
        return;

    QUrl path = QUrl::fromLocalFile(d->savedOptimizer->getLogFile());

    if (!QDesktopServices::openUrl(path))
        qDebug() << "Faild to open log file:" << path;
}

void MainWindow::actionPreferences()
{
    auto prefWindow = new PreferencesWindow();
    prefWindow->setAttribute(Qt::WA_DeleteOnClose, true);
    prefWindow->show();
}

void MainWindow::toolButtonClicked(QAbstractButton *button)
{
    Q_D(MainWindow);
    d->replaceToolWidget(new QWidget(this));

    if (button == ui->toolAddButton)
    {
        auto widget = new ToolbarAddWidget(this, d->mol3dView);
        widget->activate();
        d->replaceToolWidget(widget);
    }
    else if (button == ui->toolDeleteButton)
        d->mol3dView->setToolMode(Mol3dView::ToolModeDelete);
    else if (button == ui->toolBondButton)
        d->mol3dView->setToolMode(Mol3dView::ToolModeBond);
    else if (button == ui->toolPropsButton)
    {
        d->mol3dView->setToolMode(Mol3dView::ToolModeSelectOne);
        d->replaceToolWidget(new ToolbarPropsWidget(this, d->mol3dView));
    }
    else if (button == ui->toolMeasureButton)
    {
        d->mol3dView->setToolMode(Mol3dView::ToolModeSelect);
        d->replaceToolWidget(new ToolbarMeasureWidget(this, d->mol3dView));
    }
}

void MainWindow::cleanUpButtonClicked()
{
    Q_D(MainWindow);
    if (d->mol3dView->getMolStruct().atoms.empty())
        return;

    std::unique_ptr<OptimizerBabelFF> optimizer(new OptimizerBabelFF(this));
    optimizer->setStructure(d->mol3dView->getMolStruct());
    d->runOptimizer(std::move(optimizer), "OpenBabel Force Field Optmization", false);
}

void MainWindow::generateNWChemSurface(QString name, float threshold)
{
    Q_D(MainWindow);

    if (qobject_cast<OptimizerNWChem *>(d->savedOptimizer.get()) == nullptr)
        return;

    d->activeSurface = name;
    d->activeSurfaceThreshold = threshold;

    if (d->currentDocument.volumes.contains(name))
    {
        animateFrequency(-1);
        d->mol3dView->showVolumeData(d->currentDocument.volumes.value(d->activeSurface), d->activeSurfaceThreshold);
    }
    else
    {
        std::unique_ptr<OptimizerNWChem> optimizer(qobject_cast<OptimizerNWChem *>(d->savedOptimizer.release()));
        optimizer->requestSurface(d->activeSurface);
        d->runOptimizer(std::move(optimizer), "Generating NWChem surfaces...", true);
    }
}

void MainWindow::animateFrequency(int index)
{
    Q_D(MainWindow);

    if (d->activeAnimation == index)
    {
        d->mol3dView->showAnimation({}, 0.2f);
        d->activeAnimation = -1;
        return;
    }

    if (index >= 0 && index < d->currentDocument.frequencies.size())
    {
        d->mol3dView->showVolumeData({}, 0.0f);
        d->mol3dView->showAnimation(d->currentDocument.frequencies[index].eigenvector, 0.2f);
        d->activeAnimation = index;
    }
    else
    {
        d->mol3dView->showAnimation({}, 0.2f);
        d->activeAnimation = -1;
    }
}

namespace {
    bool checkDragUrls(QList<QUrl> const &urls)
    {
        for (const auto &url: urls)
        {
            if (!url.isLocalFile())
                return false;
            QString path = url.toLocalFile();
            if (!FileHandlers::canOpenPath(path))
                return false;
        }
        return true;
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (checkDragUrls(urls))
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();

//    QString path = urls.at(0).toLocalFile();
//    qDebug() << urls.at(0).toString() << path;

    if (checkDragUrls(urls))
    {
        openFile(urls.at(0).toLocalFile());
        event->acceptProposedAction();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!promptSave())
    {
        event->ignore();
        return;
    }

    QSettings().setValue("MainWindow/geometry", saveGeometry());
//    QSettings().setValue("MainWindow/statusBar", ui->statusBar->isVisible());

    event->accept();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    Q_D(MainWindow);

//    qDebug() << "ev" << obj << event->type();
    if (obj == d->mol3dView->getViewWindow())
    {
        if (event->type() == QEvent::DragEnter)
            dragEnterEvent(static_cast<QDragEnterEvent *>(event));
        else if (event->type() == QEvent::DragMove)
            dragMoveEvent(static_cast<QDragMoveEvent *>(event));
        else if (event->type() == QEvent::DragLeave)
            dragLeaveEvent(static_cast<QDragLeaveEvent *>(event));
        else if (event->type() == QEvent::Drop)
            dropEvent(static_cast<QDropEvent *>(event));
        else
            return false;
        return true;
    }

    return false;
}

void MainWindow::syncMenuStates()
{
    Q_D(MainWindow);
    bool haveSavedOptimizer = qobject_cast<OptimizerNWChem *>(d->savedOptimizer.get());
    ui->actionNWChemSurfaceSpinTotal->setEnabled(haveSavedOptimizer);
    ui->actionNWChemSurfaceSpinDensity->setEnabled(haveSavedOptimizer);
    ui->actionNWChemOpenLogFile->setEnabled(haveSavedOptimizer);

    bool hasFrequencies = !d->currentDocument.frequencies.isEmpty();
    QString infoItemText;
    if (hasFrequencies)
        infoItemText = "View Orbitals/Vibrations...";
    else
        infoItemText = "View Orbitals...";

    ui->actionView_Orbitals_Vibrations->setEnabled(haveSavedOptimizer || hasFrequencies);

    QString optimizeItemText;
    if (d->currentNWChemConfig.taskFreq && d->currentNWChemConfig.taskOpt)
        optimizeItemText = QStringLiteral("NWChem Optimization/Frequencies: ");
    else if (d->currentNWChemConfig.taskOpt)
        optimizeItemText = QStringLiteral("NWChem Optimization: ");
    else if (d->currentNWChemConfig.taskFreq)
        optimizeItemText = QStringLiteral("NWChem Single-Point Frequencies: ");
    else
        optimizeItemText = QStringLiteral("NWChem Single-Point Energy: ");

    if (d->currentNWChemConfig.method == NWChemConfiguration::MethodSCF)
        optimizeItemText += QStringLiteral("Hartree-Fock/%1").arg(d->currentNWChemConfig.basis);
    else if (d->currentNWChemConfig.method == NWChemConfiguration::MethodDFT)
        optimizeItemText += QStringLiteral("DFT/%1/%2").arg(d->currentNWChemConfig.functional).arg(d->currentNWChemConfig.basis);
    else
        optimizeItemText += "???";
    ui->actionNWChemOptimize->setText(optimizeItemText);

    bool nonEmtpyMolecule = !(d->currentDocument.molecule.isEmpty());
    ui->actionNWChemOptimize->setEnabled(nonEmtpyMolecule);
    ui->toolCleanUpButton->setEnabled(nonEmtpyMolecule);

    Mol3dView::DrawStyle style = d->mol3dView->getDrawStyle();
    if (Mol3dView::DrawStyle::Stick == style)
        ui->actionStyle_Stick->setChecked(true);
    else // Mol3dView::DrawStyle::BallAndStick
        ui->actionStyle_Ball_and_Stick->setChecked(true);
}

void MainWindow::openFile(QString const &filename) {
    Q_D(MainWindow);

    QString activeSurface;
    MolDocument document;
    std::unique_ptr<OptimizerNWChem> loadedOpt;

    try {
        if (filename.endsWith(".cvproj"))
        {
            document = CVProjFile::fromPath(filename);
            try {
                loadedOpt = OptimizerNWChem::fromProjFile(filename, "molecule_nwchem", document.molecule);
            } catch (QString err) {
                QMessageBox::warning(this, "Error loading saved optimizer:", err);
                qDebug() << "Error loading saved optimizer:" << err;
                return;
            }
        }
        else if (filename.endsWith(".sdf") || filename.endsWith(".mol"))
        {
            document.molecule = MolStruct::fromSDF(filename);
        }
        else if (filename.endsWith(".cube"))
        {
            document = CubeFile::fromCube(filename);
            if (!document.volumes.isEmpty() && document.volumes.first().size())
                activeSurface = document.volumes.firstKey();
        }
        else if (filename.endsWith(".xyz"))
        {
            document.molecule = MolStruct::fromXYZ(filename);
        }
        else if (filename.endsWith(".out") || filename.endsWith(".nwout") || filename.endsWith(".log"))
        {
            document = NWChem::molFromOutputPath(filename);
        }
        else
            throw QString("Unknown file type");
    } catch (QString err) {
        QMessageBox::warning(this, "Failed to open file", err);
        qDebug() << "Failed to open file:" << err;
        return;
    }

    d->mol3dView->clearUndoRedo();
    d->mol3dView->showMolStruct(document.molecule);
    if (!activeSurface.isEmpty())
        d->mol3dView->showVolumeData(document.volumes.value(activeSurface), 1.0E-02);
    d->activeSurface = activeSurface;
    d->currentDocument = document;
    if (loadedOpt)
    {
        d->currentNWChemConfig = loadedOpt->getConfiguration();
        d->savedOptimizer = std::move(loadedOpt);
    }
    d->updatePropertiesWindow();
    setWindowTitle(QFileInfo(filename).fileName() + "[*]");
    setWindowModified(false);
    setWindowFilePath(filename);
    syncMenuStates();
}
