/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqFiltersMenuManager.cxx,v $

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
#include "pqFiltersMenuManager.h"

// Server Manager Includes.
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMInputProperty.h"

// Qt Includes.
#include <QMenu>

// ParaView Includes.
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqApplicationCore.h"
#include "pqServerManagerSelectionModel.h"
#include "pqServer.h"
//-----------------------------------------------------------------------------
pqFiltersMenuManager::pqFiltersMenuManager(QMenu* _menu): Superclass(_menu)
{
}

//-----------------------------------------------------------------------------
pqFiltersMenuManager::~pqFiltersMenuManager()
{
}

//-----------------------------------------------------------------------------
void pqFiltersMenuManager::updateEnableState()
{
  // Get the list of selected sources. Make sure the list contains
  // only valid sources.
  const pqServerManagerSelection *selected =
      pqApplicationCore::instance()->getSelectionModel()->selectedItems();
  
  QList<pqOutputPort*> outputPorts;
  pqServerManagerModelItem* item = NULL;
  pqServerManagerSelection::ConstIterator iter = selected->begin();
  for( ; iter != selected->end(); ++iter)
    {
    item = *iter;
    pqPipelineSource* source = qobject_cast<pqPipelineSource *>(item);
    pqOutputPort* port = source? source->getOutputPort(0) : 
      qobject_cast<pqOutputPort*>(item);
    if (port)
      {
      outputPorts.append(port);
      }
    }

  // TODO: Add support to get the supported proxies from the server using 
  // pqServer::getSupportedProxies() and only enable those proxies that are
  // present on the current server.


  // Iterate over all filters in the menu and see if they can be
  // applied to the current source(s).
  bool some_enabled = false;
  vtkSMProxyManager *pxm = vtkSMProxyManager::GetProxyManager();
  QList<QAction *> menu_actions = this->menu()->findChildren<QAction *>();
  foreach( QAction* action, menu_actions)
    {
    QString filterName = action->data().toString();
    if (filterName.isEmpty())
      {
      continue;
      }
    action->setEnabled(false);
    if (outputPorts.size() == 0)
      {
      continue;
      }

    vtkSMProxy* output = pxm->GetPrototypeProxy(
      this->xmlGroup().toAscii().data(),
      filterName.toAscii().data());
    if (!output)
      {
      continue;
      }

    int numProcs = outputPorts[0]->getServer()->getNumberOfPartitions();
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(output);
    if (sp &&
        (sp->GetProcessSupport() == vtkSMSourceProxy::SINGLE_PROCESS && numProcs > 1) ||
        (sp->GetProcessSupport() == vtkSMSourceProxy::MULTIPLE_PROCESSES && numProcs == 1))
      {
      // Skip single process filters when running in multiprocesses and vice
      // versa.
      continue;
      }
        
    // TODO: Handle case where a proxy has multiple input properties.
    vtkSMInputProperty *input = vtkSMInputProperty::SafeDownCast(
      output->GetProperty("Input"));
    if(input)
      {
      if(!input->GetMultipleInput() && selected->size() > 1)
        {
        continue;
        }

      input->RemoveAllUncheckedProxies();
      foreach(pqOutputPort* port, outputPorts)
        {
        input->AddUncheckedInputConnection(
          port->getSource()->getProxy(), port->getPortNumber());
        }

      if(input->IsInDomains())
        {
        action->setEnabled(true);
        some_enabled = true;
        }
      input->RemoveAllUncheckedProxies();
      }
    }

  this->menu()->setEnabled(some_enabled);
}
