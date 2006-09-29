/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqMainWindowCore.h,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#ifndef _pqMainWindowCore_h
#define _pqMainWindowCore_h

#include "pqComponentsExport.h"
#include "pqVariableType.h"

#include <vtkIOStream.h>

#include <QObject>

class pqGenericViewModule;
class pqMultiView;
class pqObjectInspectorWidget;
class pqPipelineMenu;
class pqPipelineSource;
class pqPlotViewModule;
class pqProxy;
class pqProxyTabWidget;
class pqRenderModule;
class pqRenderWindowManager;
class pqRenderWindowManager;
class pqSelectionManager;
class pqServer;
class pqServerManagerModelItem;
class pqToolsMenu;
class pqVCRController;
class pqViewMenu;

class vtkUnstructuredGrid;

class QAction;
class QDockWidget;
class QIcon;
class QMenu;
class QStatusBar;
class QToolBar;
class QWidget;

/** \brief Provides a standardized main window for ParaView applications -
application authors can derive from pqMainWindowCore and call its member functions
to use as-much or as-little of the standardized functionality as desired */

class PQCOMPONENTS_EXPORT pqMainWindowCore :
  public QObject
{
  Q_OBJECT
  
public:
  pqMainWindowCore(QWidget* parent);
  ~pqMainWindowCore();

  /// Returns a multi-view widget which can be embedded in the UI  
  pqRenderWindowManager& multiViewManager();

  /// Returns the selection manager, which handles interactive selection
  pqSelectionManager& selectionManager();
  /// Returns the VCR controller, which can control animation playback
  pqVCRController& VCRController();
  
  /// Assigns a menu to be populated with sources
  void setSourceMenu(QMenu* menu);
  /// Assigns a menu to be populated with filters
  void setFilterMenu(QMenu* menu);
  
  pqPipelineMenu& pipelineMenu();
  
  /// Setup a pipeline browser, attaching it to the given dock
  void setupPipelineBrowser(QDockWidget* parent);
  /// Setup a proxy tab widget, attaching it to the given dock
  pqProxyTabWidget* setupProxyTabWidget(QDockWidget* parent);
  /// Setup an object inspector, attaching it to the given dock
  pqObjectInspectorWidget* setupObjectInspector(QDockWidget* parent);
  /// Setup a statistics view, attaching it to the given dock
  void setupStatisticsView(QDockWidget* parent);
  /// Setup an element inspector, attaching it to the given dock
  void setupElementInspector(QDockWidget* parent);
  
  /// Setup a variable-selection toolbar
  void setupVariableToolbar(QToolBar* parent);
  /// Setup a compound-proxy toolbar
  void setupCustomFilterToolbar(QToolBar* parent);
  /// Setup a representation-selection toolbar
  void setupRepresentationToolbar(QToolBar* parent);
  
  /// Setup a progress bar, attaching it to the given status bar
  void setupProgressBar(QStatusBar* parent);

  /** Compares the contents of the window with the given reference image,
  returns true iff they "match" within some tolerance */
  bool compareView(
    const QString& ReferenceImage,
    double Threshold,
    ostream& Output,
    const QString& TempDirectory);
  
  /// Call this once all of your slots/signals are connected, to
  /// set the initial state of GUI components
  void initializeStates();

  /// returns the active source.
  pqPipelineSource* getActiveSource();

  /// returns the active server.
  pqServer* getActiveServer();

  /// returns the active render module.
  pqRenderModule* getActiveRenderModule();
  pqGenericViewModule* getActiveView();
  
  void removeActiveSource();
  void removeActiveServer();
  
  // This will create a source with the given xmlname on the active server. 
  // On success returns
  // pqPipelineSource for the source proxy. The actual creation is delegated 
  // to pqPipelineBuilder instance. Using this method will optionally,
  // create a display for the source in the active render window (if both
  // the active window is indeed on the active server. The created source
  // becomes the active source.
  pqPipelineSource* createSourceOnActiveServer(const QString& xmlname);

  // This will create a filter and connect it to the active source.
  // The actual creation is delegated 
  // to pqPipelineBuilder instance. Using this method will optionally,
  // create a display for the source in the active render window (if both
  // the active window is indeed on the active server. The created source
  // becomes the active source.
  pqPipelineSource* createFilterForActiveSource( const QString& xmlname);

  // This will instantiate and register a compound proxy. A compound proxy
  // definition with the given name must have already been registered with
  // the proxy manager. If the compound proxy needs an input, the active
  // source will be used as the input. 
  pqPipelineSource* createCompoundSource(const QString& name);

  // Utility function to create a reader that reads the file on the 
  // active server. 
  pqPipelineSource* createReaderOnActiveServer( const QString& filename);
  
  /** By default, whenever a new source/filter is added to the pipeline,
  pqMainWindowCore will attach a display.  Clients that wish to manage
  their own pipeline / displays should call this method once at startup. */
  void disableAutomaticDisplays();
  
signals:
  void enableFileOpen(bool);
  void enableFileLoadServerState(bool);
  void enableFileSaveServerState(bool);
  void enableFileSaveData(bool);
  void enableFileSaveScreenshot(bool);
  void enableFileSaveAnimation(bool);
  void enableCameraUndo(bool);
  void enableCameraRedo(bool);
  void cameraUndoLabel(const QString&);
  void cameraRedoLabel(const QString&);
  void enableServerConnect(bool);
  void enableServerDisconnect(bool);
  void enableSourceCreate(bool);
  void enableFilterCreate(bool);
  void enableVariableToolbar(bool);
  void enableSelectionToolbar(bool);
  
  /** \todo Hide these private implementation details */
  void postAccept();
  void select(pqServerManagerModelItem*);
  
  // Fired when the active source changes.
  void activeSourceChanged(pqPipelineSource*);
  void activeSourceChanged(pqProxy*);

  // Fired when the active server changes.
  void activeServerChanged(pqServer*);

  // Fired when the active render module changes.
  // All these are fired at the same time, expect
  // that depending upon the type of the active view
  // some signals may have null arguments.
  void activeRenderModuleChanged(pqRenderModule*);
  void activePlotModuleChanged(pqPlotViewModule*);
  void activeViewChanged(pqGenericViewModule*);
  
  // Fired when a source/filter/reader/compound proxy is
  // created without a display.
  void pendingDisplays(bool status);


public slots:
  virtual void onFileOpen();
  virtual void onFileOpen(pqServer* Server);
  virtual void onFileOpen(const QStringList& Files);

  void onFileLoadServerState();
  void onFileLoadServerState(pqServer* Server);
  void onFileLoadServerState(const QStringList& Files);

  void onFileSaveServerState();
  void onFileSaveServerState(const QStringList& Files);

  void onFileSaveData();
  void onFileSaveData(const QStringList& files);

  void onFileSaveScreenshot();
  void onFileSaveScreenshot(const QStringList& Files);

  void onFileSaveAnimation();
  void onFileSaveAnimation(const QStringList& files);
  
  void onEditCameraUndo();
  void onEditCameraRedo();
  
  void onServerConnect();
  void onServerDisconnect();

  void onToolsCreateCustomFilter();
  void onToolsManageCustomFilters();

  void onToolsDumpWidgetNames();
  
  void onToolsRecordTest();
  void onToolsRecordTest(const QStringList &fileNames);
  
  void onToolsRecordTestScreenshot();
  void onToolsRecordTestScreenshot(const QStringList &fileNames);
  
  void onToolsPlayTest();
  void onToolsPlayTest(const QStringList &fileNames);
  
  void onToolsPythonShell();
  
  void onHelpEnableTooltips(bool enabled = true);
  
  // Call this slot to set the active source. 
  void setActiveSource(pqPipelineSource*);

  // Call this slot to set the active server. 
  void setActiveServer(pqServer*);

  // Call this slot to set the active render module.
  // Internally calls setActiveView().
  void setActiveRenderModule(pqRenderModule* ren);
  
  // Internally calls setActiveView().
  void setActivePlotModule(pqPlotViewModule* plot);

  // Changes the active view. At one time either 1 render view
  // or 1 plot view can be active.
  void setActiveView(pqGenericViewModule* view);

  // Call this slot when accept is called. This method will create
  // displays for any sources/filters that are pending.
  void createPendingDisplays();

  void filtersActivated();

  // Called to show the settings dialog.
  void onEditSettings();

  // Camera slots.
  void resetCamera();
  void resetViewDirectionPosX();
  void resetViewDirectionNegX();
  void resetViewDirectionPosY();
  void resetViewDirectionNegY();
  void resetViewDirectionPosZ();
  void resetViewDirectionNegZ();

  // Create New Plot Views.
  void createBarCharView();

private slots:
  void onCreateSource(QAction*);
  void onCreateFilter(QAction*);

  void onCreateCompoundProxy(QAction*);
  void onCompoundProxyAdded(QString proxy);
  void onCompoundProxyRemoved(QString proxy);
  void onActiveSourceChanged(pqPipelineSource*);
  void onActiveServerChanged(pqServer*);
  void onActiveRenderModuleChanged(pqRenderModule* rm);
  void onCoreActiveChanged();

  void onInitializeStates();
  void onInitializeInteractionStates();

  void onPostAccept();

  // Called when selection on the pqPipelineBrowser changes.
  // Use this slot to communicate the pipeline browser selection to the
  // ApplicationCore. Any work that needs to be done on selection
  // change should actually be done by monitoring selection events
  // from the ApplicationCore.
  void onBrowserSelectionChanged(pqServerManagerModelItem*);

  // enable/disable filters as per the source.
  void updateFiltersMenu(pqPipelineSource* source);

  // called when a source is removed by the pqServerManagerModel. If
  // the removed source is the active source, we must change it.
  void sourceRemoved(pqPipelineSource*);
  void serverRemoved(pqServer* server);

  // Called when a new server is added to the pqServerManagerModel.
  void serverAdded(pqServer* server);
  
  // Performs the set of actions need to be performed after a new 
  // source/reader/filter/customfilter is created. This includes
  // seting up handler to create a default display proxy for the source
  // after first accept, set up undo stack so that the undo/redo
  // works correctly with pending displays etc etc.
  void onSourceCreated(pqPipelineSource*);
  
private:
  class pqImplementation;
  pqImplementation* const Implementation;

  void resetViewDirection(double x, double y, double z);
};

#endif // !_pqMainWindowCore_h
