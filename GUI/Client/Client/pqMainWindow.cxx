/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqAboutDialog.h"
#include "pqConfig.h"
#include "pqConnect.h"
#include "pqFileDialog.h"
#include "pqObjectLineChartWidget.h"
#include "pqObjectHistogramWidget.h"
#include "pqLocalFileDialogModel.h"
#include "pqMainWindow.h"
#include "pqMultiViewManager.h"
#include "pqMultiViewFrame.h"
#include "pqObjectInspector.h"
#include "pqObjectInspectorWidget.h"
#include "pqParts.h"
#include "pqPipelineData.h"
#include "pqPipelineListModel.h"
#include "pqPipelineListWidget.h"
#include "pqPipelineServer.h"
#include "pqPipelineWindow.h"
#include "pqPipelineObject.h"
#include "pqRenderViewProxy.h"
#include "vtkSMMultiViewRenderModuleProxy.h"
#include "pqServer.h"
#include "pqServerBrowser.h"
#include "pqServerFileDialogModel.h"
#include "pqSetData.h"
#include "pqSetName.h"
#include "pqSMAdaptor.h"
#include "pqPicking.h"
#include "pqDataSetModel.h"
#include "pqCompoundProxyWizard.h"
#include "pqXMLUtil.h"
#include "pqSMMultiView.h"
#include "pqVariableSelectorWidget.h"

#ifdef PARAQ_EMBED_PYTHON
#include "pqPythonDialog.h"
#endif // PARAQ_EMBED_PYTHON

#include <pqImageComparison.h>

#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QTreeView>
#include <QListView>
#include <QSignalMapper>
#include <QDir>

#include <vtkPVXMLElement.h>
#include <vtkPVXMLParser.h>
#include <vtkRenderWindow.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMXMLParser.h>
#include <vtkTesting.h>
#include <vtkPVGenericRenderWindowInteractor.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyle.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPVGeometryInformation.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkPVArrayInformation.h>

#include <QVTKWidget.h>
#include <vtkEventQtSlotConnect.h>

#include <pqEventPlayer.h>
#include <pqEventPlayerXML.h>
#include <pqRecordEventsDialog.h>

pqMainWindow::pqMainWindow() :
  CurrentServer(0),
  PropertyToolbar(0),
  MultiViewManager(0),
  ServerDisconnectAction(0),
  Adaptor(0),
  Pipeline(0),
  Inspector(0),
  InspectorDock(0),
  InspectorDockAction(0),
  PipelineList(0),
  PipelineDock(0),
  PipelineDockAction(0),
  HistogramDock(0),
  LineChartDock(0),
  HistogramDockAction(0),
  LineChartDockAction(0),
  ActiveView(0),
  ElementInspectorWidget(0),
  ElementInspectorDock(0),
  ElementDockAction(0),
  CompoundProxyToolBar(0),
  VariableSelectorToolBar(0),
  CurrentProxy(0)
{
  this->setObjectName("mainWindow");
  this->setWindowTitle(QByteArray("ParaQ Client") + QByteArray(" ") + QByteArray(QT_CLIENT_VERSION));

  // Set up the main ParaQ items along with the central widget.
  this->Adaptor = new pqSMAdaptor();
  this->Pipeline = new pqPipelineData(this);
  this->VTKConnector = vtkEventQtSlotConnect::New();

  this->MultiViewManager = new pqMultiViewManager(this) << pqSetName("MultiViewManager");
  //this->MultiViewManager->hide();  // workaround for flickering in Qt 4.0.1 & 4.1.0
  this->setCentralWidget(this->MultiViewManager);
  QObject::connect(this->MultiViewManager, SIGNAL(frameAdded(pqMultiViewFrame*)),
      this, SLOT(onNewQVTKWidget(pqMultiViewFrame*)));
  QObject::connect(this->MultiViewManager, SIGNAL(frameRemoved(pqMultiViewFrame*)),
      this, SLOT(onDeleteQVTKWidget(pqMultiViewFrame*)));
  //this->MultiViewManager->show();  // workaround for flickering in Qt 4.0.1 & 4.1.0

  // Listen for the pipeline's server signals.
  QObject::connect(this->Pipeline, SIGNAL(serverAdded(pqPipelineServer *)),
      this, SLOT(onAddServer(pqPipelineServer *)));
  QObject::connect(this->Pipeline, SIGNAL(removingServer(pqPipelineServer *)),
      this, SLOT(onRemoveServer(pqPipelineServer *)));
  QObject::connect(this->Pipeline, SIGNAL(windowAdded(pqPipelineWindow *)),
      this, SLOT(onAddWindow(pqPipelineWindow *)));

  // Set up the menus for the main window.
  this->menuBar() << pqSetName("menuBar");

  // File menu.
  QMenu* const fileMenu = this->menuBar()->addMenu(tr("&File"))
    << pqSetName("fileMenu");

  fileMenu->addAction(tr("&New"))
    << pqSetName("New")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileNew()));

  fileMenu->addAction(tr("&Open..."))
    << pqSetName("Open")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileOpen()));

  fileMenu->addAction(tr("&Load Server State..."))
    << pqSetName("LoadServerState")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileOpenServerState()));

  fileMenu->addAction(tr("&Save Server State..."))
    << pqSetName("SaveServerState")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileSaveServerState()));

  fileMenu->addAction(tr("Save Screenshot..."))
    << pqSetName("SaveScreenshot")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileSaveScreenshot())); 

  fileMenu->addSeparator();
  fileMenu->addAction(tr("E&xit"))
    << pqSetName("Exit")
    << pqConnect(SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));

  // View menu
  QMenu* viewMenu = this->menuBar()->addMenu(tr("&View"))
    << pqSetName("viewMenu");

  // Server menu.
  QMenu* const serverMenu = this->menuBar()->addMenu(tr("&Server"))
    << pqSetName("serverMenu");

  serverMenu->addAction(tr("Connect..."))
    << pqSetName("Connect")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onServerConnect()));

  this->ServerDisconnectAction = serverMenu->addAction(tr("Disconnect"))
    << pqSetName("Disconnect")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onServerDisconnect()));
  this->ServerDisconnectAction->setEnabled(false);

  // Sources & Filters menus.
  this->SourcesMenu = this->menuBar()->addMenu(tr("Sources"))
    << pqSetName("sourcesMenu")
    << pqConnect(SIGNAL(triggered(QAction*)), this, SLOT(onCreateSource(QAction*)));
    
  this->FiltersMenu = this->menuBar()->addMenu(tr("Filters"))
    << pqSetName("filtersMenu")
    << pqConnect(SIGNAL(triggered(QAction*)), this, SLOT(onCreateFilter(QAction*)));

  QObject::connect(this, SIGNAL(serverChanged(pqServer*)), SLOT(onUpdateSourcesFiltersMenu(pqServer*)));

  // Tools menu.
  ToolsMenu = this->menuBar()->addMenu(tr("&Tools")) << pqSetName("toolsMenu");
  QAction* compoundFilterAction = this->ToolsMenu->addAction(tr("&Compound Filters..."))
    << pqConnect(SIGNAL(triggered(bool)), this, SLOT(onOpenCompoundFilterWizard()))
    << pqSetName("CompoundFilterAction");
  compoundFilterAction->setEnabled(false);

  this->ToolsMenu->addAction(tr("&Link Editor..."))
    << pqConnect(SIGNAL(triggered(bool)), this, SLOT(onOpenLinkEditor()));

  this->ToolsMenu->addSeparator();
  this->ToolsMenu->addAction(tr("&Record Test"))
    << pqSetName("Record")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onRecordTest()));

  this->ToolsMenu->addAction(tr("&Play Test"))
    << pqSetName("Play")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onPlayTest()));

