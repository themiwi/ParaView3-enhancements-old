/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqServerManagerSelectionModel.cxx,v $

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
#include "pqServerManagerSelectionModel.h"

#include "vtkCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMOutputPort.h"

#include <QPointer>
#include <QGlobalStatic>

#include "pqServerManagerModel.h"
#include "pqServerManagerModelItem.h"
#include "pqProxy.h"
#include "pqOutputPort.h"

// register meta type for pqSMProxy
static const int pqServerManagerSelectionId = 
qRegisterMetaType<pqServerManagerSelection>("pqServerManagerSelection");

//-----------------------------------------------------------------------------
class pqServerManagerSelectionModelInternal
{
public:
  QPointer<pqServerManagerModel> Model;

  // Both the \c Selection and \c Current are kept synchronized with those
  // maintained by vtkSMProxySelectionModel.
  pqServerManagerSelection Selection;
  QPointer<pqServerManagerModelItem> Current;

  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  vtkSmartPointer<vtkSMProxySelectionModel> ActiveSources;
};

//-----------------------------------------------------------------------------
pqServerManagerSelectionModel::pqServerManagerSelectionModel(
  pqServerManagerModel* _model, QObject* _parent /*=null*/) :QObject(_parent)
{
  this->Internal = new pqServerManagerSelectionModelInternal;
  this->Internal->Model = _model;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxySelectionModel* selmodel = pxm->GetSelectionModel("ActiveSources");
  if (!selmodel)
    {
    selmodel = vtkSMProxySelectionModel::New();
    pxm->RegisterSelectionModel("ActiveSources", selmodel);
    selmodel->Delete();
    }

  this->Internal->ActiveSources = selmodel;
  
  this->Internal->VTKConnect =
    vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Internal->VTKConnect->Connect(selmodel, vtkCommand::CurrentChangedEvent,
    this, SLOT(smCurrentChanged()));
  this->Internal->VTKConnect->Connect(selmodel,
    vtkCommand::SelectionChangedEvent, this, SLOT(smSelectionChanged()));
}

//-----------------------------------------------------------------------------
pqServerManagerSelectionModel::~pqServerManagerSelectionModel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem* pqServerManagerSelectionModel::currentItem() const
{
  return this->Internal->Current;
}

//-----------------------------------------------------------------------------
void pqServerManagerSelectionModel::smCurrentChanged()
{
  pqServerManagerModelItem* item =
    this->Internal->Model->findItem<pqServerManagerModelItem*>(
      this->Internal->ActiveSources->GetCurrentProxy());
  if (item != this->Internal->Current)
    {
    this->Internal->Current = item;
    emit this->currentChanged(item);
    }
}

//-----------------------------------------------------------------------------
void pqServerManagerSelectionModel::smSelectionChanged()
{
  pqServerManagerSelection selected;
  pqServerManagerSelection deselected;

  vtkCollection* collection =
    this->Internal->ActiveSources->GetNewlyDeselected();
  for (int cc=0; cc < collection->GetNumberOfItems(); cc++)
    {
    pqServerManagerModelItem* item =
      this->Internal->Model->findItem<pqServerManagerModelItem*>(
        vtkSMProxy::SafeDownCast(collection->GetItemAsObject(cc)));
    if (item)
      {
      this->Internal->Selection.removeAll(item);
      deselected.push_back(item);
      }
    }

  collection = this->Internal->ActiveSources->GetNewlySelected();
  for (int cc=0; cc < collection->GetNumberOfItems(); cc++)
    {
    pqServerManagerModelItem* item =
      this->Internal->Model->findItem<pqServerManagerModelItem*>(
        vtkSMProxy::SafeDownCast(collection->GetItemAsObject(cc)));
    if (item)
      {
      this->Internal->Selection.push_back(item);
      selected.push_back(item);
      }
    }

  emit this->selectionChanged(selected, deselected);
}

//-----------------------------------------------------------------------------
void pqServerManagerSelectionModel::setCurrentItem(
  pqServerManagerModelItem* item,
  const pqServerManagerSelectionModel::SelectionFlags &command)
{
  if (this->Internal->Current != item)
    {
    this->Internal->Current = item;
    vtkSMProxy* proxy = this->getProxy(item);
    this->Internal->ActiveSources->SetCurrentProxy(proxy,
      this->getCommand(command));
    emit this->currentChanged(item);
    }
}

//-----------------------------------------------------------------------------
pqServerManagerModel* pqServerManagerSelectionModel::model() const
{
  return this->Internal->Model;
}

//-----------------------------------------------------------------------------
bool pqServerManagerSelectionModel::isSelected(
  pqServerManagerModelItem* item) const
{
  return this->Internal->Selection.contains(item);
}

//-----------------------------------------------------------------------------
const pqServerManagerSelection* 
pqServerManagerSelectionModel::selectedItems() const
{
  return &this->Internal->Selection;
}

//-----------------------------------------------------------------------------
void pqServerManagerSelectionModel::select(pqServerManagerModelItem* item,
  const pqServerManagerSelectionModel::SelectionFlags& command)
{
  pqServerManagerSelection sel;
  sel.push_back(item);
  this->select(sel, command);
}

//-----------------------------------------------------------------------------
void pqServerManagerSelectionModel::select(
  const pqServerManagerSelection& items,
  const pqServerManagerSelectionModel::SelectionFlags& command)
{
  if (command == NoUpdate)
    {
    return;
    }

  vtkCollection* proxies = vtkCollection::New();
  foreach(pqServerManagerModelItem* item, items)
    {
    vtkSMProxy* proxy = this->getProxy(item);
    if (proxy)
      {
      proxies->AddItem(proxy);
      }
    }
  this->Internal->ActiveSources->Select(proxies, this->getCommand(command));
  proxies->Delete();
}

//-----------------------------------------------------------------------------
int pqServerManagerSelectionModel::getCommand(
  const pqServerManagerSelectionModel::SelectionFlags &command)
{
  int smcommand =0;
  if (command & NoUpdate)
    {
    smcommand |= vtkSMProxySelectionModel::NO_UPDATE;
    }
  if (command & Clear)
    {
    smcommand |= vtkSMProxySelectionModel::CLEAR;
    }
  if (command & Select)
    {
    smcommand |= vtkSMProxySelectionModel::SELECT;
    }
  if (command & Deselect)
    {
    smcommand |= vtkSMProxySelectionModel::DESELECT;
    }

  return smcommand;
}

//-----------------------------------------------------------------------------
vtkSMProxy*
pqServerManagerSelectionModel::getProxy(pqServerManagerModelItem* item)
{
  pqOutputPort* opport = qobject_cast<pqOutputPort*>(item);
  if (opport)
    {
    return opport->getOutputPortProxy();
    }

  pqProxy* source = qobject_cast<pqProxy*>(item);
  if (source)
    {
    return source->getProxy();
    }
  return 0;
}
