/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqViewManager.cxx,v $

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
#include "pqViewManager.h"
#include "ui_pqEmptyView.h"

// VTK includes.
#include "QVTKWidget.h"
#include "vtkPVConfig.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStateLoader.h"

// Qt includes.
#include <QAction>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QList>
#include <QMap>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QPushButton>
#include <QSet>
#include <QSignalMapper>
#include <QtDebug>
#include <QUuid>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqElementInspectorViewModule.h"
#include "pqMultiViewFrame.h"
#include "pqObjectBuilder.h"
#include "pqPluginManager.h"
#include "pqRenderViewModule.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSplitViewUndoElement.h"
#include "pqUndoStack.h"
#include "pqViewModuleInterface.h"
#include "pqXMLUtil.h"

#if WIN32
#include "process.h"
#define getpid _getpid
#else
#include "unistd.h"
#endif

template<class T>
uint qHash(const QPointer<T> key)
{
  return qHash((T*)key);
}
//-----------------------------------------------------------------------------
class pqViewManager::pqInternals 
{
public:
  QPointer<pqServer> ActiveServer;
  QPointer<pqGenericViewModule> ActiveViewModule;
  QPointer<pqUndoStack> UndoStack;
  QMenu ConvertMenu;

  typedef QMap<pqMultiViewFrame*, QPointer<pqGenericViewModule> > FrameMapType;
  FrameMapType Frames;

  QList<QPointer<pqMultiViewFrame> > PendingFrames;
  QList<QPointer<pqGenericViewModule> > PendingViews;

  QSize MaxWindowSize;

  bool DontCreateDeleteViewsModules;

  // When a frame is being closed, we create an undo element
  // and  save it to be pushed on the stack after the 
  // view in the frame has been unregistered. The sequence
  // of operations on the undo stack is 
  // * unregister view
  // * close frame
  vtkSmartPointer<pqSplitViewUndoElement> CloseFrameUndoElement;
};

//-----------------------------------------------------------------------------
pqViewManager::pqViewManager(QWidget* _parent/*=null*/)
  : pqMultiView(_parent)
{
  this->Internal = new pqInternals();
  this->Internal->DontCreateDeleteViewsModules = false;
  this->Internal->MaxWindowSize = QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
  pqServerManagerModel* smModel = pqServerManagerModel::instance();
  if (!smModel)
    {
    qDebug() << "pqServerManagerModel instance must be created before "
      <<"pqViewManager.";
    return;
    }

  // We need to know when new view modules are added.
  QObject::connect(smModel, SIGNAL(viewModuleAdded(pqGenericViewModule*)),
    this, SLOT(onViewModuleAdded(pqGenericViewModule*)));
  QObject::connect(smModel, SIGNAL(viewModuleRemoved(pqGenericViewModule*)),
    this, SLOT(onViewModuleRemoved(pqGenericViewModule*)));

  // Record creation/removal of frames.
  QObject::connect(this, SIGNAL(frameAdded(pqMultiViewFrame*)), 
    this, SLOT(onFrameAdded(pqMultiViewFrame*)));
  QObject::connect(this, SIGNAL(preFrameRemoved(pqMultiViewFrame*)), 
    this, SLOT(onPreFrameRemoved(pqMultiViewFrame*)));
  QObject::connect(this, SIGNAL(frameRemoved(pqMultiViewFrame*)), 
    this, SLOT(onFrameRemoved(pqMultiViewFrame*)));

  QObject::connect(this, 
    SIGNAL(afterSplitView(const Index&, Qt::Orientation, float, const Index&)),
    this, SLOT(onSplittingView(const Index&, Qt::Orientation, float, const Index&)));
  
  QObject::connect(&this->Internal->ConvertMenu, SIGNAL(triggered(QAction*)),
    this, SLOT(onConvertToTriggered(QAction*)));

  // Creates the default empty frame.
  this->init();
}

