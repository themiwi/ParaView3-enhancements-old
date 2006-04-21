/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMUndoStack.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUndoStack.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkUndoElement.h"
#include "vtkUndoSet.h"
#include "vtkUndoStackInternal.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyModificationUndoElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyRegisterUndoElement.h"
#include "vtkSMProxyUnRegisterUndoElement.h"
#include "vtkSMUndoRedoStateLoader.h"

//*****************************************************************************
class vtkSMUndoStackObserver : public vtkCommand
{
public:
  static vtkSMUndoStackObserver* New()
    {
    return new vtkSMUndoStackObserver;
    }
  void SetTarget(vtkSMUndoStack* t)
    {
    this->Target = t;
    }
  virtual void Execute(vtkObject* caller, unsigned long eventid, void* data)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(caller, eventid, data);
      }
    }
protected:
  vtkSMUndoStackObserver()
    {
    this->Target = 0;
    }
  ~vtkSMUndoStackObserver()
    {
    this->Target = 0;
    }
  vtkSMUndoStack* Target;
};
//*****************************************************************************
class vtkSMUndoStackUndoSet : public vtkUndoSet
{
public:
  static vtkSMUndoStackUndoSet* New();
  vtkTypeRevisionMacro(vtkSMUndoStackUndoSet, vtkUndoSet);
 
  virtual int Undo() 
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkPVXMLElement* state = pm->NewNextUndo(this->ConnectionID);
    int status=0;
    if (state)
      {
      status = this->UndoRedoManager->ProcessUndo(this->ConnectionID, state);    
      state->Delete();
      }
    return status;
    }
  
  virtual int Redo() 
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkPVXMLElement* state = pm->NewNextRedo(this->ConnectionID);
    int status = 0;
    if (state)
      {
      status = this->UndoRedoManager->ProcessRedo(this->ConnectionID, state);    
      state->Delete();
      }
    return status;
    }

  void SetConnectionID(vtkIdType id)
    {
    this->ConnectionID = id;
    }
  
  vtkIdType GetConnectionID() 
    { 
    return this->ConnectionID; 
    }

  void SetUndoRedoManager(vtkSMUndoStack* r)
    {
    this->UndoRedoManager = r;
    }

protected:
  vtkSMUndoStackUndoSet() 
    {
    this->ConnectionID = vtkProcessModuleConnectionManager::GetNullConnectionID();
    this->UndoRedoManager = 0;
    };
  ~vtkSMUndoStackUndoSet(){ };

  vtkIdType ConnectionID;
  vtkSMUndoStack* UndoRedoManager;
private:
  vtkSMUndoStackUndoSet(const vtkSMUndoStackUndoSet&);
  void operator=(const vtkSMUndoStackUndoSet&);
};

vtkStandardNewMacro(vtkSMUndoStackUndoSet);
vtkCxxRevisionMacro(vtkSMUndoStackUndoSet, "$Revision: 1.5 $");
//*****************************************************************************

vtkStandardNewMacro(vtkSMUndoStack);
vtkCxxRevisionMacro(vtkSMUndoStack, "$Revision: 1.5 $");
vtkCxxSetObjectMacro(vtkSMUndoStack, StateLoader, vtkSMUndoRedoStateLoader);
//-----------------------------------------------------------------------------
vtkSMUndoStack::vtkSMUndoStack()
{
  this->Observer = vtkSMUndoStackObserver::New();
  this->Observer->SetTarget(this);
  this->ActiveUndoSet = NULL;
  this->ActiveConnectionID = vtkProcessModuleConnectionManager::GetNullConnectionID();
  this->Label = NULL;
  this->StateLoader = NULL;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (!pxm)
    {
    vtkErrorMacro("vtkSMUndoStack must be created only"
       << " after the ProxyManager has been created.");
    }
  else
    {
    pxm->AddObserver(vtkCommand::RegisterEvent, this->Observer);
    pxm->AddObserver(vtkCommand::UnRegisterEvent, this->Observer);
    pxm->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer);
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm)
    {
    pm->AddObserver(vtkCommand::ConnectionClosedEvent, this->Observer);
    }
}

