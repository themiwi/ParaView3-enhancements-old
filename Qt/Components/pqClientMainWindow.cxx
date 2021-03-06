/*=========================================================================

   Program: ParaView
   Module:    $RCS $

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "pqClientMainWindow.h"

#include <vtkPQConfig.h>

#include "pqClientAboutDialog.h"

#include "ui_pqClientMainWindow.h"

#include <pqActiveView.h>
#include <pqAnimationManager.h>
#include <pqAnimationViewWidget.h>
#include <pqApplicationCore.h>
#include <pqColorScaleToolbar.h>
#include <pqComparativeVisPanel.h>
//#include <pqLookmarkToolbar.h>
#include <pqMainWindowCore.h>
#include <pqObjectBuilder.h>
#include <pqObjectInspectorDriver.h>
#include <pqObjectInspectorWidget.h>
#include <pqObjectNaming.h>
#include <pqPipelineBrowserContextMenu.h>
#include <pqPipelineBrowser.h>
#include <pqPipelineMenu.h>
#include <pqProgressManager.h>
#include <pqProxyTabWidget.h>
#include <pqRecentFilesMenu.h>
#include <pqRenderView.h>
#include <pqRubberBandHelper.h>
#include <pqScalarBarVisibilityAdaptor.h>
#include <pqSetName.h>
#include <pqSettings.h>
#include <pqUndoStack.h>
#include <pqVCRController.h>
#include <pqViewManager.h>
#include <pqViewMenu.h>
#include <pqHelpWindow.h>
#include <pqCoreTestUtility.h>


// Include pqPythonManager when compiled with python support
#ifdef PARAVIEW_ENABLE_PYTHON
#include <pqPythonManager.h>
#endif

#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QShortcut>
#include <QSpinBox>

//////////////////////////////////////////////////////////////////////////////
// pqClientMainWindow::pqImplementation

class pqClientMainWindow::pqImplementation
{
public:
  pqImplementation(QWidget* parent, pqMainWindowCore *core=NULL) :
    HelpWindow(0),
    RecentFilesMenu(0),
    ViewMenu(0),
    ToolbarsMenu(0)
  {
  if (core!=NULL)
    {
    core->setParent(parent);
    this->Core = core;
    }
  else
    {
    this->Core = new pqMainWindowCore(parent);
    }
  }

  ~pqImplementation()
  {
    delete this->ViewMenu;
    delete this->ToolbarsMenu;
    delete this->HelpWindow;
    delete this->Core;
  }

  QPointer<pqHelpWindow> HelpWindow;
  Ui::pqClientMainWindow UI;
  pqMainWindowCore *Core;
  pqRecentFilesMenu* RecentFilesMenu;
  pqViewMenu* ViewMenu;
  pqViewMenu* ToolbarsMenu;
  QLineEdit* CurrentTimeWidget;
  QSpinBox* CurrentTimeIndexWidget;
  QPointer<pqServer> ActiveServer;
  QString DocumentationDir;
};

//////////////////////////////////////////////////////////////////////////////
// pqClientMainWindow
#include <QTreeView>
#include "pqSILModel.h"
#include "vtkGraphReader.h"
#include "vtkExodusIIReaderParser.h"
#include "vtkMutableDirectedGraph.h"
pqClientMainWindow::pqClientMainWindow()
{
  this->Implementation = new pqImplementation(this);
  this->constructorHelper();
}

pqClientMainWindow::pqClientMainWindow(pqMainWindowCore *core)
{
  this->Implementation = new pqImplementation(this, core);
  this->constructorHelper();
}

void pqClientMainWindow::constructorHelper()
{
  this->Implementation->UI.setupUi(this);

  this->insertToolBarBreak(this->Implementation->UI.variableToolbar);
  this->insertToolBarBreak(this->Implementation->UI.commonFilters);

  this->setIconSize(QSize(24, 24));

  this->Implementation->RecentFilesMenu = new
    pqRecentFilesMenu(*this->Implementation->UI.menuRecentFiles, this);
  QObject::connect(this->Implementation->RecentFilesMenu,
    SIGNAL(serverConnectFailed()),
    this->Implementation->Core,
    SLOT(makeDefaultConnectionIfNoneExists()));

  this->Implementation->ViewMenu =
    new pqViewMenu(*this->Implementation->UI.menuView, this);
  this->Implementation->ToolbarsMenu =
    new pqViewMenu(*this->Implementation->UI.menuToolbars);
  this->Implementation->Core->setDockWindowMenu(this->Implementation->ViewMenu);
  this->Implementation->Core->setToolbarMenu(this->Implementation->ToolbarsMenu);

  QString suffix = "";
  if (PARAVIEW_VERSION_MINOR%2 != 0)
    {
    suffix = "(development)";
    }
    
  this->setWindowTitle(
    QString("Kitware ParaView %1 %2").arg(PARAVIEW_VERSION_FULL).arg(suffix));

  // Setup menus and toolbars ...
  connect(this->Implementation->UI.actionFileOpen,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onFileOpen()));

  connect(this->Implementation->UI.actionFileLoadServerState,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onFileLoadServerState()));
  connect(
    this->Implementation->Core,
    SIGNAL(enableFileLoadServerState(bool)),
    this->Implementation->UI.actionFileLoadServerState,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveServerState,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onFileSaveServerState()));
  connect(
    this->Implementation->Core,
    SIGNAL(enableFileSaveServerState(bool)),
    this->Implementation->UI.actionFileSaveServerState,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveData,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onFileSaveData()));
  connect(
    this->Implementation->Core,
    SIGNAL(enableFileSaveData(bool)),
    this->Implementation->UI.actionFileSaveData,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveScreenshot,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onFileSaveScreenshot()));
  connect(
    this->Implementation->Core,
    SIGNAL(enableFileSaveScreenshot(bool)),
    this->Implementation->UI.actionFileSaveScreenshot,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveAnimation,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onFileSaveAnimation()));
  connect(
    this->Implementation->Core,
    SIGNAL(enableFileSaveAnimation(bool)),
    this->Implementation->UI.actionFileSaveAnimation,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveGeometry, SIGNAL(triggered()),
    this->Implementation->Core, SLOT(onSaveGeometry()));
  connect(this->Implementation->Core,
    SIGNAL(enableFileSaveGeometry(bool)),
    this->Implementation->UI.actionFileSaveGeometry,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileExit, SIGNAL(triggered()),
    pqApplicationCore::instance(), SLOT(quit()));

  pqUndoStack* undoStack = this->Implementation->Core->getApplicationUndoStack();

  connect(this->Implementation->UI.actionEditUndo,
    SIGNAL(triggered()), undoStack, SLOT(undo()));
  connect(undoStack,
    SIGNAL(canUndoChanged(bool)),
    this->Implementation->UI.actionEditUndo, SLOT(setEnabled(bool)));
  connect(undoStack,
    SIGNAL(undoLabelChanged(const QString&)), this, SLOT(onUndoLabel(const QString&)));

  connect(this->Implementation->UI.actionEditViewSettings,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onEditViewSettings()));

  connect(this->Implementation->UI.actionEditSettings,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onEditSettings()));

  connect(this->Implementation->UI.actionEditRedo,
    SIGNAL(triggered()), undoStack, SLOT(redo()));
  connect(undoStack,
    SIGNAL(canRedoChanged(bool)), this->Implementation->UI.actionEditRedo, SLOT(setEnabled(bool)));
  connect(undoStack,
    SIGNAL(redoLabelChanged(const QString&)), this, SLOT(onRedoLabel(const QString&)));

  connect(this->Implementation->UI.actionEditCameraUndo,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onEditCameraUndo()));
  connect(this->Implementation->Core,
    SIGNAL(enableCameraUndo(bool)),
    this->Implementation->UI.actionEditCameraUndo,
    SLOT(setEnabled(bool)));
  connect(this->Implementation->Core,
    SIGNAL(cameraUndoLabel(const QString&)),
    this,
    SLOT(onCameraUndoLabel(const QString&)));

  connect(this->Implementation->UI.actionEditCameraRedo,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onEditCameraRedo()));
  connect(this->Implementation->Core,
    SIGNAL(enableCameraRedo(bool)),
    this->Implementation->UI.actionEditCameraRedo,
    SLOT(setEnabled(bool)));
  connect(this->Implementation->Core,
    SIGNAL(cameraRedoLabel(const QString&)),
    this,
    SLOT(onCameraRedoLabel(const QString&)));

  connect(this->Implementation->UI.actionServerConnect,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onServerConnect()));
  connect(
    this->Implementation->Core,
    SIGNAL(enableServerConnect(bool)),
    this->Implementation->UI.actionServerConnect,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionServerDisconnect,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onServerDisconnect()));
  connect(
    this->Implementation->Core,
    SIGNAL(enableServerDisconnect(bool)),
    this->Implementation->UI.actionServerDisconnect,
    SLOT(setEnabled(bool)));

  this->Implementation->Core->setSourceMenu(
    this->Implementation->UI.menuSources);
  connect(
    this->Implementation->Core,
    SIGNAL(enableSourceCreate(bool)),
    this->Implementation->UI.menuSources,
    SLOT(setEnabled(bool)));

  this->Implementation->Core->setFilterMenu(
    this->Implementation->UI.menuFilters);
  connect(
    this->Implementation->Core,
    SIGNAL(enableFilterCreate(bool)),
    this->Implementation->UI.menuFilters,
    SLOT(setEnabled(bool)));

  //this->Implementation->Core->pipelineMenu().setMenuAction(
  //pqPipelineMenu::AddSourceAction, this->Implementation->UI.actionAddSource);
  //this->Implementation->Core->pipelineMenu().setMenuAction(
  //pqPipelineMenu::AddFilterAction, this->Implementation->UI.actionAddFilter);
  this->Implementation->Core->pipelineMenu().setMenuAction(
    pqPipelineMenu::ChangeInputAction, this->Implementation->UI.actionChangeInput);
  this->Implementation->Core->pipelineMenu().setMenuAction(
    pqPipelineMenu::DeleteAction, this->Implementation->UI.actionDelete);
  this->Implementation->Core->pipelineMenu().setMenuAction(
    pqPipelineMenu::IgnoreTimeAction, this->Implementation->UI.actionIgnoreTime);
  connect(this->Implementation->UI.actionDelete_All, SIGNAL(triggered()),
          this, SLOT(onDeleteAll()));

  connect(this->Implementation->UI.actionIgnoreTime, SIGNAL(triggered(bool)),
    this->Implementation->Core,
    SLOT(ignoreTimesFromSelectedSources(bool)));

  connect(this->Implementation->UI.actionToolsCreateLookmark,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onToolsCreateLookmark()));

  connect(this->Implementation->UI.actionToolsCreateCustomFilter,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onToolsCreateCustomFilter()));

  connect(this->Implementation->UI.actionToolsManageCustomFilters,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onToolsManageCustomFilters()));

  connect(this->Implementation->UI.actionToolsManageLinks,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onToolsManageLinks()));

  connect(this->Implementation->UI.actionToolsAddCameraLink,
    SIGNAL(triggered()), this, SLOT(onAddCameraLink()));

  connect(this->Implementation->UI.actionToolsDumpWidgetNames,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onToolsDumpWidgetNames()));

  connect(this->Implementation->UI.actionToolsRecordTest,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onToolsRecordTest()));

  connect(this->Implementation->UI.actionToolsRecordTestScreenshot,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onToolsRecordTestScreenshot()));

  connect(this->Implementation->UI.actionToolsPlayTest,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onToolsPlayTest()));

  connect(this->Implementation->UI.actionToolsTimerLog, SIGNAL(triggered()),
          this->Implementation->Core, SLOT(onToolsTimerLog()));

  connect(this->Implementation->UI.actionToolsOutputWindow, SIGNAL(triggered()),
          this->Implementation->Core, SLOT(onToolsOutputWindow()));

  connect(this->Implementation->UI.actionToolsPythonShell,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onToolsPythonShell()));

  connect(this->Implementation->UI.actionHelpAbout,
    SIGNAL(triggered()), this, SLOT(onHelpAbout()));

  connect(this->Implementation->UI.actionHelpHelp,
    SIGNAL(triggered()), this, SLOT(onHelpHelp()));

  connect(this->Implementation->UI.actionHelpEnableTooltips,
    SIGNAL(triggered(bool)),
    this->Implementation->Core, SLOT(onHelpEnableTooltips(bool)));
  connect(this->Implementation->Core, SIGNAL(enableTooltips(bool)),
    this->Implementation->UI.actionHelpEnableTooltips,
    SLOT(setChecked(bool)));

  connect(this->Implementation->UI.actionVCRPlay, SIGNAL(triggered()),
    &this->Implementation->Core->VCRController(), SLOT(onPlay()));

  connect(this->Implementation->UI.actionVCRFirstFrame,
    SIGNAL(triggered()), &this->Implementation->Core->VCRController(), SLOT(onFirstFrame()));

  connect(this->Implementation->UI.actionVCRPreviousFrame,
    SIGNAL(triggered()), &this->Implementation->Core->VCRController(), SLOT(onPreviousFrame()));

  connect(this->Implementation->UI.actionVCRNextFrame,
    SIGNAL(triggered()), &this->Implementation->Core->VCRController(), SLOT(onNextFrame()));

  connect(this->Implementation->UI.actionVCRLastFrame,
    SIGNAL(triggered()), &this->Implementation->Core->VCRController(), SLOT(onLastFrame()));

  connect(this->Implementation->UI.actionVCRLoop, SIGNAL(toggled(bool)),
    &this->Implementation->Core->VCRController(), SLOT(onLoop(bool)));

  pqVCRController* vcrcontroller = &this->Implementation->Core->VCRController();
  connect(vcrcontroller, SIGNAL(enabled(bool)),
    this->Implementation->UI.actionVCRPlay, SLOT(setEnabled(bool)));
  connect(vcrcontroller, SIGNAL(enabled(bool)),
    this->Implementation->UI.actionVCRFirstFrame, SLOT(setEnabled(bool)));
  connect(vcrcontroller, SIGNAL(enabled(bool)),
    this->Implementation->UI.actionVCRPreviousFrame, SLOT(setEnabled(bool)));
  connect(vcrcontroller, SIGNAL(enabled(bool)),
    this->Implementation->UI.actionVCRNextFrame, SLOT(setEnabled(bool)));
  connect(vcrcontroller, SIGNAL(enabled(bool)),
    this->Implementation->UI.actionVCRLastFrame, SLOT(setEnabled(bool)));
  connect(vcrcontroller, SIGNAL(enabled(bool)),
    this->Implementation->UI.actionVCRLoop, SLOT(setEnabled(bool)));
  connect(vcrcontroller, SIGNAL(timeRanges(double, double)),
    this, SLOT(setTimeRanges(double, double)));
  connect(vcrcontroller, SIGNAL(loop(bool)),
    this->Implementation->UI.actionVCRLoop, SLOT(setChecked(bool)));
  connect(vcrcontroller, SIGNAL(playing(bool)),
    this, SLOT(onPlaying(bool)));

  pqProgressManager* progress_manager =
    pqApplicationCore::instance()->getProgressManager();
  progress_manager->addNonBlockableObject(
    this->Implementation->UI.VCRToolbar);

  QShortcut *ctrlSpace = new QShortcut(Qt::CTRL + Qt::Key_Space,
    &this->Implementation->Core->multiViewManager());
  QObject::connect(ctrlSpace, SIGNAL(activated()),
    this, SLOT(onQuickLaunchShortcut()));

  QShortcut *altSpace = new QShortcut(Qt::ALT + Qt::Key_Space,
    &this->Implementation->Core->multiViewManager());
  QObject::connect(altSpace, SIGNAL(activated()),
    this, SLOT(onQuickLaunchShortcut()));

  connect(
    this->Implementation->Core, SIGNAL(enableFileSaveScreenshot(bool)),
    this->Implementation->UI.actionResetCamera, SLOT(setEnabled(bool)));
  connect(
    this->Implementation->Core, SIGNAL(enableFileSaveScreenshot(bool)),
    this->Implementation->UI.actionPositiveX, SLOT(setEnabled(bool)));
  connect(
    this->Implementation->Core, SIGNAL(enableFileSaveScreenshot(bool)),
    this->Implementation->UI.actionNegativeX, SLOT(setEnabled(bool)));
  connect(
    this->Implementation->Core, SIGNAL(enableFileSaveScreenshot(bool)),
    this->Implementation->UI.actionPositiveY, SLOT(setEnabled(bool)));
  connect(
    this->Implementation->Core, SIGNAL(enableFileSaveScreenshot(bool)),
    this->Implementation->UI.actionNegativeY, SLOT(setEnabled(bool)));
  connect(
    this->Implementation->Core, SIGNAL(enableFileSaveScreenshot(bool)),
    this->Implementation->UI.actionPositiveZ, SLOT(setEnabled(bool)));
  connect(
    this->Implementation->Core, SIGNAL(enableFileSaveScreenshot(bool)),
    this->Implementation->UI.actionNegativeZ, SLOT(setEnabled(bool)));

  connect(
    this->Implementation->UI.actionResetCamera, SIGNAL(triggered()),
    this->Implementation->Core, SLOT(resetCamera()));
  connect(
    this->Implementation->UI.actionPositiveX, SIGNAL(triggered()),
    this->Implementation->Core, SLOT(resetViewDirectionPosX()));
  connect(
    this->Implementation->UI.actionNegativeX, SIGNAL(triggered()),
    this->Implementation->Core, SLOT(resetViewDirectionNegX()));
  connect(
    this->Implementation->UI.actionPositiveY, SIGNAL(triggered()),
    this->Implementation->Core, SLOT(resetViewDirectionPosY()));
  connect(
    this->Implementation->UI.actionNegativeY, SIGNAL(triggered()),
    this->Implementation->Core, SLOT(resetViewDirectionNegY()));
  connect(
    this->Implementation->UI.actionPositiveZ, SIGNAL(triggered()),
    this->Implementation->Core, SLOT(resetViewDirectionPosZ()));
  connect(
    this->Implementation->UI.actionNegativeZ, SIGNAL(triggered()),
    this->Implementation->Core, SLOT(resetViewDirectionNegZ()));

  connect(
    this->Implementation->UI.actionFullScreen, SIGNAL(triggered()),
    &this->Implementation->Core->multiViewManager(),
    SLOT(toggleFullScreen()));


  // Setup the 'modes' so that they are exclusively selected
  QActionGroup *modeGroup = new QActionGroup(this);
  modeGroup->addAction(this->Implementation->UI.actionMoveMode);
  modeGroup->addAction(this->Implementation->UI.actionSelectionMode);
  modeGroup->addAction(this->Implementation->UI.actionSelectSurfacePoints);
 // modeGroup->addAction(this->Implementation->UI.actionSelect_Thresholds);
  modeGroup->addAction(this->Implementation->UI.actionSelect_Frustum);
  modeGroup->addAction(this->Implementation->UI.actionSelectFrustumPoints);
  modeGroup->addAction(this->Implementation->UI.actionSelect_Block);

  this->Implementation->Core->setupVariableToolbar(
    this->Implementation->UI.variableToolbar);
  foreach(QAction* a, this->Implementation->UI.variableToolbar->actions())
    {
    if (a == this->Implementation->UI.actionScalarBarVisibility)
      {
      // actionScalarBarVisibility's enabled state is managed by
      // pqScalarBarVisibilityAdaptor.
      continue;
      }
    connect(
      this->Implementation->Core,
      SIGNAL(enableVariableToolbar(bool)),
      a, SLOT(setEnabled(bool)));
    }

  this->Implementation->Core->setupRepresentationToolbar(
    this->Implementation->UI.representationToolbar);
  foreach(QAction* a, this->Implementation->UI.representationToolbar->actions())
    {
    connect(
      this->Implementation->Core,
      SIGNAL(enableVariableToolbar(bool)),
      a, SLOT(setEnabled(bool)));
    }

  this->Implementation->Core->setupCommonFiltersToolbar(
    this->Implementation->UI.commonFilters);

  this->Implementation->Core->setupLookmarkToolbar(
    this->Implementation->UI.lookmarkToolbar);

  // Setup dockable windows ...
  this->Implementation->Core->setupPipelineBrowser(
    this->Implementation->UI.pipelineBrowserDock);
  pqPipelineBrowser *browser = this->Implementation->Core->pipelineBrowser();
  this->Implementation->Core->pipelineMenu().setModels(browser->getModel(),
    browser->getSelectionModel());
  //connect(this->Implementation->UI.actionAddSource, SIGNAL(triggered()),
  //browser, SLOT(addSource()));
  //connect(this->Implementation->UI.actionAddFilter, SIGNAL(triggered()),
  //browser, SLOT(addFilter()));
  connect(this->Implementation->UI.actionChangeInput, SIGNAL(triggered()),
    browser, SLOT(changeInput()));
  connect(this->Implementation->UI.actionDelete, SIGNAL(triggered()),
    browser, SLOT(deleteSelected()));
  pqPipelineBrowserContextMenu *browserMenu =
    new pqPipelineBrowserContextMenu(browser);
  browserMenu->setMenuAction(
    pqPipelineBrowserContextMenu::OPEN,
    this->Implementation->UI.actionFileOpen);
  //browserMenu->setMenuAction(this->Implementation->UI.actionAddSource);
  //browserMenu->setMenuAction(this->Implementation->UI.actionAddFilter);
  browserMenu->setMenuAction(
    pqPipelineBrowserContextMenu::CHANGE_INPUT,
    this->Implementation->UI.actionChangeInput);
  browserMenu->setMenuAction(
    pqPipelineBrowserContextMenu::DELETE,
    this->Implementation->UI.actionDelete);
  browserMenu->setMenuAction(
    pqPipelineBrowserContextMenu::CREATE_CUSTOM_FILTER,
    this->Implementation->UI.actionToolsCreateCustomFilter);
  browserMenu->setMenuAction(
    pqPipelineBrowserContextMenu::IGNORE_TIME,
    this->Implementation->UI.actionIgnoreTime);

  pqProxyTabWidget* const proxyTab =
    this->Implementation->Core->setupProxyTabWidget(
      this->Implementation->UI.objectInspectorDock);

  QObject::connect(
    proxyTab->getObjectInspector(),
    SIGNAL(helpRequested(QString)),
    this,
    SLOT(showHelpForProxy(QString)));

  QObject::connect(
    proxyTab->getObjectInspector(),
    SIGNAL(preaccept()),
    this,
    SLOT(onPreAccept()));

  QObject::connect(
    proxyTab->getObjectInspector(),
    SIGNAL(postaccept()),
    this,
    SLOT(onPostAccept()));

  QObject::connect(
    this->Implementation->UI.actionTesting_Window_Size,
    SIGNAL(toggled(bool)),
    this->Implementation->Core,
    SLOT(enableTestingRenderWindowSize(bool)));

  this->Implementation->Core->setupStatisticsView(
    this->Implementation->UI.statisticsViewDock);


  this->Implementation->Core->setupSelectionInspector(
    this->Implementation->UI.selectionInspectorDock);

  this->Implementation->Core->setupLookmarkBrowser(
    this->Implementation->UI.lookmarkBrowserDock);

  this->Implementation->Core->setupLookmarkInspector(
    this->Implementation->UI.lookmarkInspectorDock);

  QObject::connect(
    this->Implementation->Core->getAnimationManager(),
    SIGNAL(activeSceneChanged(pqAnimationScene*)),
    this->Implementation->UI.currentTimeToolbar,
    SLOT(setAnimationScene(pqAnimationScene*)));

  pqComparativeVisPanel* cv_panel =
    new pqComparativeVisPanel(
    this->Implementation->UI.comparativePanelDock);
  this->Implementation->UI.comparativePanelDock->setWidget(cv_panel);

  this->Implementation->Core->setupAnimationView(
    this->Implementation->UI.animationViewDock);

  // Setup the view menu ...
  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.variableToolbar,
    this->Implementation->UI.variableToolbar->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.cameraToolbar,
    this->Implementation->UI.cameraToolbar->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.axesToolbar,
    this->Implementation->UI.axesToolbar->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.commonFilters,
    this->Implementation->UI.commonFilters->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.currentTimeToolbar,
    this->Implementation->UI.currentTimeToolbar->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.lookmarkToolbar,
    this->Implementation->UI.lookmarkToolbar->windowTitle());

  // The macro toolbar defaults to hidden.
  this->Implementation->UI.macroToolbar->setVisible(0);
#ifdef PARAVIEW_ENABLE_PYTHON
  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.macroToolbar,
    this->Implementation->UI.macroToolbar->windowTitle());
#endif // PARAVIEW_ENABLE_PYTHON

  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.mainToolBar,
    this->Implementation->UI.mainToolBar->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.representationToolbar,
    this->Implementation->UI.representationToolbar->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.selectionToolbar,
    this->Implementation->UI.selectionToolbar->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.undoRedoToolbar,
    this->Implementation->UI.undoRedoToolbar->windowTitle());

  this->Implementation->ToolbarsMenu->addWidget(
    this->Implementation->UI.VCRToolbar,
    this->Implementation->UI.VCRToolbar->windowTitle());

  //this->Implementation->ToolbarsMenu->addWidget(
  //  this->Implementation->UI.customFilterToolbar,
  //  this->Implementation->UI.customFilterToolbar->windowTitle());


  this->Implementation->ViewMenu->addWidget(
    this->Implementation->UI.animationViewDock,
    this->Implementation->UI.animationViewDock->windowTitle());

  this->Implementation->ViewMenu->addWidget(
    this->Implementation->UI.comparativePanelDock,
    this->Implementation->UI.comparativePanelDock->windowTitle());

  this->Implementation->ViewMenu->addWidget(
    this->Implementation->UI.selectionInspectorDock,
    this->Implementation->UI.selectionInspectorDock->windowTitle());

  this->Implementation->ViewMenu->addWidget(
    this->Implementation->UI.lookmarkBrowserDock,
    this->Implementation->UI.lookmarkBrowserDock->windowTitle());

  this->Implementation->ViewMenu->addWidget(
    this->Implementation->UI.lookmarkInspectorDock,
    this->Implementation->UI.lookmarkInspectorDock->windowTitle());

  this->Implementation->ViewMenu->addWidget(
    this->Implementation->UI.objectInspectorDock,
    this->Implementation->UI.objectInspectorDock->windowTitle());

  this->Implementation->ViewMenu->addWidget(
    this->Implementation->UI.pipelineBrowserDock,
    this->Implementation->UI.pipelineBrowserDock->windowTitle());

  this->Implementation->ViewMenu->addWidget(
    this->Implementation->UI.statisticsViewDock,
    this->Implementation->UI.statisticsViewDock->windowTitle());

  // Setup the multiview render window ...
  this->setCentralWidget(&this->Implementation->Core->multiViewManager());

  // Setup the statusbar ...
  this->Implementation->Core->setupProgressBar(this->statusBar());

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Setup the default dock configuration ...
  this->Implementation->UI.lookmarkBrowserDock->hide();
  this->Implementation->UI.lookmarkInspectorDock->hide();
  this->Implementation->UI.statisticsViewDock->hide();
  this->Implementation->UI.comparativePanelDock->hide();
  this->Implementation->UI.animationViewDock->hide();
  this->Implementation->UI.selectionInspectorDock->hide();

  // Fix the toolbar layouts from designer.
  this->Implementation->UI.mainToolBar->layout()->setSpacing(0);
  this->Implementation->UI.selectionToolbar->layout()->setSpacing(0);
  this->Implementation->UI.variableToolbar->layout()->setSpacing(0);
  this->Implementation->UI.representationToolbar->layout()->setSpacing(0);
  //this->Implementation->UI.customFilterToolbar->layout()->setSpacing(0);
  this->Implementation->UI.lookmarkToolbar->layout()->setSpacing(0);
  this->Implementation->UI.undoRedoToolbar->layout()->setSpacing(0);
  this->Implementation->UI.VCRToolbar->layout()->setSpacing(0);
  this->Implementation->UI.cameraToolbar->layout()->setSpacing(0);

  // Now that we're ready, initialize everything ...
  this->Implementation->Core->initializeStates();

  this->Implementation->UI.actionEditUndo->setEnabled(
    undoStack->canUndo());
  this->Implementation->UI.actionEditRedo->setEnabled(
    undoStack->canRedo());
  this->onUndoLabel(undoStack->undoLabel());
  this->onRedoLabel(undoStack->redoLabel());

  // Set up scalar bar visibility tool bar item.
  pqScalarBarVisibilityAdaptor* sbva = new pqScalarBarVisibilityAdaptor(
      this->Implementation->UI.actionScalarBarVisibility);
  QObject::connect(this->Implementation->Core->getObjectInspectorDriver(),
    SIGNAL(representationChanged(pqDataRepresentation*, pqView*)),
    sbva, SLOT(setActiveRepresentation(pqDataRepresentation*)));

  // Set up the color map tool buttons.
  pqColorScaleToolbar *colorScale =
      this->Implementation->Core->getColorScaleEditorManager();
  colorScale->setColorAction(this->Implementation->UI.actionEditColorMap);
  colorScale->setRescaleAction(this->Implementation->UI.actionResetRange);

  // Set up Center Axes toolbar.
  QObject::connect(
    this->Implementation->UI.actionShowOrientationAxes, SIGNAL(toggled(bool)),
    this->Implementation->Core, SLOT(setOrientationAxesVisibility(bool)));
  QObject::connect(
    this->Implementation->UI.actionShowCenterAxes, SIGNAL(toggled(bool)),
    this->Implementation->Core, SLOT(setCenterAxesVisibility(bool)));
  QObject::connect(
    this->Implementation->UI.actionResetCenter, SIGNAL(triggered()),
    this->Implementation->Core,
    SLOT(resetCenterOfRotationToCenterOfCurrentData()));
  QObject::connect(
    this->Implementation->UI.actionPickCenter, SIGNAL(toggled(bool)),
    this->Implementation->Core,
    SLOT(pickCenterOfRotation(bool)));

  QObject::connect(
    this->Implementation->Core, SIGNAL(enableShowCenterAxis(bool)),
    this, SLOT(onShowCenterAxisChanged(bool)), Qt::QueuedConnection);
  QObject::connect(
    this->Implementation->Core, SIGNAL(enableResetCenter(bool)),
    this->Implementation->UI.actionResetCenter, SLOT(setEnabled(bool)));
  QObject::connect(
    this->Implementation->Core, SIGNAL(enablePickCenter(bool)),
    this->Implementation->UI.actionPickCenter, SLOT(setEnabled(bool)));
  QObject::connect(
    this->Implementation->Core, SIGNAL(pickingCenter(bool)),
    this->Implementation->UI.actionPickCenter, SLOT(setChecked(bool)));
  QObject::connect(
    this->Implementation->Core->renderViewSelectionHelper(),
    SIGNAL(enableZoom(bool)),
    this->Implementation->UI.actionZoomToBox, SLOT(setEnabled(bool)));
  QObject::connect(
    this->Implementation->UI.actionZoomToBox, SIGNAL(triggered()),
    this->Implementation->Core->renderViewSelectionHelper(), SLOT(beginZoom()));

  connect(this->Implementation->UI.actionManage_Plugins,
    SIGNAL(triggered()), this->Implementation->Core, SLOT(onManagePlugins()));


  // Set up selection buttons.
  QObject::connect(
    this->Implementation->UI.actionMoveMode, SIGNAL(triggered()),
    this->Implementation->Core->renderViewSelectionHelper(), SLOT(endSelection()));

  // 3d Selection Modes
  QObject::connect(
    this->Implementation->Core->renderViewSelectionHelper(),
    SIGNAL(enableSurfaceSelection(bool)),
    this->Implementation->UI.actionSelectionMode, SLOT(setEnabled(bool)));
  QObject::connect(
    this->Implementation->Core->renderViewSelectionHelper(),
    SIGNAL(enableSurfacePointsSelection(bool)),
    this->Implementation->UI.actionSelectSurfacePoints, SLOT(setEnabled(bool)));
  QObject::connect(
    this->Implementation->Core->renderViewSelectionHelper(),
    SIGNAL(enableFrustumSelection(bool)),
    this->Implementation->UI.actionSelect_Frustum, SLOT(setEnabled(bool)));
  QObject::connect(
    this->Implementation->Core->renderViewSelectionHelper(),
    SIGNAL(enableFrustumPointSelection(bool)),
    this->Implementation->UI.actionSelectFrustumPoints, SLOT(setEnabled(bool)));
  QObject::connect(
    this->Implementation->Core->renderViewSelectionHelper(),
    SIGNAL(enableBlockSelection(bool)),
    this->Implementation->UI.actionSelect_Block, SLOT(setEnabled(bool)));


  QObject::connect(
    this->Implementation->UI.actionSelectionMode, SIGNAL(triggered()),
    this->Implementation->Core->renderViewSelectionHelper(), SLOT(beginSurfaceSelection()));
  QObject::connect(
    this->Implementation->UI.actionSelectSurfacePoints, SIGNAL(triggered()),
    this->Implementation->Core->renderViewSelectionHelper(), SLOT(beginSurfacePointsSelection()));
  QObject::connect(
    this->Implementation->UI.actionSelect_Frustum, SIGNAL(triggered()),
    this->Implementation->Core->renderViewSelectionHelper(), SLOT(beginFrustumSelection()));
  QObject::connect(
    this->Implementation->UI.actionSelectFrustumPoints, SIGNAL(triggered()),
    this->Implementation->Core->renderViewSelectionHelper(), SLOT(beginFrustumPointsSelection()));
  QObject::connect(
    this->Implementation->UI.actionSelect_Block, SIGNAL(triggered()),
    this->Implementation->Core->renderViewSelectionHelper(),
    SLOT(beginBlockSelection()));

  QObject::connect(
    this->Implementation->Core->renderViewSelectionHelper(),
    SIGNAL(selectionModeChanged(int)),
    this, SLOT(onSelectionModeChanged(int)));
  QObject::connect(
    this->Implementation->Core->renderViewSelectionHelper(),
    SIGNAL(interactionModeChanged(bool)),
    this->Implementation->UI.actionMoveMode, SLOT(setChecked(bool)));

  // When a selection is marked, we revert to interaction mode.
  QObject::connect(
    this->Implementation->Core->renderViewSelectionHelper(),
    SIGNAL(selectionFinished(int, int, int, int)),
    this->Implementation->Core->renderViewSelectionHelper(), SLOT(endSelection()));

  QObject::connect(
    this->Implementation->UI.actionExport, SIGNAL(triggered()),
    this->Implementation->Core, SLOT(onFileExport()));
  QObject::connect(
    this->Implementation->Core, SIGNAL(enableExport(bool)),
    this->Implementation->UI.actionExport, SLOT(setEnabled(bool)));

  // QOpenFileEvent handling (Michael Wild <themiwi@users.sourceforge.net>)
  QObject::connect(
      &this->Implementation->Core->multiViewManager(), SIGNAL(triggerFileOpen(const QStringList&)),
      this->Implementation->Core, SLOT(onFileOpen(const QStringList&)));

  // The Macros menu defaults to hidden
  this->Implementation->UI.menuMacros->menuAction()->setVisible(false);

#ifdef PARAVIEW_ENABLE_PYTHON
  // Give the macro toolbar and macro menu to the pqPythonMacroSupervisor
  pqPythonManager* manager = qobject_cast<pqPythonManager*>(
    pqApplicationCore::instance()->manager("PYTHON_MANAGER"));
  if (manager)
    {
    manager->addWidgetForMacros(this->Implementation->UI.menuMacros);
    manager->addWidgetForMacros(this->Implementation->UI.macroToolbar);
    }

  // Connect the onSettingsModified slot, then call it once to initialize
  connect(pqApplicationCore::instance()->settings(), SIGNAL(modified()),
    this, SLOT(onSettingsModified()));
  this->onSettingsModified();
#endif // PARAVIEW_ENABLE_PYTHON


  // Restore the state of the window ...
  pqApplicationCore::instance()->settings()->restoreState("pqClientMainWindow", *this);

  this->Implementation->UI.actionScalarBarVisibility->setEnabled(false);
}

pqClientMainWindow::~pqClientMainWindow()
{
  this->Implementation->Core->removePluginToolBars();

  // Save the state of the window ...
  pqApplicationCore::instance()->settings()->saveState(*this, "pqClientMainWindow");

  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onSettingsModified()
{
  // Hide or show the Macros menu
  bool showMacroMenu = pqApplicationCore::instance()->settings()->value(
    "PythonMacros/ShowMenu", false).toBool();
  this->Implementation->UI.menuMacros->menuAction()->setVisible(showMacroMenu);
}

//-----------------------------------------------------------------------------
QString pqClientMainWindow::getTestDirectory()
{
  return pqCoreTestUtility::TestDirectory();
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onShowCenterAxisChanged(bool enabled)
{
  pqRenderView* renView = qobject_cast<pqRenderView*>(
    pqActiveView::instance().current());

  this->Implementation->UI.actionShowOrientationAxes->setEnabled(enabled);
  this->Implementation->UI.actionShowOrientationAxes->blockSignals(true);
  this->Implementation->UI.actionShowOrientationAxes->setChecked(
    renView? renView->getOrientationAxesVisibility() : false);
  this->Implementation->UI.actionShowOrientationAxes->blockSignals(false);

  this->Implementation->UI.actionShowCenterAxes->setEnabled(enabled);
  this->Implementation->UI.actionShowCenterAxes->blockSignals(true);
  this->Implementation->UI.actionShowCenterAxes->setChecked(
    renView ? renView->getCenterAxesVisibility() : false);
  this->Implementation->UI.actionShowCenterAxes->blockSignals(false);
}

//-----------------------------------------------------------------------------
bool pqClientMainWindow::compareView(const QString& ReferenceImage,
  double Threshold,
  ostream& Output,
  const QString& TempDirectory)
{
  return this->Implementation->Core->compareView(
    ReferenceImage, Threshold, Output, TempDirectory);
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onUndoLabel(const QString& label)
{
  this->Implementation->UI.actionEditUndo->setText(
    label.isEmpty() ? tr("Can't Undo") : QString(tr("&Undo %1")).arg(label));
  this->Implementation->UI.actionEditUndo->setStatusTip(
    label.isEmpty() ? tr("Can't Undo") : QString(tr("Undo %1")).arg(label));
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onRedoLabel(const QString& label)
{
  this->Implementation->UI.actionEditRedo->setText(
    label.isEmpty() ? tr("Can't Redo") : QString(tr("&Redo %1")).arg(label));
  this->Implementation->UI.actionEditRedo->setStatusTip(
    label.isEmpty() ? tr("Can't Redo") : QString(tr("Redo %1")).arg(label));
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onCameraUndoLabel(const QString& label)
{
  this->Implementation->UI.actionEditCameraUndo->setText(
    label.isEmpty() ? tr("Can't Undo Camera") : QString(tr("U&ndo %1")).arg(label));
  this->Implementation->UI.actionEditCameraUndo->setStatusTip(
    label.isEmpty() ? tr("Can't Undo Camera") : QString(tr("Undo %1")).arg(label));
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onCameraRedoLabel(const QString& label)
{
  this->Implementation->UI.actionEditCameraRedo->setText(
    label.isEmpty() ? tr("Can't Redo Camera") : QString(tr("R&edo %1")).arg(label));
  this->Implementation->UI.actionEditCameraRedo->setStatusTip(
    label.isEmpty() ? tr("Can't Redo Camera") : QString(tr("Redo %1")).arg(label));
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::setMessage(const QString& message)
{
  this->statusBar()->showMessage(message);
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onPreAccept()
{
  this->statusBar()->showMessage(tr("Updating..."));
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onPostAccept()
{
  this->statusBar()->showMessage(tr("Ready"), 2000);
  QTimer::singleShot(0, this, SLOT(endWaitCursor()));
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::endWaitCursor()
{
  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onHelpAbout()
{
  pqClientAboutDialog* const dialog = new pqClientAboutDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::makeAssistant()
{
  if (!this->Implementation->HelpWindow)
    {
    pqHelpWindow* helpWindow = new pqHelpWindow("ParaView Online Help", this);
    QObject::connect(helpWindow, SIGNAL(helpWarnings(const QString&)),
      this, SLOT(assistantError(const QString&)));
    helpWindow->registerDocumentation(":/ParaViewResources/pqClient.qch");
    this->Implementation->HelpWindow = helpWindow;
    this->Implementation->HelpWindow->showPage(
      "qthelp://paraview.org/paraview/Documentation/index.html");
    }
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onHelpHelp()
{
  this->makeAssistant();
  
  if(this->Implementation->HelpWindow)
    {
    this->Implementation->HelpWindow->show();
    this->Implementation->HelpWindow->raise();
    }
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::assistantError(const QString& error)
{
  qCritical(error.toAscii().data());
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onSelectionModeChanged(int mode)
{
  if(this->Implementation->UI.selectionToolbar->isEnabled())
    {
    switch (mode)
      {
    case pqRubberBandHelper::SELECT://surface selection
      this->Implementation->UI.actionSelectionMode->setChecked(true);
      break;

    case pqRubberBandHelper::SELECT_POINTS: //surface selection
      this->Implementation->UI.actionSelectSurfacePoints->setChecked(true);
      break;

    case pqRubberBandHelper::FRUSTUM:
      this->Implementation->UI.actionSelect_Frustum->setChecked(true);
      break;

    case pqRubberBandHelper::FRUSTUM_POINTS:
      this->Implementation->UI.actionSelectFrustumPoints->setChecked(true);
      break;

    case pqRubberBandHelper::BLOCKS:
      this->Implementation->UI.actionSelect_Block->setChecked(true);
      break;

    case pqRubberBandHelper::INTERACT:
      this->Implementation->UI.actionMoveMode->setChecked(true);
      break;

    default:
      break;
    }
  }
  this->Implementation->UI.actionZoomToBox->setChecked(mode ==
    pqRubberBandHelper::ZOOM);
}


//-----------------------------------------------------------------------------
QVariant pqClientMainWindow::findToolBarActionsNotInMenus()
{
  QStringList missingInActions;

  // get all QActions on toolbars
  QList<QToolBar*> toolBars = this->findChildren<QToolBar*>();
  QList<QAction*> toolBarActions;
  foreach(QToolBar* tb, toolBars)
    {
    toolBarActions += tb->actions();
    }

  // get all QActions on menus (recursively)
  QList<QAction*> menuActions = this->menuBar()->actions();
  for(int i = 0; i < menuActions.size();)
    {
    QAction* a = menuActions[i];
    if(a->menu())
      {
      menuActions += a->menu()->actions();
      menuActions.removeAt(i);
      }
    else
      {
      i++;
      }
    }

  // sort actions
  qSort(toolBarActions.begin(), toolBarActions.end());
  qSort(menuActions.begin(), menuActions.end());

  // make sure all toolbar icons are in the menu
  QList<QAction*>::iterator iter = menuActions.begin();
  foreach(QAction* a, toolBarActions)
    {
    QList<QAction*>::iterator newiter;
    newiter = qBinaryFind(iter, menuActions.end(), a);
    if(newiter == menuActions.end())
      {
      missingInActions.append(pqObjectNaming::GetName(*a));
      }
    else
      {
      iter = newiter;
      }
    }

  return missingInActions.join(", ");
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onPlaying(bool playing)
{
  if(playing)
    {
    disconnect(this->Implementation->UI.actionVCRPlay, SIGNAL(triggered()),
      &this->Implementation->Core->VCRController(), SLOT(onPlay()));
    connect(this->Implementation->UI.actionVCRPlay, SIGNAL(triggered()),
      &this->Implementation->Core->VCRController(), SLOT(onPause()));
    this->Implementation->UI.actionVCRPlay->setIcon(
      QIcon(":/pqWidgets/Icons/pqVcrPause24.png"));
    this->Implementation->UI.actionVCRPlay->setText("Pa&use");
    }
  else
    {
    connect(this->Implementation->UI.actionVCRPlay, SIGNAL(triggered()),
      &this->Implementation->Core->VCRController(), SLOT(onPlay()));
    disconnect(this->Implementation->UI.actionVCRPlay, SIGNAL(triggered()),
      &this->Implementation->Core->VCRController(), SLOT(onPause()));
    this->Implementation->UI.actionVCRPlay->setIcon(
      QIcon(":/pqWidgets/Icons/pqVcrPlay24.png"));
    this->Implementation->UI.actionVCRPlay->setText("&Play");
    }

  this->Implementation->Core->setSelectiveEnabledState(!playing);

}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onAddCameraLink()
{
  pqView* vm = pqActiveView::instance().current();
  pqRenderView* rm = qobject_cast<pqRenderView*>(vm);
  if(rm)
    {
    rm->linkToOtherView();
    }
  else
    {
    QMessageBox::information(this, "Add Camera Link",
                             "No render module is active");
    }
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onDeleteAll()
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  this->Implementation->Core->getApplicationUndoStack()->
    beginUndoSet("Delete All");
  builder->destroyPipelineProxies();
  this->Implementation->Core->getApplicationUndoStack()->endUndoSet();
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::setTimeRanges(double start, double end)
{
  this->Implementation->UI.actionVCRFirstFrame->setToolTip(
    QString("First Frame (%1)").arg(start, 0, 'g'));
  this->Implementation->UI.actionVCRLastFrame->setToolTip(
    QString("Last Frame (%1)").arg(end, 0, 'g'));
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::onQuickLaunchShortcut()
{
  this->Implementation->Core->quickLaunch();
}

//-----------------------------------------------------------------------------
void pqClientMainWindow::showHelpForProxy(const QString& proxy)
{
  // make sure assistant is ready
  this->makeAssistant();

  if (this->Implementation->HelpWindow)
    {
    QString page = QString(
      "qthelp://paraview.org/paraview/Documentation/%1.html").arg(proxy);
    this->Implementation->HelpWindow->showPage(page);
    this->Implementation->HelpWindow->show();
    this->Implementation->HelpWindow->raise();
    }
}

void pqClientMainWindow::disableSelections()
{
  this->Implementation->UI.actionSelectionMode->setDisabled(true);
  this->Implementation->UI.actionSelectionMode->setVisible(false);
  this->Implementation->UI.actionSelectSurfacePoints->setDisabled(true);
  this->Implementation->UI.actionSelectSurfacePoints->setVisible(false);
  this->Implementation->UI.actionSelect_Block->setDisabled(true);
  this->Implementation->UI.actionSelect_Block->setVisible(false);

  this->Implementation->UI.actionSelect_Frustum->setDisabled(true);
  this->Implementation->UI.actionSelect_Frustum->setVisible(false);
  this->Implementation->UI.actionSelectFrustumPoints->setDisabled(true);
  this->Implementation->UI.actionSelectFrustumPoints->setVisible(false);

  this->Implementation->UI.selectionToolbar->setVisible(false);
}