#ifdef PARAQ_EMBED_PYTHON

  this->ToolsMenu->addAction(tr("Python &Shell"))
    << pqSetName("PythonShell")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onPythonShell()));

#endif // PARAQ_EMBED_PYTHON

  // Help menu.
  QMenu* const helpMenu = this->menuBar()->addMenu(tr("&Help"))
    << pqSetName("helpMenu");

  helpMenu->addAction(QString(tr("&About %1 %2")).arg("ParaQ").arg(QT_CLIENT_VERSION))
    << pqSetName("About")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onHelpAbout()));

  // Set up the dock window corners to give the vertical docks
  // more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Add the pipeline list dock window.
  this->PipelineDock = new QDockWidget("Pipeline Inspector", this);
  if(this->PipelineDock)
    {
    this->PipelineDock->setObjectName("PipelineDock");
    this->PipelineDock->setAllowedAreas(
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->PipelineDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    // Make sure the pipeline data instance is created before the
    // pipeline list model. This ensures that the connections will
    // work.
    this->PipelineList = new pqPipelineListWidget(this->PipelineDock);
    if(this->PipelineList)
      {
      this->PipelineList->setObjectName("PipelineList");
      this->PipelineDock->setWidget(this->PipelineList);
      }

    this->addDockWidget(Qt::LeftDockWidgetArea, this->PipelineDock);
    }

  // Add the object inspector dock window.
  this->InspectorDock = new QDockWidget("Object Inspector", this);
  if(this->InspectorDock)
    {
    this->InspectorDock->setObjectName("InspectorDock");
    this->InspectorDock->setAllowedAreas(
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->InspectorDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    this->Inspector = new pqObjectInspectorWidget(this->InspectorDock);
    if(this->Inspector)
      {
      this->Inspector->setObjectName("Inspector");
      this->InspectorDock->setWidget(this->Inspector);
      if(this->PipelineList)
        {
        connect(this->PipelineList, SIGNAL(proxySelected(vtkSMProxy *)),
            this->Inspector, SLOT(setProxy(vtkSMProxy *)));
        }
      }

    this->addDockWidget(Qt::LeftDockWidgetArea, this->InspectorDock);
    }

  // Add the histogram dock window.
  this->HistogramDock = new QDockWidget("Histogram View", this);
  this->HistogramDock->setObjectName("HistogramDock");
  this->HistogramDock->setFeatures(QDockWidget::AllDockWidgetFeatures);
  this->HistogramDock->setAllowedAreas(Qt::BottomDockWidgetArea |
      Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

  pqObjectHistogramWidget* const histogram = new pqObjectHistogramWidget(this->HistogramDock);  
  this->HistogramDock->setWidget(histogram);
  if(this->PipelineList)
    {
    connect(this->PipelineList, SIGNAL(proxySelected(vtkSMProxy*)),
        histogram, SLOT(setProxy(vtkSMProxy*)));
    }

  this->addDockWidget(Qt::LeftDockWidgetArea, this->HistogramDock);

  // Add the line plot dock window.
  this->LineChartDock = new QDockWidget("Line Chart View", this);
  this->LineChartDock->setObjectName("LineChartDock");
  this->LineChartDock->setFeatures(QDockWidget::AllDockWidgetFeatures);
  this->LineChartDock->setAllowedAreas(Qt::BottomDockWidgetArea |
      Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

  pqObjectLineChartWidget* const line_chart = new pqObjectLineChartWidget(this->LineChartDock);  
  this->LineChartDock->setWidget(line_chart);
  if(this->PipelineList)
    {
    connect(this->PipelineList, SIGNAL(proxySelected(vtkSMProxy*)),
        line_chart, SLOT(setProxy(vtkSMProxy*)));
    }

  this->addDockWidget(Qt::BottomDockWidgetArea, this->LineChartDock);
  
  // Add the element inspector dock window.
  this->ElementInspectorDock = new QDockWidget("Element Inspector View", this);
  if(this->ElementInspectorDock)
    {
    this->ElementInspectorDock->setObjectName("ElementInspectorDock");
    this->ElementInspectorDock->setFeatures(QDockWidget::AllDockWidgetFeatures);
    this->ElementInspectorDock->setAllowedAreas(Qt::BottomDockWidgetArea |
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    this->ElementInspectorWidget = new QTreeView(this->ElementInspectorDock);
    if(this->ElementInspectorWidget)
      {
      this->ElementInspectorWidget->setObjectName("ElementInspectorWidget");
      this->ElementInspectorWidget->setRootIsDecorated(false);
      //this->ElementInspectorWidget->setAlternatingRowColors(true);
      this->ElementInspectorDock->setWidget(this->ElementInspectorWidget);
      }

    this->addDockWidget(Qt::BottomDockWidgetArea, this->ElementInspectorDock);
    }

  // Set up the view menu items for the dock windows.
  this->PipelineDockAction = viewMenu->addAction(
    QIcon(":pqWidgets/pqPipelineList22.png"), tr("&Pipeline Inspector"))
    << pqSetName("Pipeline");
  this->PipelineDockAction->setCheckable(true);
  this->PipelineDockAction->setChecked(true);
  this->PipelineDockAction << pqConnect(SIGNAL(triggered(bool)),
    this->PipelineDock, SLOT(setVisible(bool)));
  this->PipelineDock->installEventFilter(this);

  this->InspectorDockAction = viewMenu->addAction(tr("&Object Inspector"))
    << pqSetName("Inspector");
  this->InspectorDockAction->setCheckable(true);
  this->InspectorDockAction->setChecked(true);
  this->InspectorDockAction << pqConnect(SIGNAL(triggered(bool)),
    this->InspectorDock, SLOT(setVisible(bool)));
  this->InspectorDock->installEventFilter(this);

  this->HistogramDockAction = viewMenu->addAction(
    QIcon(":pqChart/pqHistogram22.png"), tr("&Histogram View"))
    << pqSetName("Histogram");
  this->HistogramDockAction->setCheckable(true);
  this->HistogramDockAction->setChecked(true);
  this->HistogramDockAction << pqConnect(SIGNAL(triggered(bool)),
    this->HistogramDock, SLOT(setVisible(bool)));
  this->HistogramDock->installEventFilter(this);

  this->LineChartDockAction = viewMenu->addAction(
    QIcon(":pqChart/pqLineChart22.png"), tr("&Line Chart View"))
    << pqSetName("LineChart");
  this->LineChartDockAction->setCheckable(true);
  this->LineChartDockAction->setChecked(true);
  this->LineChartDockAction << pqConnect(SIGNAL(triggered(bool)),
    this->LineChartDock, SLOT(setVisible(bool)));
  this->LineChartDock->installEventFilter(this);

  this->ElementDockAction = viewMenu->addAction(tr("&Element Inspector"))
    << pqSetName("Element");
  this->ElementDockAction->setCheckable(true);
  this->ElementDockAction->setChecked(true);
  this->ElementDockAction << pqConnect(SIGNAL(triggered(bool)),
    this->ElementInspectorDock, SLOT(setVisible(bool)));
  this->ElementInspectorDock->installEventFilter(this);

  this->VariableSelectorToolBar = new QToolBar(tr("Variables"), this) << pqSetName("VariableSelectorToolBar");
  this->addToolBar(Qt::TopToolBarArea, this->VariableSelectorToolBar);
  pqVariableSelectorWidget* varSelector = new pqVariableSelectorWidget(this->VariableSelectorToolBar) << pqSetName("VariableSelector");
  this->VariableSelectorToolBar->addWidget(varSelector);
  this->connect(this->PipelineList, SIGNAL(proxySelected(vtkSMProxy *)), SLOT(onProxySelected(vtkSMProxy *)));
  this->connect(varSelector, SIGNAL(variableChanged(pqVariableType, const QString&)), SLOT(onVariableChanged(pqVariableType, const QString&)));
  this->connect(varSelector, SIGNAL(variableChanged(pqVariableType, const QString&)), histogram, SLOT(setVariable(pqVariableType, const QString&)));
  this->connect(varSelector, SIGNAL(variableChanged(pqVariableType, const QString&)), line_chart, SLOT(setVariable(pqVariableType, const QString&)));

  this->CompoundProxyToolBar = new QToolBar(tr("Compound Proxies"), this) << pqSetName("CompoundProxyToolBar");
  this->addToolBar(Qt::TopToolBarArea, this->CompoundProxyToolBar);
  this->connect(this->CompoundProxyToolBar, SIGNAL(actionTriggered(QAction*)), SLOT(onCreateCompoundProxy(QAction*)));
}

pqMainWindow::~pqMainWindow()
{
  // Clean up the model before deleting the adaptor.
  if(this->Inspector)
    {
    delete this->Inspector;
    this->Inspector = 0;
    }

  // Clean up the pipeline inspector before the views.
  if(this->PipelineList)
    {
    delete this->PipelineList;
    this->PipelineList = 0;
    }
  
  // clean up multiview before server
  if(this->MultiViewManager)
    {
    delete this->MultiViewManager;
    this->MultiViewManager = 0;
    }

  this->VTKConnector->Delete();
  delete this->CurrentServer;
  delete this->Adaptor;
}

bool pqMainWindow::eventFilter(QObject* watched, QEvent* e)
{
  if(e->type() == QEvent::Hide || e->type() == QEvent::Show)
    {
    bool checked = e->type() == QEvent::Show;
    if(watched == this->PipelineDock)
      {
      this->PipelineDockAction->setChecked(checked);
      }
    else if(watched == this->InspectorDock)
      {
      this->InspectorDockAction->setChecked(checked);
      }
    else if(watched == this->HistogramDock)
      {
      this->HistogramDockAction->setChecked(checked);
      }
    else if(watched == this->LineChartDock)
      {
      this->LineChartDockAction->setChecked(checked);
      }
    else if(watched == this->ElementInspectorDock)
      {
      this->ElementDockAction->setChecked(checked);
      }
    }

  return QMainWindow::eventFilter(watched, e);
}

void pqMainWindow::setServer(pqServer* Server)
{
  if(this->CurrentServer)
    {
    this->CompoundProxyToolBar->clear();
    this->Pipeline->removeServer(this->CurrentServer);
    delete this->CurrentServer;
    }

  this->CurrentServer = Server;
  if(this->CurrentServer)
    {
    // preload compound proxies
    QDir appDir = QCoreApplication::applicationDirPath();
    if(appDir.cd("filters"))
      {
      QStringList files = appDir.entryList(QStringList() += "*.xml", QDir::Files | QDir::Readable);
      pqCompoundProxyWizard* wizard = new pqCompoundProxyWizard(this->CurrentServer, this);
      wizard->hide();
      this->connect(wizard, SIGNAL(newCompoundProxy(const QString&, const QString&)),
                            SLOT(onCompoundProxyAdded(const QString&, const QString&)));
      wizard->onLoad(files);
      delete wizard;
      }
    
    this->Pipeline->addServer(this->CurrentServer);
    this->onNewQVTKWidget(qobject_cast<pqMultiViewFrame *>(
        this->MultiViewManager->widgetOfIndex(pqMultiView::Index())));
    }

  emit serverChanged(this->CurrentServer);
}

void pqMainWindow::onFileNew()
{
  // Reset the multi-view. Use the removed widget list to clean
  // up the render modules. Then, delete the widgets.
  if(this->MultiViewManager)
    {
    QList<QWidget *> removed;
    pqMultiViewFrame *frame = 0;
    this->MultiViewManager->reset(removed);
    QList<QWidget *>::Iterator iter = removed.begin();
    for( ; iter != removed.end(); ++iter)
      {
      frame = qobject_cast<pqMultiViewFrame *>(*iter);
      if(frame)
        {
        this->cleanUpWindow(qobject_cast<QVTKWidget *>(frame->mainWidget()));
        }

      delete *iter;
      }
    }

  // Clean up the pipeline.
  if(this->Pipeline)
    {
    this->Pipeline->clearPipeline();
    }

  // Clean up the current server.
  if(this->CurrentServer)
    {
    delete this->CurrentServer;
    this->CurrentServer = 0;
    }

  // Call this method to ensure the menu items get updated.
  emit serverChanged(this->CurrentServer);
}

void pqMainWindow::onFileNew(pqServer* Server)
{
  setServer(Server);
}

void pqMainWindow::onFileOpen()
{
  if(!this->CurrentServer)
    {
    pqServerBrowser* const server_browser = new pqServerBrowser(this);
    server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onFileOpen(pqServer*)));
    server_browser->show();
    }
  else
    {
    this->onFileOpen(this->CurrentServer);
    }
}

void pqMainWindow::onFileOpen(pqServer* Server)
{
  if(this->CurrentServer != Server)
    setServer(Server);

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->CurrentServer->GetProcessModule()), tr("Open File:"), this, "fileOpenDialog");
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileOpen(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileOpen(const QStringList& Files)
{
  if(!this->Pipeline || !this->PipelineList)
    return;
    
  QVTKWidget *win = this->PipelineList->getCurrentWindow();
  if(win)
    {
    vtkSMSourceProxy* source = 0;
    vtkSMRenderModuleProxy* rm = this->Pipeline->getRenderModule(win);
    for(int i = 0; i != Files.size(); ++i)
      {
      QString file = Files[i];
      
      source = this->Pipeline->createSource("ExodusReader", win);
      this->CurrentServer->GetProxyManager()->RegisterProxy("paraq", "source1", source);
      source->Delete();
      Adaptor->setProperty(source->GetProperty("FileName"), file);
      Adaptor->setProperty(source->GetProperty("FilePrefix"), file);
      Adaptor->setProperty(source->GetProperty("FilePattern"), "%s");
      source->UpdateVTKObjects();
      this->Pipeline->setVisibility(this->Pipeline->createDisplay(source), true);
      }

    rm->ResetCamera();
    win->update();

    // Select the latest source in the pipeline inspector.
    if(source)
      this->PipelineList->selectProxy(source);
    }
}

void pqMainWindow::onFileOpenServerState()
{
  pqFileDialog *fileDialog = new pqFileDialog(new pqLocalFileDialogModel(),
      tr("Open Server State File:"), this, "fileOpenDialog");
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList&)),
      this, SLOT(onFileOpenServerState(const QStringList&)));
  fileDialog->show();
}