//-----------------------------------------------------------------------------
vtkSMUndoStack::~vtkSMUndoStack()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (pxm)
    {
    pxm->RemoveObserver(this->Observer);
    }
  this->Observer->SetTarget(NULL);
  this->Observer->Delete();
  if (this->ActiveUndoSet)
    {
    this->ActiveUndoSet->Delete();
    this->ActiveUndoSet = NULL;
    }
  this->SetLabel(NULL);
  this->SetStateLoader(NULL);
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::BeginUndoSet(vtkIdType cid, const char* label)
{
  if (this->ActiveUndoSet)
    {
    vtkErrorMacro("BeginUndoSet cannot be nested. EndUndoSet must be called "
      << "before calling BeginUndoSet again.");
    return;
    }
  this->ActiveUndoSet = vtkUndoSet::New();
  this->SetLabel(label);
  this->ActiveConnectionID = cid;
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::EndUndoSet()
{
  if (!this->ActiveUndoSet)
    {
    vtkErrorMacro("BeginUndoSet must be called before calling EndUndoSet.");
    return;
    }

  this->Push(this->ActiveConnectionID, this->Label, this->ActiveUndoSet);
  
  this->ActiveUndoSet->Delete();
  this->ActiveUndoSet = NULL;
  this->SetLabel(NULL);
  this->ActiveConnectionID = 
    vtkProcessModuleConnectionManager::GetNullConnectionID();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::Push(vtkIdType cid, const char* label, vtkUndoSet* set)
{
  if (!set)
    {
    vtkErrorMacro("No set sepecified to Push.");
    return;
    }
  
  if (!label)
    {
    vtkErrorMacro("Label is required.");
    return;
    }
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVXMLElement* state = set->SaveState(NULL);
  pm->PushUndo(cid, label, state);
  state->Delete();

  // For now, call this method direcly, eventually, PM may fire and event or something
  // when the undo stack on any connection is updated.
  this->PushUndoConnection(label, cid); 
}

//-----------------------------------------------------------------------------
// TODO: Eventually this method will be called as an effect of the PM telling the client
// that something has been pushed on the server side undo stack.
// As a consequence each client will update their undo stack status. Note,
// only the status is updated, the actual undo state is not sent to the client
// until it requests it. Ofcourse, this part is still not implemnted. For now,
// multiple clients are not supported.
void vtkSMUndoStack::PushUndoConnection(const char* label, 
  vtkIdType id)
{
  vtkSMUndoStackUndoSet* elem = vtkSMUndoStackUndoSet::New();
  elem->SetConnectionID(id);
  elem->SetUndoRedoManager(this);
  this->Superclass::Push(label, elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::ExecuteEvent(vtkObject* vtkNotUsed(caller), 
  unsigned long eventid,  void* data)
{
  switch (eventid)
    {
  case vtkCommand::RegisterEvent:
    if (this->ActiveUndoSet)
      {
      this->OnRegisterProxy(data);
      }
    break;

  case vtkCommand::UnRegisterEvent:
    if (this->ActiveUndoSet)
      {
      this->OnUnRegisterProxy(data);
      }
    break;

  case vtkCommand::PropertyModifiedEvent:
    if (this->ActiveUndoSet)
      {
      this->OnPropertyModified(data);
      }
    break;

  case vtkCommand::ConnectionClosedEvent:
    this->OnConnectionClosed(*reinterpret_cast<vtkIdType*>(data));
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::OnRegisterProxy(void* data)
{
  vtkSMProxyManager::RegisteredProxyInformation &info =*(static_cast<
    vtkSMProxyManager::RegisteredProxyInformation*>(data));

  vtkSMProxyRegisterUndoElement* elem = vtkSMProxyRegisterUndoElement::New();
  elem->SetConnectionID(this->ActiveConnectionID);
  elem->ProxyToRegister(info.GroupName, info.ProxyName, info.Proxy);
  this->ActiveUndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::OnUnRegisterProxy(void* data)
{
  vtkSMProxyManager::RegisteredProxyInformation &info =*(static_cast<
    vtkSMProxyManager::RegisteredProxyInformation*>(data));

  vtkSMProxyUnRegisterUndoElement* elem = 
    vtkSMProxyUnRegisterUndoElement::New();
  elem->SetConnectionID(this->ActiveConnectionID);
  elem->ProxyToUnRegister(info.GroupName, info.ProxyName, info.Proxy);
  this->ActiveUndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::OnPropertyModified(void* data)
{
  vtkSMProxyManager::ModifiedPropertyInformation &info =*(static_cast<
    vtkSMProxyManager::ModifiedPropertyInformation*>(data)); 
 
  vtkSMProperty* prop = info.Proxy->GetProperty(info.PropertyName);
  if (prop && !prop->GetInformationOnly() && !prop->GetIsInternal())
    {
    vtkSMPropertyModificationUndoElement* elem = 
      vtkSMPropertyModificationUndoElement::New();
    elem->ModifiedProperty(info.Proxy, info.PropertyName);
    this->ActiveUndoSet->AddElement(elem);
    elem->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::OnConnectionClosed(vtkIdType cid)
{
  // Connection closed, remove undo/redo elements belonging to the connection.
  vtkUndoStackInternal::VectorOfElements::iterator iter;
  vtkUndoStackInternal::VectorOfElements tempStack;
  
  for (iter = this->Internal->UndoStack.begin();
    iter != this->Internal->UndoStack.end(); ++iter)
    {
    vtkSMUndoStackUndoSet* set = vtkSMUndoStackUndoSet::SafeDownCast(
      iter->UndoSet);
    if (!set || set->GetConnectionID() != cid)
      {
      tempStack.push_back(*iter);
      }
    }
  this->Internal->UndoStack.clear();
  this->Internal->UndoStack.insert(this->Internal->UndoStack.begin(),
    tempStack.begin(), tempStack.end());

  tempStack.clear();
  for (iter = this->Internal->RedoStack.begin();
    iter != this->Internal->RedoStack.end(); ++iter)
    {
     vtkSMUndoStackUndoSet* set = vtkSMUndoStackUndoSet::SafeDownCast(
      iter->UndoSet);
    if (!set || set->GetConnectionID() != cid)
      {
      tempStack.push_back(*iter);
      }
    }
  this->Internal->RedoStack.clear();
  this->Internal->RedoStack.insert(this->Internal->RedoStack.begin(),
    tempStack.begin(), tempStack.end());
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSMUndoStack::ProcessUndo(vtkIdType id, vtkPVXMLElement* root)
{
  if (!this->StateLoader)
    {
    vtkSMUndoRedoStateLoader* sl = vtkSMUndoRedoStateLoader::New();
    this->SetStateLoader(sl);
    sl->Delete();
    }
  this->StateLoader->SetConnectionID(id); 
  vtkUndoSet* undo = this->StateLoader->LoadUndoRedoSet(root);
  int status = 0;
  if (undo)
    {
    status = undo->Undo();
    undo->Delete();
    // Update modified proxies.
    vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
    }
  return status;
}

//-----------------------------------------------------------------------------
int vtkSMUndoStack::ProcessRedo(vtkIdType id, vtkPVXMLElement* root)
{
  if (!this->StateLoader)
    {
    vtkSMUndoRedoStateLoader* sl = vtkSMUndoRedoStateLoader::New();
    this->SetStateLoader(sl);
    sl->Delete();
    }
  this->StateLoader->SetConnectionID(id); 
  vtkUndoSet* redo = this->StateLoader->LoadUndoRedoSet(root);
  int status = 0;
  if (redo)
    {
    status = redo->Redo();
    redo->Delete();
    // Update modified proxies.
    vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
    }
  return status;
}


//-----------------------------------------------------------------------------
int vtkSMUndoStack::Undo()
{
  if (!this->CanUndo())
    {
    vtkErrorMacro("Cannot undo. Nothing on undo stack.");
    return 0;
    }
  return this->Superclass::Undo();
}

//-----------------------------------------------------------------------------
int vtkSMUndoStack::Redo()
{
  if (!this->CanRedo())
    {
    vtkErrorMacro("Cannot redo. Nothing on redo stack.");
    return 0;
    }
  return this->Superclass::Redo();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent <<"StateLoader: " << this->StateLoader << endl;
}

