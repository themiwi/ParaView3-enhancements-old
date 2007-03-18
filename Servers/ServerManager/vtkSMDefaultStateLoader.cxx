/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMDefaultStateLoader.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDefaultStateLoader.h"

#include "vtkClientServerInterpreter.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkSMDefaultStateLoader);
vtkCxxRevisionMacro(vtkSMDefaultStateLoader, "$Revision: 1.5 $");
//-----------------------------------------------------------------------------
vtkSMDefaultStateLoader::vtkSMDefaultStateLoader()
{
}

//-----------------------------------------------------------------------------
vtkSMDefaultStateLoader::~vtkSMDefaultStateLoader()
{
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMDefaultStateLoader::NewProxy(int id)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerID csid;
  csid.ID = static_cast<vtkTypeUInt32>(id);
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(
    pm->GetInterpreter()->GetObjectFromID(csid, 1));
  if (proxy)
    {
    proxy->Register(this);
    return proxy;
    }
  vtkDebugMacro("Creating new proxy: " << id);
  proxy = this->Superclass::NewProxy(id);
  // the extra check for SelfID is needed since Superclass::NewProxy(id)
  // can lead to call to vtkSMDefaultStateLoader::NewProxyFromElement()
  // which sets the self id and we don't want to raise unnecessary errors
  // if that's the case.
  if (proxy && proxy->GetSelfID() != csid)
    {
    proxy->SetSelfID(csid);
    }
  return proxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMDefaultStateLoader::NewProxyFromElement(
  vtkPVXMLElement* proxyElement, int id)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerID csid;
  csid.ID = static_cast<vtkTypeUInt32>(id);
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(
    pm->GetInterpreter()->GetObjectFromID(csid, 1));
  if (proxy)
    {
    proxy->Register(this);
    return proxy;
    }
  vtkDebugMacro("Creating new proxy: " << id);
  proxy = this->Superclass::NewProxyFromElement(proxyElement, id);
  if (proxy)
    {
    proxy->SetSelfID(csid);
    } 
  return proxy;
}

//-----------------------------------------------------------------------------
void vtkSMDefaultStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
