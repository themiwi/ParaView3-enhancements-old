/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqUndoStack.cxx,v $

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
#include "pqUndoStack.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkSmartPointer.h"
#include "vtkSMUndoStack.h"
#include "vtkSMProxyManager.h"


#include <QtDebug>
#include <QPointer>

#include "pqApplicationCore.h"
#include "pqServer.h"
#include "pqUndoRedoStateLoader.h"

//-----------------------------------------------------------------------------
class pqUndoStackImplementation
{
public:
  pqUndoStackImplementation()
    {
    this->NestedCount = 0;
    }
  vtkSmartPointer<vtkSMUndoStack> UndoStack;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnector;
  QPointer<pqServer> Server;
  int NestedCount;
};

//-----------------------------------------------------------------------------
pqUndoStack::pqUndoStack(bool clientOnly, QObject* _parent/*=null*/) 
:QObject(_parent)
{
  this->Implementation = new pqUndoStackImplementation;
  this->Implementation->UndoStack = vtkSmartPointer<vtkSMUndoStack>::New();
  this->Implementation->UndoStack->SetClientOnly(clientOnly);
  pqUndoRedoStateLoader* loader = pqUndoRedoStateLoader::New();
  this->Implementation->UndoStack->SetStateLoader(loader);
  loader->Delete();

  this->Implementation->VTKConnector = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->Implementation->VTKConnector->Connect(this->Implementation->UndoStack,
    vtkCommand::ModifiedEvent, this, SLOT(onStackChanged(vtkObject*, 
        unsigned long, void*, void*, vtkCommand*)), NULL, 1.0);
  this->Implementation->Server = NULL;
}

//-----------------------------------------------------------------------------
pqUndoStack::~pqUndoStack()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
bool pqUndoStack::CanUndo()
{
  return this->Implementation->UndoStack->CanUndo();
}

//-----------------------------------------------------------------------------
bool pqUndoStack::CanRedo()
{
  return this->Implementation->UndoStack->CanRedo();
}

const QString pqUndoStack::UndoLabel()
{
  return this->Implementation->UndoStack->CanUndo() ?
    this->Implementation->UndoStack->GetUndoSetLabel(0) :
    QString();
}

const QString pqUndoStack::RedoLabel()
{
  return this->Implementation->UndoStack->CanRedo() ?
    this->Implementation->UndoStack->GetRedoSetLabel(0) :
    QString();
}

//-----------------------------------------------------------------------------
void pqUndoStack::AddToActiveUndoSet(vtkUndoElement* element)
{
  this->Implementation->UndoStack->AddToActiveUndoSet(element);
}

//-----------------------------------------------------------------------------
void pqUndoStack::onStackChanged(vtkObject*, unsigned long, void*, 
    void*,  vtkCommand*)
{
  bool canUndo = false;
  bool canRedo = false;
  QString undoLabel;
  QString redoLabel;
  if (this->Implementation->UndoStack->CanUndo())
    {
    canUndo = true;
    undoLabel = this->Implementation->UndoStack->GetUndoSetLabel(0);
    }
  if (this->Implementation->UndoStack->CanRedo())
    {
    canRedo = true;
    redoLabel = this->Implementation->UndoStack->GetRedoSetLabel(0);
    }
    
  emit this->StackChanged(canUndo, undoLabel, canRedo, redoLabel);
  emit this->CanUndoChanged(canUndo);
  emit this->CanRedoChanged(canRedo);
  emit this->UndoLabelChanged(undoLabel);
  emit this->RedoLabelChanged(redoLabel);
}

//-----------------------------------------------------------------------------
void pqUndoStack::BeginUndoSet(QString label)
{
  if (!this->Implementation->Server)
    {
    qDebug()<< "no server specified. cannot undo/redo.";
    return;
    }

  if(this->Implementation->NestedCount == 0)
    {
    vtkIdType cid = this->Implementation->Server->GetConnectionID();
    this->Implementation->UndoStack->BeginOrContinueUndoSet(cid,
      label.toStdString().c_str());
    }

  this->Implementation->NestedCount++;
}

//-----------------------------------------------------------------------------
void pqUndoStack::EndUndoSet()
{
  if(this->Implementation->NestedCount == 0)
    {
    qDebug() << "EndUndoSet called without a BeginUndoSet.";
    return;
    }

  this->Implementation->NestedCount--;
  if(this->Implementation->NestedCount == 0)
    {
    this->Implementation->UndoStack->EndUndoSet();
    }
}

//-----------------------------------------------------------------------------
void pqUndoStack::Accept()
{
  this->BeginUndoSet("Accept");
}

//-----------------------------------------------------------------------------
void pqUndoStack::Reset()
{
  this->Implementation->UndoStack->CancelUndoSet();
  if(this->Implementation->NestedCount != 0)
    {
    qDebug() << "Reset called without a closing EndUndoSet.";
    }
  this->Implementation->NestedCount = 0;
}

//-----------------------------------------------------------------------------
void pqUndoStack::Undo()
{
  this->Implementation->UndoStack->Undo();
  // Update of proxies have to happen in order.
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("sources", 1);
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("displays", 1);
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
  pqApplicationCore::instance()->render();
  emit this->Undone();
}

//-----------------------------------------------------------------------------
void pqUndoStack::Redo()
{
  this->Implementation->UndoStack->Redo();
  // Update of proxies have to happen in order.
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("sources", 1);
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies("displays", 1);
  vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
  pqApplicationCore::instance()->render();
  emit this->Redone();
}

//-----------------------------------------------------------------------------
void pqUndoStack::Clear()
{
  this->Implementation->UndoStack->Clear();
}
  
void pqUndoStack::setActiveServer(pqServer* server)
{
  this->Implementation->Server = server;
}