void pqMainWindow::onFileOpenServerState(pqServer* /*Server*/)
{
}

void pqMainWindow::onFileOpenServerState(const QStringList& Files)
{
  if(Files.size() == 0)
    {
    return;
    }

  // Clean up the current state.
  this->onFileNew();

  // Read in the xml file to restore.
  vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
  xmlParser->SetFileName(Files[0].toAscii().data());
  xmlParser->Parse();

  // Get the root element from the parser.
  vtkPVXMLElement *root = xmlParser->GetRootElement();
  QString name = root->GetName();
  if(name == "ParaQ")
    {
    // Restore the application size and position.
    vtkPVXMLElement *element = ParaQ::FindNestedElementByName(root, "MainWindow");
    if(element)
      {
      int xpos = 0;
      int ypos = 0;
      if(element->GetScalarAttribute("x", &xpos) &&
          element->GetScalarAttribute("y", &ypos))
        {
        this->move(xpos, ypos);
        }

      int w = 0;
      int h = 0;
      if(element->GetScalarAttribute("width", &w) &&
          element->GetScalarAttribute("height", &h))
        {
        this->resize(w, h);
        }
      }

    // Restore the multi-view windows.
    if(this->MultiViewManager)
      {
      this->MultiViewManager->loadState(root);
      }

    // Restore the pipeline.
    if(this->Pipeline)
      {
      this->Pipeline->loadState(root, this->MultiViewManager);
      }
    }
  else
    {
    // If the xml file is not a ParaQ file, it may be a server manager
    // state file.
    }

  xmlParser->Delete();
}

