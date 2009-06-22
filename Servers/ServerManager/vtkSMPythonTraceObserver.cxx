/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPythonTraceObserver.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPythonTraceObserver.h"
#include "vtkSMProxyManager.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
class vtkSMPythonTraceObserverCommand : public vtkCommand
{
public:
  static vtkSMPythonTraceObserverCommand* New()
    { 
    return new vtkSMPythonTraceObserverCommand; 
    }
  
  void SetTarget(vtkSMPythonTraceObserver* t)
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

private:
  vtkSMPythonTraceObserverCommand()
    {
    this->Target = 0;
    }

  vtkSMPythonTraceObserver* Target;
};

//-----------------------------------------------------------------------------
class vtkSMPythonTraceObserver::vtkInternal {
public:

  vtkSMProxyManager::RegisteredProxyInformation LastRegisterProxyInfo;
  vtkSMProxyManager::RegisteredProxyInformation LastUnRegisterProxyInfo;
  vtkSMProxyManager::ModifiedPropertyInformation LastModifiedPropertyInfo;
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPythonTraceObserver);
vtkCxxRevisionMacro(vtkSMPythonTraceObserver, "$Revision: 1.2 $");

//-----------------------------------------------------------------------------
vtkSMPythonTraceObserver::vtkSMPythonTraceObserver()
{
  this->Internal = new vtkInternal;

  this->Observer = vtkSMPythonTraceObserverCommand::New();
  this->Observer->SetTarget(this);

  // Get the proxy manager
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (!pxm)
    {
    vtkErrorMacro("vtkSMPythonTraceObserver must be created only"
       << " after the ProxyManager has been created.");
    }
  else
    {
    // Be notified on these proxy manager events
    pxm->AddObserver(vtkCommand::RegisterEvent, this->Observer);
    pxm->AddObserver(vtkCommand::UnRegisterEvent, this->Observer);
    pxm->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer);
    pxm->AddObserver(vtkCommand::UpdateInformationEvent, this->Observer);
    }
}

//-----------------------------------------------------------------------------
vtkSMPythonTraceObserver::~vtkSMPythonTraceObserver()
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (pxm)
    {
    pxm->RemoveObserver(this->Observer);
    }

  this->Observer->SetTarget(NULL);
  this->Observer->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkSMPythonTraceObserver::ExecuteEvent(vtkObject* vtkNotUsed(caller), 
  unsigned long eventid, void* data)
{
  switch (eventid)
    {
    case vtkCommand::RegisterEvent:
      {
      vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
        vtkSMProxyManager::RegisteredProxyInformation*>(data));

      if (info.Type == vtkSMProxyManager::RegisteredProxyInformation::PROXY)
        {
        this->Internal->LastRegisterProxyInfo = info;
        this->InvokeEvent(vtkCommand::RegisterEvent);
        }
      }
    break;

    case vtkCommand::UnRegisterEvent:
      {
      vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
        vtkSMProxyManager::RegisteredProxyInformation*>(data));

      if (info.Type == vtkSMProxyManager::RegisteredProxyInformation::PROXY)
        {
        this->Internal->LastUnRegisterProxyInfo = info;
        this->InvokeEvent(vtkCommand::UnRegisterEvent);
        }
      }
    break;

    case vtkCommand::PropertyModifiedEvent:
      {
      vtkSMProxyManager::ModifiedPropertyInformation &info =*(reinterpret_cast<
        vtkSMProxyManager::ModifiedPropertyInformation*>(data));
      this->Internal->LastModifiedPropertyInfo = info;
      this->InvokeEvent(vtkCommand::PropertyModifiedEvent);
      }
    break;

    case vtkCommand::UpdateInformationEvent:
      {
      this->InvokeEvent(vtkCommand::UpdateInformationEvent);
      }
    break;
    }
}


//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPythonTraceObserver::GetLastPropertyModifiedProxy()
{
  return this->Internal->LastModifiedPropertyInfo.Proxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPythonTraceObserver::GetLastProxyRegistered()
{
  return this->Internal->LastRegisterProxyInfo.Proxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPythonTraceObserver::GetLastProxyUnRegistered()
{
  return this->Internal->LastUnRegisterProxyInfo.Proxy;
}

//-----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastPropertyModifiedName()
{
  return this->Internal->LastModifiedPropertyInfo.PropertyName;
}

//-----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastProxyRegisteredGroup()
{
  return this->Internal->LastRegisterProxyInfo.GroupName;
}

//-----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastProxyRegisteredName()
{
  return this->Internal->LastRegisterProxyInfo.ProxyName;
}

//-----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastProxyUnRegisteredGroup()
{
  return this->Internal->LastUnRegisterProxyInfo.GroupName;
}

//-----------------------------------------------------------------------------
const char* vtkSMPythonTraceObserver::GetLastProxyUnRegisteredName()
{
  return this->Internal->LastUnRegisterProxyInfo.ProxyName;
}

//-----------------------------------------------------------------------------
void vtkSMPythonTraceObserver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  //os << indent << "Var: " << this->Var << endl;
}