//-----------------------------------------------------------------------------
pqViewManager::~pqViewManager()
{
  // Cleanup all render modules.
  foreach (pqMultiViewFrame* frame , this->Internal->Frames.keys())
    {
    if (frame)
      {
      this->onFrameRemovedInternal(frame);
      }
    }
  pqServerManagerModel* smModel = pqServerManagerModel::instance();
  if (smModel)
    {
    QObject::disconnect(smModel, 0, this, 0);
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqViewManager::buildConvertMenu()
{
  this->Internal->ConvertMenu.clear();

  QAction* view_action = new QAction("3D View", this);
  view_action->setData(pqRenderViewModule::renderViewType());
  this->Internal->ConvertMenu.addAction(view_action);

  // Create actions for converting view types.
  QObjectList ifaces =
    pqApplicationCore::instance()->getPluginManager()->interfaces();
  foreach(QObject* iface, ifaces)
    {
    pqViewModuleInterface* vi = qobject_cast<pqViewModuleInterface*>(iface);
    if(vi)
      {
      QStringList viewtypes = vi->viewTypes();
      QStringList::iterator iter;
      for(iter = viewtypes.begin(); iter != viewtypes.end(); ++iter)
        {
        if ((*iter) == "TableView" || (*iter) == "ElementInspectorView")
          {
          // Ignore these views for now.
          continue;
          }
        view_action = new QAction(vi->viewTypeName(*iter), this);
        view_action->setData(*iter);
        this->Internal->ConvertMenu.addAction(view_action);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::setActiveServer(pqServer* server)
{
  this->Internal->ActiveServer = server;
}

//-----------------------------------------------------------------------------
pqGenericViewModule* pqViewManager::getActiveViewModule() const
{
  return this->Internal->ActiveViewModule;
}

//-----------------------------------------------------------------------------
void pqViewManager::setUndoStack(pqUndoStack* stack)
{
  if (this->Internal->UndoStack)
    {
    QObject::disconnect(this->Internal->UndoStack, 0, this, 0);
    }

  this->Internal->UndoStack = stack;

  if (stack)
    {
    QObject::connect(this, SIGNAL(beginUndo(const QString&)),
      stack, SLOT(beginUndoSet(QString)));
    QObject::connect(this, SIGNAL(endUndo()), stack, SLOT(endUndoSet()));
    QObject::connect(this, SIGNAL(addToUndoStack(vtkUndoElement*)),
      stack, SLOT(addToActiveUndoSet(vtkUndoElement*)));
    QObject::connect(this, SIGNAL(beginNonUndoableChanges()),
      stack, SLOT(beginNonUndoableChanges()));
    QObject::connect(this, SIGNAL(endNonUndoableChanges()),
      stack, SLOT(endNonUndoableChanges()));
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::updateConversionActions(pqMultiViewFrame* frame)
{
  QString to_exclude;
  if (this->Internal->Frames.contains(frame))
    {
    to_exclude = this->Internal->Frames[frame]->getViewType();
    }

  bool server_exists = (pqApplicationCore::instance()->getServerManagerModel()->
    getNumberOfServers() >= 1);
  foreach (QAction* action, this->Internal->ConvertMenu.actions())
    {
    action->setEnabled(server_exists && (to_exclude != action->data().toString()));
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onFrameAdded(pqMultiViewFrame* frame)
{
  // We connect drag-drop signals event for empty frames.
  QObject::connect(frame, SIGNAL(dragStart(pqMultiViewFrame*)),
    this, SLOT(frameDragStart(pqMultiViewFrame*)));
  QObject::connect(frame, SIGNAL(dragEnter(pqMultiViewFrame*,QDragEnterEvent*)),
    this, SLOT(frameDragEnter(pqMultiViewFrame*,QDragEnterEvent*)));
  QObject::connect(frame, SIGNAL(dragMove(pqMultiViewFrame*,QDragMoveEvent*)),
    this, SLOT(frameDragMove(pqMultiViewFrame*,QDragMoveEvent*)));
  QObject::connect(frame, SIGNAL(drop(pqMultiViewFrame*,QDropEvent*)),
    this, SLOT(frameDrop(pqMultiViewFrame*,QDropEvent*)));

  frame->installEventFilter(this);
  frame->MaximizeButton->show();
  frame->CloseButton->show();
  frame->SplitVerticalButton->show();
  frame->SplitHorizontalButton->show();
  frame->LookmarkButton->show();

  frame->getContextMenu()->addSeparator();
  QAction* subAction = frame->getContextMenu()->addMenu(
    &this->Internal->ConvertMenu);
  subAction->setText("Convert To");

  QSignalMapper* sm = new QSignalMapper(frame);
  sm->setMapping(frame, frame);
  QObject::connect(frame, SIGNAL(activeChanged(bool)), sm, SLOT(map()));
  QObject::connect(sm, SIGNAL(mapped(QWidget*)), 
    this, SLOT(onActivate(QWidget*)));

  sm = new QSignalMapper(frame);
  sm->setMapping(frame, frame);
  QObject::connect(frame, SIGNAL(contextMenuRequested()), sm, SLOT(map()));
  QObject::connect(sm, SIGNAL(mapped(QWidget*)), 
    this, SLOT(onFrameContextMenuRequested(QWidget*)));

  // A newly added frames gets collected as an empty frame.
  // It will be used next time a view module is created.
  this->Internal->PendingFrames.removeAll(frame);
  this->Internal->PendingFrames.push_back(frame);

  frame->setActive(true);

  // HACK: When undo-redoing, a view may be registered before
  // a frame is created, in that case the view is added to
  // PendingViews and we assign it to the frame here.
  if (this->Internal->PendingViews.size() > 0)
    {
    pqGenericViewModule* view = this->Internal->PendingViews.takeAt(0);
    this->assignFrame(view);
    }

  // Setup the UI shown when no view is present in the frame.
  QWidget* emptyFrame = frame->emptyMainWidget();
  Ui::EmptyView ui;
  ui.setupUi(emptyFrame);

  this->buildConvertMenu();

  // Add buttons for all conversion actions.
  QList<QAction*> convertActions = 
    this->Internal->ConvertMenu.actions();
  foreach (QAction* action, convertActions)
    {
    QPushButton* button = new QPushButton(action->text(), frame);
    ui.ConvertActionsFrame->layout()->addWidget(button);
    button->addAction(action);
    QObject::connect(button, SIGNAL(clicked()),
      this, SLOT(onConvertToButtonClicked()));
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onFrameRemovedInternal(pqMultiViewFrame* frame)
{
  QObject::disconnect(frame, SIGNAL(dragStart(pqMultiViewFrame*)),
    this, SLOT(frameDragStart(pqMultiViewFrame*)));
  QObject::disconnect(frame, SIGNAL(dragEnter(pqMultiViewFrame*,QDragEnterEvent*)),
    this, SLOT(frameDragEnter(pqMultiViewFrame*,QDragEnterEvent*)));
  QObject::disconnect(frame, SIGNAL(dragMove(pqMultiViewFrame*,QDragMoveEvent*)),
    this, SLOT(frameDragMove(pqMultiViewFrame*,QDragMoveEvent*)));
  QObject::disconnect(frame, SIGNAL(drop(pqMultiViewFrame*,QDropEvent*)),
    this, SLOT(frameDrop(pqMultiViewFrame*,QDropEvent*)));

  frame->removeEventFilter(this);
  this->Internal->PendingFrames.removeAll(frame);
  if (!this->Internal->Frames.contains(frame))
    {
    // A frame with no view module has been removed. 
    return;
    }

  // When a frame is removed, its render module is destroyed.
  pqGenericViewModule* view = this->Internal->Frames.take(frame);
  this->disconnect(frame, view);

  this->Internal->PendingFrames.removeAll(frame);

  // Generally, we destroy the view module when the frame goes away,
  // unless told otherwise.
  if (this->Internal->DontCreateDeleteViewsModules)
    {
    return;
    }

  // When a frame is removed, the contained view is also destroyed.
  if (view)
    {
    pqApplicationCore::instance()->getObjectBuilder()->destroy(view);
    }

}

//-----------------------------------------------------------------------------
void pqViewManager::onFrameRemoved(pqMultiViewFrame* frame)
{
  this->onFrameRemovedInternal(frame);

  if (this->Internal->CloseFrameUndoElement)
    {
    emit this->addToUndoStack(this->Internal->CloseFrameUndoElement);
    this->Internal->CloseFrameUndoElement = 0;
    }
  emit this->endUndo();

  // Now activate some frame, so that we have an active view.
  if (this->Internal->Frames.size() > 0)
    {
    pqMultiViewFrame* new_active_frame =
      this->Internal->Frames.begin().key();
    if (new_active_frame->active())
      {
      this->onActivate(new_active_frame);
      }
    else
      {
      new_active_frame->setActive(true);
      }
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onPreFrameRemoved(pqMultiViewFrame* frame)
{
  emit this->beginUndo("Close View");

  pqMultiView::Index index = this->indexOf(frame);
  pqMultiView::Index parent_index = this->parentIndex(index);

  pqSplitViewUndoElement* elem = pqSplitViewUndoElement::New();
  elem->SplitView(parent_index, 
    this->widgetOrientation(frame), 
    this->widgetSplitRatio(frame), index, true);
  this->Internal->CloseFrameUndoElement = elem;
  elem->Delete();
}

//-----------------------------------------------------------------------------
void pqViewManager::connect(pqMultiViewFrame* frame, pqGenericViewModule* view)
{
  if (!frame || !view)
    {
    return;
    }
  this->Internal->PendingFrames.removeAll(frame);

  QWidget* viewWidget = view->getWidget();
  if(viewWidget)
    {
    viewWidget->setParent(frame);
    frame->setMainWidget(viewWidget);
    viewWidget->installEventFilter(this);
    viewWidget->setMaximumSize(this->Internal->MaxWindowSize);
    }
  else
    {
    frame->setMainWidget(NULL);
    }

  if (view->supportsUndo())
    {
    // Setup undo/redo connections if the view module
    // supports interaction undo.
    frame->BackButton->show();
    frame->ForwardButton->show();

    QObject::connect(frame->BackButton, SIGNAL(pressed()), 
      view, SLOT(undo()));
    QObject::connect(frame->ForwardButton, SIGNAL(pressed()),
      view, SLOT(redo()));
    QObject::connect(view, SIGNAL(canUndoChanged(bool)),
      frame->BackButton, SLOT(setEnabled(bool)));
    QObject::connect(view, SIGNAL(canRedoChanged(bool)),
      frame->ForwardButton, SLOT(setEnabled(bool)));
    }
  else
    {
    frame->BackButton->hide();
    frame->ForwardButton->hide();
    }

  frame->LookmarkButton->show();
  QObject::connect(frame, SIGNAL(createLookmark()),
    this, SIGNAL(createLookmark()));
  frame->LookmarkButton->setEnabled(true);

  this->Internal->Frames.insert(frame, view);
}

//-----------------------------------------------------------------------------
void pqViewManager::disconnect(pqMultiViewFrame* frame, pqGenericViewModule* view)
{
  if (!frame || !view)
    {
    return;
    }

  this->Internal->Frames.remove(frame);

  QWidget* viewWidget = view->getWidget();
  if(viewWidget)
    {
    viewWidget->setParent(NULL);
    viewWidget->removeEventFilter(this);
    }
  frame->setMainWidget(NULL);

  if (view->supportsUndo())
    {
    QObject::disconnect(frame->BackButton, 0, view, 0);
    QObject::disconnect(frame->ForwardButton, 0, view, 0);
    QObject::disconnect(view, 0,frame->BackButton, 0);
    QObject::disconnect(view, 0,frame->ForwardButton, 0);
    }

  QObject::disconnect(frame, SIGNAL(createLookmark()),
    this, SIGNAL(createLookmark()));

  frame->BackButton->hide();
  frame->ForwardButton->hide();
  frame->LookmarkButton->setEnabled(false);

  this->Internal->PendingFrames.push_back(frame);
}

//-----------------------------------------------------------------------------
void pqViewManager::onViewModuleAdded(pqGenericViewModule* view)
{
  if (qobject_cast<pqElementInspectorViewModule*>(view))
    {
    // Ignore element inspector view modules.
    return;
    }
  this->assignFrame(view);
}

//-----------------------------------------------------------------------------
void pqViewManager::assignFrame(pqGenericViewModule* view)
{
  pqMultiViewFrame* frame = 0;
  if (this->Internal->PendingFrames.size() == 0)
    {
    // Create a new frame.
  
    if (this->Internal->UndoStack && (
      this->Internal->UndoStack->getInUndo() ||
      this->Internal->UndoStack->getInRedo()))
      {
      // HACK: If undo-redoing, don't split 
      // to create a new pane, it will be created 
      // as a part of the undo/redo.
      this->Internal->PendingViews.push_back(view);
      return;
      }

    // Locate frame to split.
    // If there is an active view, use it.
    pqMultiViewFrame* oldFrame = 0;
    if (this->Internal->ActiveViewModule)
      {
      oldFrame = this->getFrame(this->Internal->ActiveViewModule);
      }
    else if (this->Internal->Frames.size() > 0)
      {
      oldFrame = this->Internal->Frames.begin().key();
      }
    else
      {
      // There are no pending frames and not used frames, how
      // can that be? Hence, we are use it will happen,
      // atleast flag an error!
      qCritical() << "Internal state of frames has got messed up!";
      return;
      }
  
    this->Internal->DontCreateDeleteViewsModules = true;
    QSize cur_size = oldFrame->size();
    if (cur_size.width() > 1.15*cur_size.height()) 
        // give a slight preference to
        // vertical splitting.
                      
      {
      frame = this->splitWidgetHorizontal(oldFrame);
      }
    else
      {
      frame = this->splitWidgetVertical(oldFrame);
      }
    this->Internal->DontCreateDeleteViewsModules = false;
    }
  else
    {
    // It is possible that the active frame is empty, if so,
    // we use it.
    foreach (pqMultiViewFrame* curframe, this->Internal->PendingFrames)
      {
      if (curframe->active())
        {
        frame = curframe;
        break;
        }
      }
    if (!frame)
      {
      frame = this->Internal->PendingFrames.first();
      }
    this->Internal->PendingFrames.removeAll(frame);
    }

  if (frame)
    {
    this->connect(frame, view);
    if (frame->active())
      {
      this->onActivate(frame);
      }
    else
      {
      frame->setActive(true);
      }
    }
}

//-----------------------------------------------------------------------------
pqMultiViewFrame* pqViewManager::getFrame(pqGenericViewModule* view) const
{
  return qobject_cast<pqMultiViewFrame*>(view->getWidget()->parentWidget());
}

//-----------------------------------------------------------------------------
void pqViewManager::onViewModuleRemoved(pqGenericViewModule* view)
{
  if (qobject_cast<pqElementInspectorViewModule*>(view))
    {
    // Ignore element inspector view modules.
    return;
    }

  pqMultiViewFrame* frame = this->getFrame(view);
  if (frame)
    {
    this->disconnect(frame, view);
    }

  this->Internal->PendingViews.removeAll(view);

  this->onActivate(frame);
}

//-----------------------------------------------------------------------------
void pqViewManager::onConvertToButtonClicked()
{
  QPushButton* button = qobject_cast<QPushButton*>(this->sender());
  if (!button)
    {
    return;
    }

  pqMultiViewFrame* frame = 0;

  // Try to locate the frame in which this button exists.
  QWidget* button_parent = button->parentWidget();
  while (button_parent)
    {
    frame = qobject_cast<pqMultiViewFrame*>(button_parent);
    if (frame)
      {
      break;
      }
    button_parent = button_parent->parentWidget();
    }

  if (!frame)
    {
    return;
    }

  // Make the frame active.
  frame->setActive(true);
  if (button->actions().size() > 0)
    {
    QAction* action = button->actions()[0];
    this->onConvertToTriggered(action);
    }
  else
    {
    qCritical() << "No actions!" << endl;
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onConvertToTriggered(QAction* action)
{
  QString type = action->data().toString();

  // FIXME: We may want to fix this to use the active server instead.
  pqServer* server= pqApplicationCore::instance()->
    getServerManagerModel()->getServerByIndex(0);
  if (!server)
    {
    qDebug() << "No server present cannot convert view.";
    return;
    }

  emit this->beginUndo(QString("Convert View to %1").arg(type));

  pqObjectBuilder* builder = 
    pqApplicationCore::instance()-> getObjectBuilder();
  if (this->Internal->ActiveViewModule)
    {
    builder->destroy(this->Internal->ActiveViewModule);
    }

  builder->createView(type, server);

  emit this->endUndo();
}

//-----------------------------------------------------------------------------
void pqViewManager::onFrameContextMenuRequested(QWidget* wid)
{
  this->buildConvertMenu();

  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(wid);
  if (frame)
    {
    this->updateConversionActions(frame);
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onActivate(QWidget* obj)
{
  if (!obj)
    {
    this->Internal->ActiveViewModule = 0;
    emit this->activeViewModuleChanged(this->Internal->ActiveViewModule);
    return; 
    }

  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(obj);
  if (!frame->active())
    {
    return;
    }


  pqGenericViewModule* view = this->Internal->Frames.value(frame);
  // If the frame does not have a view, view==NULL.
  this->Internal->ActiveViewModule = view;

  // Make sure no other frame is active.
  foreach (pqMultiViewFrame* fr, this->Internal->Frames.keys())
    {
    if (fr != frame)
      {
      fr->setActive(false);
      }
    }
  foreach (pqMultiViewFrame* fr, this->Internal->PendingFrames)
    {
    if (fr != frame)
      {
      fr->setActive(false);
      }
    }

  emit this->activeViewModuleChanged(this->Internal->ActiveViewModule);
}

//-----------------------------------------------------------------------------
bool pqViewManager::eventFilter(QObject* caller, QEvent* e)
{
  if(e->type() == QEvent::MouseButtonPress)
    {
    QVTKWidget* view = qobject_cast<QVTKWidget*>(caller);
    if (view)
      {
      pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(
        view->parentWidget());
      if (frame)
        {
        frame->setActive(true);
        }
      }
    pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(caller);
    if (frame)
      {
      frame->setActive(true);
      }
    }
  else if (e->type() == QEvent::FocusIn)
    {
    QWidget* wdg = qobject_cast<QWidget*>(caller);
    if (wdg)
      {
      pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(
        wdg->parentWidget());
      if (frame)
        {
        frame->setActive(true);
        }
      }
    }
  else if (e->type() == QEvent::Resize)
    {
    // Update WindowPosition and GUISize properties on all view modules.
    this->updateViewModulePositions(); 
    }
  return QObject::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
void pqViewManager::setMaxViewWindowSize(const QSize& win_size)
{
  this->Internal->MaxWindowSize = win_size.isEmpty()?
      QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX) : win_size;
  foreach (pqMultiViewFrame* frame, this->Internal->Frames.keys())
    {
    frame->mainWidget()->setMaximumSize(this->Internal->MaxWindowSize);
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::updateViewModulePositions()
{
  // find a rectangle that bounds all views
  QRect totalBounds;
  
  foreach(pqGenericViewModule* view, this->Internal->Frames)
    {
    QRect bounds = view->getWidget()->rect();
    bounds.moveTo(view->getWidget()->mapToGlobal(QPoint(0,0)));
    totalBounds |= bounds;
    }

  /// GUISize and WindowPosition properties are managed
  /// by the GUI, the undo/redo stack should not worry about 
  /// the changes made to them.
  emit this->beginNonUndoableChanges();

  // Now we loop thorough all view modules and set the GUISize/WindowPosition.
  foreach(pqGenericViewModule* view, this->Internal->Frames)
    {
    vtkSMIntVectorProperty* prop = 0;

    // set size containing all views
    prop = vtkSMIntVectorProperty::SafeDownCast(
      view->getProxy()->GetProperty("GUISize"));
    if(prop)
      {
      prop->SetElements2(totalBounds.width(), totalBounds.height());
      }

    // position relative to the bounds of all views
    prop = vtkSMIntVectorProperty::SafeDownCast(
      view->getProxy()->GetProperty("WindowPosition"));
    if(prop)
      {
      QPoint view_pos = view->getWidget()->mapToGlobal(QPoint(0,0));
      view_pos -= totalBounds.topLeft();
      prop->SetElements2(view_pos.x(), view_pos.y());
      }
    }

  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void pqViewManager::saveState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* rwRoot = vtkPVXMLElement::New();
  rwRoot->SetName("ViewManager");
  rwRoot->AddAttribute("version", PARAVIEW_VERSION_FULL);
  root->AddNestedElement(rwRoot);
  rwRoot->Delete();

  // Save the window layout.
  this->pqMultiView::saveState(rwRoot);

  // Save the render module - window mapping.
  pqInternals::FrameMapType::Iterator iter = this->Internal->Frames.begin();
  for(; iter != this->Internal->Frames.end(); ++iter)
    {
    pqMultiViewFrame* frame = iter.key();
    pqGenericViewModule* view = iter.value();

    pqMultiView::Index index = this->indexOf(frame);
    vtkPVXMLElement* frameElem = vtkPVXMLElement::New();
    frameElem->SetName("Frame");
    frameElem->AddAttribute("index", index.getString().toAscii().data());
    frameElem->AddAttribute("view_module", view->getProxy()->GetSelfIDAsString());
    rwRoot->AddNestedElement(frameElem);
    frameElem->Delete();
    }
}

//-----------------------------------------------------------------------------
bool pqViewManager::loadState(vtkPVXMLElement* rwRoot, 
  vtkSMStateLoader* loader)
{
  if (!rwRoot || !rwRoot->GetName() || strcmp(rwRoot->GetName(), "ViewManager"))
    {
    qDebug() << "Argument must be <ViewManager /> element.";
    return false;
    }

  // When state is loaded by the server manager,
  // the View Manager will have already layed out all the view modules
  // using a default/random scheme. The role of this method
  // is to re-arrange all the views based on the layout in the 
  // state file.
  this->Internal->DontCreateDeleteViewsModules = true; 

  // We remove all "randomly" layed out frames. Note that we are not
  // destroying the view modules, only the frames that got created
  // when the server manager state was getting loaded.
  foreach (pqMultiViewFrame* frame, this->Internal->Frames.keys())
    {
    this->removeWidget(frame);
    }
  this->Superclass::loadState(rwRoot);
  this->Internal->DontCreateDeleteViewsModules = false; 
  
  this->Internal->Frames.clear();
  for(unsigned int cc=0; cc < rwRoot->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* elem = rwRoot->GetNestedElement(cc);
    if (strcmp(elem->GetName(), "Frame") == 0)
      {
      QString index_string = elem->GetAttribute("index");

      pqMultiView::Index index;
      index.setFromString(index_string);
      int id = 0;
      elem->GetScalarAttribute("view_module", &id);
      vtkSmartPointer<vtkSMProxy> viewModule;
      viewModule.TakeReference(loader->NewProxy(id));
      if (!viewModule.GetPointer())
        {
        qCritical() << "Failed to locate view module mentioned in state!";
        return false;
        }

      pqGenericViewModule* view = qobject_cast<pqGenericViewModule*>(
        pqApplicationCore::instance()->getServerManagerModel()->getPQProxy(viewModule));
      pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(
        this->widgetOfIndex(index));
      if (frame && view)
        {
        this->connect(frame, view);
        }
      }
    }
  pqMultiViewFrame* frame = 0;
  if (this->Internal->Frames.size() > 0)
    {
    // Make the first frame active.
    frame = this->Internal->Frames.begin().key();
    }
  else if (this->Internal->PendingFrames.size() > 0)
    {
    frame = this->Internal->PendingFrames[0];
    }

  if (frame)
    {
    if (frame->active())
      {
      this->onActivate(frame);
      }
    else
      {
      frame->setActive(true);
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
void pqViewManager::frameDragStart(pqMultiViewFrame* frame)
{
  QPixmap pixmap(":/pqWidgets/Icons/pqWindow16.png");

  QByteArray output;
  QDataStream dataStream(&output, QIODevice::WriteOnly);
  dataStream << frame->uniqueID();

  QString mimeType = QString("application/paraview3/%1").arg(getpid());

  QMimeData *mimeData = new QMimeData;
  mimeData->setData(mimeType, output);

  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
  drag->setPixmap(pixmap);

  drag->start();
}

//-----------------------------------------------------------------------------
void pqViewManager::frameDragEnter(pqMultiViewFrame*,
                                           QDragEnterEvent* e)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if(e->mimeData()->hasFormat(mimeType))
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::frameDragMove(pqMultiViewFrame*,
                                          QDragMoveEvent* e)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if(e->mimeData()->hasFormat(mimeType))
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::frameDrop(pqMultiViewFrame* acceptingFrame,
                                      QDropEvent* e)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if (e->mimeData()->hasFormat(mimeType))
    {
    QByteArray input= e->mimeData()->data(mimeType);
    QDataStream dataStream(&input, QIODevice::ReadOnly);

    QUuid uniqueID;
    dataStream>>uniqueID;

    pqMultiViewFrame* originatingFrame=NULL;
    pqMultiViewFrame* f;
    foreach(f, this->Internal->Frames.keys())
      {
      if(f->uniqueID()==uniqueID)
        {
        originatingFrame=f;
        break;
        }
      }
    if (!originatingFrame)
      {
      foreach (f, this->Internal->PendingFrames)
        {
        if (f->uniqueID() == uniqueID)
          {
          originatingFrame = f;
          break;
          }
        }
      }
    
    if(originatingFrame && originatingFrame != acceptingFrame)
      {
      this->hide(); 
      //Switch the originalFrame with the frame;

      Index originatingIndex=this->indexOf(originatingFrame);
      Index acceptingIndex=this->indexOf(acceptingFrame);
      pqMultiViewFrame *tempFrame= new pqMultiViewFrame;

      this->replaceView(originatingIndex,tempFrame);
      this->replaceView(acceptingIndex,originatingFrame);
      originatingIndex=this->indexOf(tempFrame);
      this->replaceView(originatingIndex,acceptingFrame);

      this->updateViewModulePositions();
      delete tempFrame;
      
      this->show();
      }
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onSplittingView(const Index& index, 
  Qt::Orientation orientation, float fraction, const Index& childIndex)
{
  emit this->beginUndo("Split View");

  pqSplitViewUndoElement* elem = pqSplitViewUndoElement::New();
  elem->SplitView(index, orientation, fraction, childIndex, false);
  emit this->addToUndoStack(elem);
  elem->Delete();

  emit this->endUndo();
}

