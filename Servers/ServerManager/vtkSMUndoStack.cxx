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

#include <vtksys/RegularExpression.hxx>

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
    int status=0;
    vtkPVXMLElement* state;
    if (this->State)
      {
      state = this->State;
      state->Register(this);
      }
    else
      {
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      state = pm->NewNextUndo(this->ConnectionID);
      }
    if (state)
      {
      status = this->UndoRedoManager->ProcessUndo(this->ConnectionID, state);    
      state->Delete();
      }
    return status;
    }
  
  virtual int Redo() 
    {
    int status = 0;
    vtkPVXMLElement* state;
    if (this->State)
      {
      state = this->State;
      state->Register(this);
      }
    else
      {
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      state = pm->NewNextRedo(this->ConnectionID);
      }
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

  void SetState(vtkPVXMLElement* elem)
    {
    this->State = elem;
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

  // State is set for Client side only elements. If state is NULL, then and then 
  // alone an attempt is made to obtain the state from the server.
  vtkSmartPointer<vtkPVXMLElement> State;
private:
  vtkSMUndoStackUndoSet(const vtkSMUndoStackUndoSet&);
  void operator=(const vtkSMUndoStackUndoSet&);
};

vtkStandardNewMacro(vtkSMUndoStackUndoSet);
vtkCxxRevisionMacro(vtkSMUndoStackUndoSet, "$Revision: 1.11 $");
//*****************************************************************************

vtkStandardNewMacro(vtkSMUndoStack);
vtkCxxRevisionMacro(vtkSMUndoStack, "$Revision: 1.11 $");
vtkCxxSetObjectMacro(vtkSMUndoStack, StateLoader, vtkSMUndoRedoStateLoader);
//-----------------------------------------------------------------------------
vtkSMUndoStack::vtkSMUndoStack()
{
  this->ClientOnly = 0;
  this->Observer = vtkSMUndoStackObserver::New();
  this->Observer->SetTarget(this);
  this->ActiveUndoSet = NULL;
  this->ActiveConnectionID = vtkProcessModuleConnectionManager::GetNullConnectionID();
  this->Label = NULL;
  this->StateLoader = NULL;
  this->BuildUndoSet = 0;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (!pxm)
    {
    vtkErrorMacro("vtkSMUndoStack must be created only"
       << " after the ProxyManager has been created.");
    }
  else
    {
    // It is essential that the Undo/Redo system notices these events
    // before anyone else, hence we put these observers on a high priority level.
    pxm->AddObserver(vtkCommand::RegisterEvent, this->Observer, 100);
    pxm->AddObserver(vtkCommand::UnRegisterEvent, this->Observer, 100);
    pxm->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer, 100);
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
void vtkSMUndoStack::BeginOrContinueUndoSet(vtkIdType cid, const char* label)
{
  if (this->ActiveUndoSet && this->ActiveConnectionID != cid)
    {
    vtkWarningMacro("Connection ID has changed, cannot continue "
      << "with the active undo set. Starting a new one.");
    this->EndUndoSet();
    }

  if (!this->ActiveUndoSet)
    {
    this->ActiveUndoSet = vtkUndoSet::New();
    this->SetLabel(label);
    this->ActiveConnectionID = cid;
    }
  this->BuildUndoSet = 1;
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::PauseUndoSet()
{
  if (!this->BuildUndoSet)
    {
    vtkWarningMacro("No undo set active.");
    }
  this->BuildUndoSet = 0;
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::EndUndoSet()
{
  if (!this->ActiveUndoSet)
    {
    vtkErrorMacro("BeginOrContinueUndoSet must be called before calling EndUndoSet.");
    return;
    }

  // We add an undo set to the stack only if it has some elements.
  if (this->ActiveUndoSet->GetNumberOfElements() > 0)
    {
    this->Push(this->ActiveConnectionID, this->Label, this->ActiveUndoSet);
    }
  this->CancelUndoSet();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::CancelUndoSet()
{
  if (this->ActiveUndoSet)
    {
    this->ActiveUndoSet->Delete();
    this->ActiveUndoSet = NULL;
    this->SetLabel(NULL);
    this->ActiveConnectionID = 
      vtkProcessModuleConnectionManager::GetNullConnectionID();
    this->BuildUndoSet = 0;
    }
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
  
  vtkPVXMLElement* state = set->SaveState(NULL);
  // if (!this->ClientOnly)
  //   state->PrintXML();
  if (this->ClientOnly)
    {
    vtkSMUndoStackUndoSet* elem = vtkSMUndoStackUndoSet::New();
    elem->SetConnectionID(cid);
    elem->SetUndoRedoManager(this);
    elem->SetState(state);
    this->Superclass::Push(label, elem);
    elem->Delete();
    }
  else
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->PushUndo(cid, label, state);

    // For now, call this method direcly, eventually, PM may fire and event or something
    // when the undo stack on any connection is updated.
    this->PushUndoConnection(label, cid); 
    }
  state->Delete();
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
    if (this->BuildUndoSet)
      {
      this->OnRegisterProxy(data);
      }
    break;

  case vtkCommand::UnRegisterEvent:
    if (this->BuildUndoSet)
      {
      this->OnUnRegisterProxy(data);
      }
    break;

  case vtkCommand::PropertyModifiedEvent:
    if (this->BuildUndoSet)
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
void vtkSMUndoStack::AddToActiveUndoSet(vtkUndoElement* element)
{
  if (!this->ActiveUndoSet)
    {
    vtkErrorMacro("No active UndoSet, cannot add the element to it.");
    return;
    }
  if (!element)
    {
    return;
    }
  this->ActiveUndoSet->AddElement(element);
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::OnRegisterProxy(void* data)
{
  // proxies registered as prototypes don't participate in
  // undo/redo.
  vtksys::RegularExpression prototypesRe("_prototypes$");

  vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
    vtkSMProxyManager::RegisteredProxyInformation*>(data));
  if (prototypesRe.find(info.GroupName) != 0)
    {
    return;
    }

  vtkSMProxyRegisterUndoElement* elem = vtkSMProxyRegisterUndoElement::New();
  elem->SetConnectionID(this->ActiveConnectionID);
  elem->ProxyToRegister(info.GroupName, info.ProxyName, info.Proxy);
  this->ActiveUndoSet->AddElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::OnUnRegisterProxy(void* data)
{
  // proxies registered as prototypes don't participate in
  // undo/redo.
  vtksys::RegularExpression prototypesRe("_prototypes$");

  vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
    vtkSMProxyManager::RegisteredProxyInformation*>(data));
  if (prototypesRe.find(info.GroupName) != 0)
    {
    return;
    }
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
  // TODO: We need to determine if the property is being changed on a proxy 
  // that is registered only as a prototype. If so, we should not worry
  // about recording its property changes. When we update the SM data structure
  // to separately manage prototypes, this will be take care of automatically.
  // Hence, we defer it for now.
  vtkSMProxyManager::ModifiedPropertyInformation &info =*(reinterpret_cast<
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
    // vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
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
    // vtkSMProxyManager::GetProxyManager()->UpdateRegisteredProxies(1);
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
  if (this->ActiveUndoSet)
    {
    this->ActiveUndoSet->Undo();
    this->CancelUndoSet();
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

  // before redoing, get rid of the half done state.
  if (this->ActiveUndoSet)
    {
    this->ActiveUndoSet->Undo();
    this->CancelUndoSet();
    }

  return this->Superclass::Redo();
}

//-----------------------------------------------------------------------------
void vtkSMUndoStack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "StateLoader: " << this->StateLoader << endl;
  os << indent << "ClientOnly: " << this->ClientOnly << endl;
}