void pqMainWindow::onFileSaveServerState()
{
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save Server State:"), this, "fileSaveDialog");
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileSaveServerState(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileSaveServerState(const QStringList& Files)
{
  if(Files.size() == 0)
    {
    return;
    }

  vtkPVXMLElement *root = vtkPVXMLElement::New();
  root->SetName("ParaQ");

  // Save the size and dock window information.
  vtkPVXMLElement *element = vtkPVXMLElement::New();
  element->SetName("MainWindow");
  QString number;
  number.setNum(this->x());
  element->AddAttribute("x", number.toAscii().data());
  number.setNum(this->y());
  element->AddAttribute("y", number.toAscii().data());
  number.setNum(this->width());
  element->AddAttribute("width", number.toAscii().data());
  number.setNum(this->height());
  element->AddAttribute("height", number.toAscii().data());
  element->AddAttribute("docking", this->saveState().data());
  root->AddNestedElement(element);
  element->Delete();

  if(this->MultiViewManager)
    {
    this->MultiViewManager->saveState(root);
    }

  if(this->Pipeline)
    {
    this->Pipeline->saveState(root, this->MultiViewManager);
    }

  // Print the xml to the requested file(s).
  for(int i = 0; i != Files.size(); ++i)
    {
    ofstream os(Files[i].toAscii().data(), ios::out);
    root->PrintXML(os, vtkIndent());
    }

  root->Delete();
}

void pqMainWindow::onFileSaveScreenshot()
{
  if(!this->CurrentServer)
    {
    QMessageBox::critical(this, tr("Save Screenshot:"), tr("No server connections to save"));
    return;
    }

  vtkRenderWindow* const render_window =
    this->ActiveView ? qobject_cast<QVTKWidget*>(this->ActiveView->mainWidget())->GetRenderWindow() : 0;

  if(!render_window)
    {
    QMessageBox::critical(this, tr("Save Screenshot:"), tr("No render window to save"));
    return;
    }

  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save Screenshot:"), this, "fileSaveDialog");
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileSaveScreenshot(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileSaveScreenshot(const QStringList& Files)
{
  vtkRenderWindow* const render_window =
    this->ActiveView ? qobject_cast<QVTKWidget*>(this->ActiveView->mainWidget())->GetRenderWindow() : 0;

  for(int i = 0; i != Files.size(); ++i)
    {
    if(!pqSaveScreenshot(render_window, Files[i]))
      QMessageBox::critical(this, tr("Save Screenshot:"), tr("Error saving file"));
    }
}

bool pqMainWindow::compareView(const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory)
{
  vtkRenderWindow* const render_window =
    this->ActiveView ? qobject_cast<QVTKWidget*>(this->ActiveView->mainWidget())->GetRenderWindow() : 0;
    
  if(!render_window)
    return false;
    
  return pqCompareImage(render_window, ReferenceImage, Threshold, Output, TempDirectory);
}

void pqMainWindow::onServerConnect()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onServerConnect(pqServer*)));
  server_browser->show();
}

void pqMainWindow::onServerConnect(pqServer* Server)
{
  setServer(Server);
}

void pqMainWindow::onServerDisconnect()
{
  setServer(0);
}

void pqMainWindow::onUpdateWindows()
{
  /*
  if(this->CurrentServer)
    this->CurrentServer->GetRenderModule()->StillRender();
    */
}

void pqMainWindow::onUpdateSourcesFiltersMenu(pqServer* /*Server*/)
{
  this->FiltersMenu->clear();
  this->SourcesMenu->clear();

  // Update the menu items for the server and compound filters too.
  QAction* compoundFilterAction = this->ToolsMenu->findChild<QAction*>(
      "CompoundFilterAction");
  compoundFilterAction->setEnabled(this->CurrentServer);
  this->ServerDisconnectAction->setEnabled(this->CurrentServer);

  if(this->CurrentServer)
    {
    vtkSMProxyManager* manager = this->CurrentServer->GetProxyManager();
    manager->InstantiateGroupPrototypes("filters");
    int numFilters = manager->GetNumberOfProxies("filters_prototypes");
    for(int i=0; i<numFilters; i++)
      {
      const char* proxyName = manager->GetProxyName("filters_prototypes",i);
      this->FiltersMenu->addAction(proxyName) << pqSetName(proxyName) << pqSetData(proxyName);
      }

#if 1
    // hard code sources
    this->SourcesMenu->addAction("2D Glyph") << pqSetName("2D Glyph") << pqSetData("GlyphSource2D");
    this->SourcesMenu->addAction("3D Text") << pqSetName("3D Text") << pqSetData("VectorText");
    this->SourcesMenu->addAction("Arrow") << pqSetName("Arrow") << pqSetData("ArrowSource");
    this->SourcesMenu->addAction("Axes") << pqSetName("Axes") << pqSetData("Axes");
    this->SourcesMenu->addAction("Box") << pqSetName("Box") << pqSetData("CubeSource");
    this->SourcesMenu->addAction("Cone") << pqSetName("Cone") << pqSetData("ConeSource");
    this->SourcesMenu->addAction("Cylinder") << pqSetName("Cylinder") << pqSetData("CylinderSource");
    this->SourcesMenu->addAction("Hierarchical Fractal") << pqSetName("Hierarchical Fractal") << pqSetData("HierarchicalFractal");
    this->SourcesMenu->addAction("Line") << pqSetName("Line") << pqSetData("LineSource");
    this->SourcesMenu->addAction("Mandelbrot") << pqSetName("Mandelbrot") << pqSetData("ImageMandelbrotSource");
    this->SourcesMenu->addAction("Plane") << pqSetName("Plane") << pqSetData("PlaneSource");
    this->SourcesMenu->addAction("Sphere") << pqSetName("Sphere") << pqSetData("SphereSource");
    this->SourcesMenu->addAction("Superquadric") << pqSetName("Superquadric") << pqSetData("SuperquadricSource");
    this->SourcesMenu->addAction("Wavelet") << pqSetName("Wavelet") << pqSetData("RTAnalyticSource");
#else
    manager->InstantiateGroupPrototypes("sources");
    int numSources = manager->GetNumberOfProxies("sources_prototypes");
    for(int i=0; i<numSources; i++)
      {
      const char* proxyName = manager->GetProxyName("sources_prototypes",i);
      this->SourcesMenu->addAction(proxyName) << pqSetName(proxyName) << pqSetData(proxyName);
      }
#endif
    }
}

void pqMainWindow::onCreateSource(QAction* action)
{
  if(!action || !this->Pipeline || !this->PipelineList)
    return;

  QByteArray sourceName = action->data().toString().toAscii();
  QVTKWidget* win = this->PipelineList->getCurrentWindow();
  if(win)
    {
    vtkSMSourceProxy* source = this->Pipeline->createSource(sourceName, win);
    this->Pipeline->setVisibility(this->Pipeline->createDisplay(source), true);
    vtkSMRenderModuleProxy* rm = this->Pipeline->getRenderModule(win);
    rm->ResetCamera();
    win->update();
    this->PipelineList->selectProxy(source);
    }
}

void pqMainWindow::onCreateFilter(QAction* action)
{
  if(!action || !this->Pipeline || !this->PipelineList)
    return;
  
  QByteArray filterName = action->data().toString().toAscii();
  vtkSMProxy *current = this->PipelineList->getSelectedProxy();
  QVTKWidget *win = this->PipelineList->getCurrentWindow();
  if(current && win)
    {
    vtkSMSourceProxy *source = 0;
    vtkSMProxy *next = this->PipelineList->getNextProxy();
    if(next)
      {
      this->PipelineList->getListModel()->beginCreateAndInsert();
      source = this->Pipeline->createFilter(filterName, win);
      this->Pipeline->addInput(source, current);
      this->Pipeline->addInput(next, source);
      this->Pipeline->removeInput(next, current);
      this->PipelineList->getListModel()->finishCreateAndInsert();
      this->Pipeline->setVisibility(this->Pipeline->createDisplay(source), false);
      }
    else
      {
      this->PipelineList->getListModel()->beginCreateAndAppend();
      source = this->Pipeline->createFilter(filterName, win);
      this->Pipeline->addInput(source, current);
      this->PipelineList->getListModel()->finishCreateAndAppend();

      // Only turn on visibility for added filters?
      this->Pipeline->setVisibility(this->Pipeline->createDisplay(source), true);
      }

    vtkSMRenderModuleProxy *rm = this->Pipeline->getRenderModule(win);
    rm->ResetCamera();
    win->update();
    this->PipelineList->selectProxy(source);
    }
}

void pqMainWindow::onOpenLinkEditor()
{
}

void pqMainWindow::onOpenCompoundFilterWizard()
{
  pqCompoundProxyWizard* wizard = new pqCompoundProxyWizard(this->CurrentServer, this);
  wizard->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed

  this->connect(wizard, SIGNAL(newCompoundProxy(const QString&, const QString&)), 
                        SLOT(onCompoundProxyAdded(const QString&, const QString&)));
  
  wizard->show();
}


void pqMainWindow::onHelpAbout()
{
  pqAboutDialog* const dialog = new pqAboutDialog(this);
  dialog->show();
}

void pqMainWindow::onRecordTest()
{
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Record Test:"), this, "fileSaveDialog");
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onRecordTest(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onRecordTest(const QStringList& Files)
{
  for(int i = 0; i != Files.size(); ++i)
    {
    pqRecordEventsDialog* const dialog = new pqRecordEventsDialog(Files[i], this);
    dialog->show();
    }
}

void pqMainWindow::onPlayTest()
{
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Record Test:"), this, "fileSaveDialog");
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onPlayTest(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onPlayTest(const QStringList& Files)
{
  pqEventPlayer player(*this);
  player.addDefaultWidgetEventPlayers();

  for(int i = 0; i != Files.size(); ++i)
    {
      pqEventPlayerXML xml_player;
      xml_player.playXML(player, Files[i]);
    }
}

void pqMainWindow::onPythonShell()
{
#ifdef PARAQ_EMBED_PYTHON
  pqPythonDialog* const dialog = new pqPythonDialog(this);
  dialog->show();
#endif // PARAQ_EMBED_PYTHON
}

void pqMainWindow::onNewQVTKWidget(pqMultiViewFrame* frame)
{
  QVTKWidget *widget = ParaQ::AddQVTKWidget(frame, this->MultiViewManager,
      this->CurrentServer);
  if(widget)
    {
    // Select the new window in the pipeline list.
    this->PipelineList->selectWindow(widget);

    QSignalMapper* sm = new QSignalMapper(frame);
    sm->setMapping(frame, frame);
    QObject::connect(frame, SIGNAL(activeChanged(bool)), sm, SLOT(map()));
    QObject::connect(sm, SIGNAL(mapped(QWidget*)), this, SLOT(onFrameActive(QWidget*)));

    //frame->setActive(1);
    }
}

void pqMainWindow::onDeleteQVTKWidget(pqMultiViewFrame* p)
{
  QVTKWidget* w = qobject_cast<QVTKWidget*>(p->mainWidget());
  this->cleanUpWindow(w);

  // Remove the window from the pipeline data structure.
  this->Pipeline->removeWindow(w);

  if(this->ActiveView == p)
    {
    this->ActiveView = 0;
    }
}

void pqMainWindow::onFrameActive(QWidget* w)
{
  if(this->ActiveView && this->ActiveView != w)
    {
    pqMultiViewFrame* f = qobject_cast<pqMultiViewFrame*>(this->ActiveView);
    if(f->active())
      f->setActive(0);
    }

  this->ActiveView = qobject_cast<pqMultiViewFrame*>(w);
}

void pqMainWindow::onNewSelections(vtkSMProxy*, vtkUnstructuredGrid* selections)
{
  pqDataSetModel* model = qobject_cast<pqDataSetModel*>(
      this->ElementInspectorWidget->model());
  if(!model)
    {
    model = new pqDataSetModel(this->ElementInspectorWidget);
    this->ElementInspectorWidget->setModel(model);
    }

  if(model)
    {
    model->setDataSet(selections);
    }
  
  this->ElementInspectorWidget->reset();
  this->ElementInspectorWidget->update();
}

void pqMainWindow::onCreateCompoundProxy(QAction* action)
{
  if(!action || !this->Pipeline || !this->PipelineList)
    return;
  
  QByteArray filterName = action->data().toString().toAscii();
  vtkSMProxy *current = this->PipelineList->getSelectedProxy();
  QVTKWidget *win = this->PipelineList->getCurrentWindow();
  if(current && win)
    {
    vtkSMCompoundProxy *source = 0;
    vtkSMProxy *next = this->PipelineList->getNextProxy();
    bool vis = false;
    if(next)
      {
      this->PipelineList->getListModel()->beginCreateAndInsert();
      source = this->Pipeline->createCompoundProxy(filterName, win);
      this->Pipeline->addInput(source->GetMainProxy(), current);
      this->Pipeline->addInput(next, source);
      this->Pipeline->removeInput(next, current);
      this->PipelineList->getListModel()->finishCreateAndInsert();
      }
    else
      {
      this->PipelineList->getListModel()->beginCreateAndAppend();
      source = this->Pipeline->createCompoundProxy(filterName, win);
      this->Pipeline->addInput(source, current);
      this->PipelineList->getListModel()->finishCreateAndAppend();
      vis = true;
      // Only turn on visibility for added filters?
      }

    if(source)
      {
      // TODO: how to decide which part of the compound proxy to display ????
      // for now just get the last one, and assuming it is a source proxy
      vtkSMProxy* sourceDisplay = NULL;
      for(int i=source->GetNumberOfProxies(); sourceDisplay == NULL && i>0; i--)
        {
        sourceDisplay = vtkSMSourceProxy::SafeDownCast(source->GetProxy(i-1));
        }
      this->Pipeline->setVisibility(this->Pipeline->createDisplay(vtkSMSourceProxy::SafeDownCast(sourceDisplay), source), vis);
      //this->Pipeline->setVisibility(this->Pipeline->createDisplay(source), vis);
      }

    vtkSMRenderModuleProxy *rm = this->Pipeline->getRenderModule(win);
    rm->ResetCamera();
    win->update();
    this->PipelineList->selectProxy(source);
    }
}

void pqMainWindow::onCompoundProxyAdded(const QString&, const QString& proxy)
{
  this->CompoundProxyToolBar->addAction(QIcon(":/pqWidgets/pqFilter32.png"), proxy) 
    << pqSetName(proxy) << pqSetData(proxy);
}

void pqMainWindow::onAddServer(pqPipelineServer *server)
{
  // When restoring a state file, the PipelineData object will
  // create the pqServer. Make sure the CurrentServer gets set.
  if(server && !this->CurrentServer)
    {
    this->CurrentServer = server->GetServer();
    emit serverChanged(this->CurrentServer);
    }
}

void pqMainWindow::onRemoveServer(pqPipelineServer *server)
{
  if(!server || !this->MultiViewManager)
    {
    return;
    }

  // Clean up all the views associated with the server.
  QWidget *widget = 0;
  pqPipelineWindow *win = 0;
  int total = server->GetWindowCount();
  this->MultiViewManager->blockSignals(true);
  for(int i = 0; i < total; i++)
    {
    win = server->GetWindow(i);
    widget = win->GetWidget();

    // Clean up the render module.
    this->cleanUpWindow(qobject_cast<QVTKWidget *>(widget));

    // Remove the window from the multi-view.
    this->MultiViewManager->removeWidget(widget->parentWidget());
    }

  this->MultiViewManager->blockSignals(false);
}

void pqMainWindow::onAddWindow(pqPipelineWindow *win)
{
  if(!win)
    {
    return;
    }

  QVTKWidget *widget = qobject_cast<QVTKWidget *>(win->GetWidget());
  if(widget)
    {
    // Get the render module from the view map. Use the render module,
    // the interactor, and the qvtk widget to set up picking.
    vtkSMRenderModuleProxy* view = this->Pipeline->getRenderModule(widget);
    vtkPVGenericRenderWindowInteractor* iren =
        vtkPVGenericRenderWindowInteractor::SafeDownCast(view->GetInteractor());
    pqPicking* picking = new pqPicking(view, widget);
    this->VTKConnector->Connect(iren, vtkCommand::CharEvent, picking,
        SLOT(computeSelection(vtkObject*,unsigned long, void*, void*, vtkCommand*)),
        NULL, 1.0);
    QObject::connect(picking,
        SIGNAL(selectionChanged(vtkSMProxy*, vtkUnstructuredGrid*)),
        this, SLOT(onNewSelections(vtkSMProxy*, vtkUnstructuredGrid*)));
    }
}

void pqMainWindow::cleanUpWindow(QVTKWidget *win)
{
  if(win && this->Pipeline)
    {
    // Remove the render module from the pipeline's view map and delete it.
    vtkSMRenderModuleProxy *rm = this->Pipeline->removeViewMapping(win);
    if(rm)
      {
      rm->Delete();
      }
    }
}


void pqMainWindow::onProxySelected(vtkSMProxy* p)
{
  this->CurrentProxy = p;

  pqVariableSelectorWidget* selector = this->VariableSelectorToolBar->findChild<pqVariableSelectorWidget*>();
  selector->clear();

  if(!this->CurrentProxy)
    return;
    
  pqPipelineObject* pqObject = pqPipelineData::instance()->getObjectFor(p);
  vtkSMDisplayProxy* display = pqObject->GetDisplayProxy();
  if(!display)
    return;
  
  pqPipelineObject* reader = pqObject;
  while(reader->GetInput(0))
    reader = reader->GetInput(0);

  vtkPVDataInformation* geomInfo = display->GetGeometryInformation();
  vtkPVDataSetAttributesInformation* cellinfo = geomInfo->GetCellDataInformation();
  int i;
  for(i=0; i<cellinfo->GetNumberOfArrays(); i++)
    {
    vtkPVArrayInformation* info = cellinfo->GetArrayInformation(i);
    selector->addVariable(VARIABLE_TYPE_CELL, info->GetName());
    }
  
  // also include unloaded arrays if any
  QList<QVariant> extraCellArrays = pqSMAdaptor::instance()->getProperty(reader->GetProxy()->GetProperty("CellArrayStatus")).toList();
  for(i=0; i<extraCellArrays.size(); i++)
    {
    QList<QVariant> cell = extraCellArrays[i].toList();
    if(cell[1] == false)
      {
      selector->addVariable(VARIABLE_TYPE_CELL, cell[0].toString());
      }
    }

  
  vtkPVDataSetAttributesInformation* pointinfo = geomInfo->GetPointDataInformation();
  for(i=0; i<pointinfo->GetNumberOfArrays(); i++)
    {
    vtkPVArrayInformation* info = pointinfo->GetArrayInformation(i);
    selector->addVariable(VARIABLE_TYPE_NODE, info->GetName());
    }
  
  // also include unloaded arrays if any
  QList<QVariant> extraPointArrays = pqSMAdaptor::instance()->getProperty(reader->GetProxy()->GetProperty("PointArrayStatus")).toList();
  for(i=0; i<extraPointArrays.size(); i++)
    {
    QList<QVariant> cell = extraPointArrays[i].toList();
    if(cell[1] == false)
      {
      selector->addVariable(VARIABLE_TYPE_NODE, cell[0].toString());
      }
    }

  // set to the active display scalar
  vtkSMStringVectorProperty* d_svp = vtkSMStringVectorProperty::SafeDownCast(display->GetProperty("ColorArray"));
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(display->GetProperty("ScalarMode"));
  int fieldtype = ivp->GetElement(0);
  if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
    {
    selector->chooseVariable(VARIABLE_TYPE_CELL, d_svp->GetElement(0));
    }
  else
    {
    selector->chooseVariable(VARIABLE_TYPE_NODE, d_svp->GetElement(0));
    }
}

void pqMainWindow::onVariableChanged(pqVariableType type, const QString& name)
{
  if(this->CurrentProxy)
    {
    pqPipelineObject* o = pqPipelineData::instance()->getObjectFor(this->CurrentProxy);
    vtkSMDisplayProxy* display = o->GetDisplayProxy();
    QWidget* widget = o->GetParent()->GetWidget();

    switch(type)
      {
      case VARIABLE_TYPE_CELL:
        pqColorPart(display, name.toAscii().data(), vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
        break;
      case VARIABLE_TYPE_NODE:
        pqColorPart(display, name.toAscii().data(), vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
        break;
      }
          
    widget->update();
    }
}
