/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqPipelineBuilder.cxx,v $

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

========================================================================*/

#include "pqPipelineBuilder.h"

// Qt includes.
#include <QtDebug>
#include <QString>

// vtk includes

// Paraview Server Manager includes
#include "vtkProcessModule.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMMultiViewRenderModuleProxy.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"

// ParaView includes
#include "pqApplicationCore.h"
#include "pqDisplayPolicy.h"
#include "pqNameCount.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqPlotViewModule.h"
#include "pqRenderViewModule.h"
#include "pqScalarBarDisplay.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqTableViewModule.h"
#include "pqUndoStack.h"

#include <assert.h>

//-----------------------------------------------------------------------------
pqPipelineBuilder* pqPipelineBuilder::Instance = 0;
pqPipelineBuilder* pqPipelineBuilder::instance()
{
  return pqPipelineBuilder::Instance;
}

//-----------------------------------------------------------------------------
pqPipelineBuilder::pqPipelineBuilder(QObject* p/*=0*/):
  QObject(p)
{
  this->NameGenerator = new pqNameCount(); 
  this->UndoStack = 0;
  if (!pqPipelineBuilder::Instance)
    {
    pqPipelineBuilder::Instance = this;
    }
}

//-----------------------------------------------------------------------------
pqPipelineBuilder::~pqPipelineBuilder()
{
  delete this->NameGenerator;
  this->UndoStack = 0;
  if (pqPipelineBuilder::Instance == this)
    {
    pqPipelineBuilder::Instance = 0;
    }
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqPipelineBuilder::createSource(const char* xmlgroup,
    const char* xmlname, pqServer* server)
{
  vtkSMProxy* proxy = this->createPipelineProxy(xmlgroup, xmlname, server);
  if (proxy)
    {
    return pqServerManagerModel::instance()->getPQSource(proxy);
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::addConnection(pqPipelineSource* source, 
  pqPipelineSource* sink)
{
  if(!source || !sink)
    {
    qCritical() << "Cannot addConnection. source or sink missing.";
    return;
    }

  if (this->UndoStack)
    {
    this->UndoStack->BeginUndoSet(QString("Add Connection"));
    }

  vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(
    sink->getProxy()->GetProperty("Input"));
  if(inputProp)
    {
    // If the sink already has an input, the previous connection
    // needs to be broken if it doesn't support multiple inputs.
    if(!inputProp->GetMultipleInput() && inputProp->GetNumberOfProxies() > 0)
      {
      inputProp->RemoveAllProxies();
      }

    // Add the input to the proxy in the server manager.
    inputProp->AddProxy(source->getProxy());
    }
  else
    {
    qCritical() << "Failed to locate property Input on proxy:" 
      << source->getProxy()->GetXMLGroup() << ", " << source->getProxy()->GetXMLName();
    }

  if (this->UndoStack)
    {
    this->UndoStack->EndUndoSet();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::removeConnection(pqPipelineSource* pqsource,
  pqPipelineSource* pqsink)
{
  vtkSMCompoundProxy *compoundProxy =
    vtkSMCompoundProxy::SafeDownCast(pqsource->getProxy());

  vtkSMProxy* source = pqsource->getProxy();
  vtkSMProxy* sink = pqsink->getProxy();

  if(compoundProxy)
    {
    // TODO: How to find the correct output proxy?
    source = 0;
    for(int i = compoundProxy->GetNumberOfProxies(); source == 0 && i > 0; i--)
      {
      source = vtkSMSourceProxy::SafeDownCast(compoundProxy->GetProxy(i-1));
      }
    }

  compoundProxy = vtkSMCompoundProxy::SafeDownCast(sink);
  if(compoundProxy)
    {
    // TODO: How to find the correct input proxy?
    sink = compoundProxy->GetMainProxy();
    }

  if(!source || !sink)
    {
    qCritical() << "Cannot removeConnection. source or sink missing.";
    return;
    }

  if (this->UndoStack)
    {
    this->UndoStack->BeginUndoSet(QString("Remove Connection"));
    }

  vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(
    sink->GetProperty("Input"));
  if(inputProp)
    {
    // Remove the input from the server manager.
    inputProp->RemoveProxy(source);
    }

  if (this->UndoStack)
    {
    this->UndoStack->EndUndoSet();
    }
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineBuilder::createProxy(const char* xmlgroup, 
  const char* xmlname, const char* register_group, pqServer* server,
  bool is_undoable/*=true*/)
{
  if(!server)
    {
    qDebug() << "Cannot create proxy, no server specified.";
    return NULL;
    }

  if (!register_group)
    {
    qDebug() << "register_group cannot be null.";
    return NULL;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* proxy = NULL;
  if (xmlgroup)
    {
    proxy = pxm->NewProxy(xmlgroup, xmlname);
    }
  else
    {
    proxy = pxm->NewCompoundProxy(xmlname);
    }
  if (!proxy)
    {
    qCritical() << "Failed to create proxy: " 
      << (xmlgroup? xmlgroup: "") << "," << xmlname;
    return NULL;
    }
  proxy->SetConnectionID(server->GetConnectionID());

  if (this->UndoStack && is_undoable)
    {
    QString label = QString("Create %1").arg(xmlname);
    this->UndoStack->BeginUndoSet(label);
    }

  QString name = QString("%1%2");
  name = name.arg(proxy->GetXMLName());
  name = name.arg(this->NameGenerator->GetCountAndIncrement(proxy->GetXMLName()));

  pxm->RegisterProxy(register_group, name.toAscii().data(), proxy);
  proxy->Delete();

  if (this->UndoStack && is_undoable)
    {
    this->UndoStack->EndUndoSet();
    }
  return proxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineBuilder::createPipelineProxy(const char* xmlgroup,
    const char* xmlname, pqServer* server)
{
  vtkSMProxy* proxy = this->createProxy(xmlgroup, xmlname, "sources",
    server, true);
  return proxy;
}

//-----------------------------------------------------------------------------
pqConsumerDisplay* pqPipelineBuilder::createDisplay(pqPipelineSource* src,
  pqGenericViewModule* viewModule)
{
  if (!src || !viewModule )
    {
    qCritical() <<"Missing required attribute.";
    return NULL;
    }

  vtkSMProxy *proxy = src->getProxy();
  if (!proxy)
    {
    qDebug() << "Failed to locate proxy to connect display to.";
    return NULL;
    }

  if (this->UndoStack)
    {
    QString label("Display %1");
    label = label.arg(proxy->GetXMLName()?  proxy->GetXMLName() : "" );
    this->UndoStack->BeginUndoSet(label);
    }
  pqConsumerDisplay* display = 
    this->createDisplayProxyInternal(src , viewModule);

  if (this->UndoStack)
    {
    this->UndoStack->EndUndoSet();
    }

  return display;
}

//-----------------------------------------------------------------------------
pqConsumerDisplay* pqPipelineBuilder::createDisplayProxyInternal(
  pqPipelineSource* src, pqGenericViewModule* view)
{
  pqDisplayPolicy* policy = pqApplicationCore::instance()->getDisplayPolicy();
  vtkSMProxy* display = policy? policy->newDisplay(src, view) : 0; 
  if (!display)
    {
    qDebug() << "Cannot create display. "
      <<"Make sure that your Display Policy is initialized properly.";
    return NULL;
    }

  // Register the proxy -- must be done first before any property changes 
  // (of undo/redo to work).
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  pxm->RegisterProxy("displays", display->GetSelfIDAsString(), display);
  display->Delete();

  vtkSMProxy* proxy = src->getProxy();
  vtkSMProxy* viewModuleProxy = view->getProxy();

  // Set the display proxy input.
  pqSMAdaptor::setProxyProperty(display->GetProperty("Input"), proxy);
  display->UpdateVTKObjects();

  // Add the display proxy to render module.
  pqSMAdaptor::addProxyProperty(viewModuleProxy->GetProperty("Displays"), display);
  viewModuleProxy->UpdateVTKObjects();

  pqConsumerDisplay* dispObject = 
    pqServerManagerModel::instance()->getPQDisplay(display);
  dispObject->setDefaults();
  return dispObject;
}

//-----------------------------------------------------------------------------
int pqPipelineBuilder::removeInternal(pqDisplay* display)
{
  if (!display)
    {
    qDebug() << "Cannot remove null display.";
    return 0;
    }

  // 1) Remove display from the render module.
  // eventually, the pqPipelineDisplay can tell us which render module
  // it belongs to. For now, we just use the active render module.
  unsigned int numRenModules = display->getNumberOfViewModules();
  for(unsigned int i=0; i<numRenModules; i++)
    {
    pqGenericViewModule* renModule = display->getViewModule(i);
    
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      renModule->getProxy()->GetProperty("Displays"));
    pp->RemoveProxy(display->getProxy());
    renModule->getProxy()->UpdateVTKObjects();
    }

  // Unregister display.
  vtkSMProxyManager::GetProxyManager()->UnRegisterProxy(
    display->getSMGroup().toAscii().data(), 
    display->getSMName().toAscii().data(),
    display->getProxy());
  return 1;
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::remove(pqPipelineDisplay* display, 
  bool is_undoable/*=true*/)
{
  if (is_undoable && this->UndoStack)
    {
    this->UndoStack->BeginUndoSet(QString("Remove Display"));
    }

  this->removeInternal(display);

  if (is_undoable && this->UndoStack)
    {
    this->UndoStack->EndUndoSet();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::remove(pqPipelineSource* source,
  bool is_undoable/*=false*/)
{
  if (!source)
    {
    qDebug() << "Cannot remove null source.";
    return;
    }

  if (source->getNumberOfConsumers())
    {
    qDebug() << "Cannot remove source with consumers.";
    return;
    }

  if (this->UndoStack && is_undoable)
    {
    this->UndoStack->BeginUndoSet(QString("Remove Source"));
    }



  // 2) remove inputs.
  // TODO: this step should not be necessary, but it currently
  // is :(. Needs some looking into.
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    source->getProxy()->GetProperty("Input"));
  if (pp)
    {
    pp->RemoveAllProxies();
    }

  // 1) remove all displays.
  while (source->getDisplayCount())
    {
    if (!this->removeInternal(source->getDisplay(0)))
      {
      qDebug() << "Failed to remove display!";
      return;
      }
    }

  // 3) Unregister proxy.
  vtkSMProxyManager::GetProxyManager()->UnRegisterProxy(
    source->getSMGroup().toAscii().data(), 
    source->getSMName().toAscii().data(),
    source->getProxy());

  if (this->UndoStack && is_undoable)
    {
    this->UndoStack->EndUndoSet();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::remove(pqProxy* proxy, bool is_undoable)
{
  if (this->UndoStack && is_undoable)
    {
    this->UndoStack->BeginUndoSet(QString("Remove proxy"));
    }

  vtkSMProxyManager::GetProxyManager()->UnRegisterProxy(
    proxy->getSMGroup().toAscii().data(), 
    proxy->getSMName().toAscii().data(),
    proxy->getProxy());

  if (this->UndoStack && is_undoable)
    {
    this->UndoStack->EndUndoSet();
    }
}

//-----------------------------------------------------------------------------
pqGenericViewModule* pqPipelineBuilder::createView(int type, pqServer* server)
{
  if (!server)
    {
    qDebug() << "Cannot create view without server.";
    return NULL;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* proxy= 0;
  switch (type)
    {
  case pqGenericViewModule::RENDER_VIEW:
    proxy = server->newRenderModule();
    break;

  case pqGenericViewModule::XY_PLOT:
    proxy = pxm->NewProxy("plotmodules", "XYPlotViewModule");
    break;

  case pqGenericViewModule::BAR_CHART:
    proxy = pxm->NewProxy("plotmodules","BarChartViewModule");
    break;

  case pqGenericViewModule::TABLE_VIEW:
    proxy = pxm->NewProxy("views", "TableView");
    break;

  default:
    qDebug() << "Unknown view type : " << type;
    }

  if (!proxy)
    {
    qDebug() << "Failed to create a proxy for the requested view type.";
    return NULL;
    }
  proxy->SetConnectionID(server->GetConnectionID());

  QString name = QString("%1%2").arg(proxy->GetXMLName()).arg(
    this->NameGenerator->GetCountAndIncrement(proxy->GetXMLName()));
  pxm->RegisterProxy("view_modules", name.toAscii().data(), proxy);
  proxy->Delete();

  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqGenericViewModule* view = qobject_cast<pqGenericViewModule*>(
    model->getPQProxy(proxy));
  if (view)
    {
    view->setDefaults();
    }
  else
    {
    qDebug() << "Cannot locate the pqGenericViewModule for the " 
      << "view module proxy.";
    }
  
  return view;
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::removeView(pqGenericViewModule* view)
{
  if (!view)
    {
    qDebug() << "Nothing to remove.";
    return;
    }

  // Get a list of all displays belonging to this render module. We delete
  // all the displays that belong only to this render module.
  QList<pqDisplay*> displays = view->getDisplays();

  // Unregister the proxy....the rest of the GUI will(rather should) manage itself!
  QString name = view->getSMName();
  vtkSMProxy* proxy = view->getProxy();

  if (qobject_cast<pqRenderViewModule*>(view))
    {
    vtkSMMultiViewRenderModuleProxy* multiRM = view->getServer()->GetRenderModule();  

    // This need to be done since multiRM adds all created rendermodules to itself.
    // This may need revisiting once we fully support multi-view.
    // This removal is necessary,as otherwise the vtkSMRenderModuleProxy lingers
    // after this call -- which is not good, since the vtkSMRenderModuleProxy
    // is as such not useful.
    for (unsigned int cc=0; cc < multiRM->GetNumberOfProxies(); cc++)
      {
      if (multiRM->GetProxy(cc) == proxy)
        {
        multiRM->RemoveProxy(multiRM->GetProxyName(cc));
        break;
        }
      }
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->UnRegisterProxy(view->getSMGroup().toAscii().data(), name.toAscii().data(), proxy);
  // view is invalid at this point.
 
  // Now clean up any orphan displays.
  foreach (pqDisplay* disp, displays)
    {
    if (disp && disp->getNumberOfViewModules() == 0)
      {
      this->removeInternal(disp);      
      }
    }
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineBuilder::createLookupTable(pqPipelineDisplay* display)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  
  vtkSMProxy* lut = pxm->NewProxy("lookup_tables", "LookupTable");
  lut->SetConnectionID(display->getProxy()->GetConnectionID());
  lut->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  
  // register it.
  pxm->RegisterProxy("lookup_tables", lut->GetSelfIDAsString(), lut);
  lut->Delete();

  // LUT must go from blue to red.
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    lut->GetProperty("HueRange"));
  dvp->SetElement(0, 0.6667);
  dvp->SetElement(1, 0.0);
  lut->UpdateVTKObjects();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    display->getProxy()->GetProperty("LookupTable"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(lut);
    }

  return lut;
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::deleteProxies(pqServer* server)
{
  if (!server)
    {
    qDebug() << "Server cannot be NULL.";
    return;
    }
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->UnRegisterProxies(server->GetConnectionID());
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::getSupportedProxies(const QString& xmlgroup, 
  pqServer* vtkNotUsed(server), QList<QString>& names)
{
  names.clear();
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  unsigned int numProxies = pxm->GetNumberOfXMLProxies(
    xmlgroup.toAscii().data());
  for (unsigned int cc=0; cc <numProxies; cc++)
    {
    const char* name = pxm->GetXMLProxyName(xmlgroup.toAscii().data(),
      cc);
    if (name)
      {
      names.push_back(name);
      }
    }
}

//-----------------------------------------------------------------------------
pqScalarBarDisplay* pqPipelineBuilder::createScalarBar(pqScalarsToColors *lut,
  pqRenderViewModule* renModule)
{
  if (!lut && !renModule)
    {
    qDebug() << "Both lut and renModule cannot be null. Cannot create scalar bar.";
    return 0;
    }

  if (this->UndoStack)
    {
    this->UndoStack->BeginUndoSet("Create ScalarBar");
    }
  pqServer* server = (lut? lut->getServer() : renModule->getServer());
  vtkSMProxy* scalarBarProxy = this->createProxy("displays", "ScalarBarWidget",
    "scalar_bars", server, true);
  pqScalarBarDisplay* scalarBar = 0;

  if (scalarBarProxy)
    {
    scalarBar = qobject_cast<pqScalarBarDisplay*>(
      pqServerManagerModel::instance()->getPQProxy(scalarBarProxy));
    if (scalarBar && lut)
      {
      pqSMAdaptor::setProxyProperty(scalarBarProxy->GetProperty("LookupTable"),
        lut->getProxy());
      scalarBarProxy->UpdateVTKObjects();
      }
    if (scalarBar && renModule)
      {
      pqSMAdaptor::addProxyProperty(renModule->getProxy()->GetProperty("Displays"),
        scalarBarProxy);
      renModule->getProxy()->UpdateVTKObjects();
      }
    }

  if (this->UndoStack)
    {
    this->UndoStack->EndUndoSet();
    } 
  return scalarBar;
}
