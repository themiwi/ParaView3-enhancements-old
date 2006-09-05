/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPQStateLoader.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPQStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMMultiViewRenderModuleProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"

vtkStandardNewMacro(vtkSMPQStateLoader);
vtkCxxRevisionMacro(vtkSMPQStateLoader, "$Revision: 1.9 $");
vtkCxxSetObjectMacro(vtkSMPQStateLoader, MultiViewRenderModuleProxy, 
  vtkSMMultiViewRenderModuleProxy);
//-----------------------------------------------------------------------------
vtkSMPQStateLoader::vtkSMPQStateLoader()
{
  this->MultiViewRenderModuleProxy = 0;
  this->UseExistingRenderModules = 0;
  this->UsedExistingRenderModules = 0;
}

//-----------------------------------------------------------------------------
vtkSMPQStateLoader::~vtkSMPQStateLoader()
{
  this->SetMultiViewRenderModuleProxy(0);
}

//-----------------------------------------------------------------------------
int vtkSMPQStateLoader::LoadState(vtkPVXMLElement* rootElement, 
  int keep_proxies)
{
  this->UsedExistingRenderModules= 0;
  return this->Superclass::LoadState(rootElement, keep_proxies);
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPQStateLoader::NewProxyInternal(
  const char* xml_group, const char* xml_name)
{
  // Check if the proxy requested is a render module.
  if (xml_group && xml_name && strcmp(xml_group, "rendermodules") == 0)
    {
    if (strcmp(xml_name, "MultiViewRenderModule") == 0)
      {
      if (this->MultiViewRenderModuleProxy)
        {
        this->MultiViewRenderModuleProxy->Register(this);
        return this->MultiViewRenderModuleProxy;
        }
      vtkWarningMacro("MultiViewRenderModuleProxy is not set. "
        "Creating MultiViewRenderModuleProxy from the state.");
      }
    else
      {
      // Create a rendermodule.
      if (this->MultiViewRenderModuleProxy)
        {
        if (this->UseExistingRenderModules)
          {
          vtkSMProxy* p = this->MultiViewRenderModuleProxy->GetProxy(
            static_cast<unsigned int>(this->UsedExistingRenderModules));
          this->UsedExistingRenderModules++;
          if (p)
            {
            p->Register(this);
            return p;
            }
          }
        // Can't use exiting module (none present, or all present are have
        // already been used, hence we allocate a new one.
        return this->MultiViewRenderModuleProxy->NewRenderModule();
        }
      vtkWarningMacro("MultiViewRenderModuleProxy is not set. "
        "Creating MultiViewRenderModuleProxy from the state.");
      }
    }
  else if (xml_group && xml_name && strcmp(xml_group, "displays")==0)
    {
    vtkSMProxy *display = this->Superclass::NewProxyInternal(xml_group, xml_name);
    if (vtkSMDataObjectDisplayProxy::SafeDownCast(display))
      {
      if (this->MultiViewRenderModuleProxy)
        {
        display->Delete();
        display = this->MultiViewRenderModuleProxy->CreateDisplayProxy();
        }
      }
    return display;
    }
  return this->Superclass::NewProxyInternal(xml_group, xml_name);
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::RegisterProxyInternal(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  if (proxy->GetXMLGroup() 
    && strcmp(proxy->GetXMLGroup(), "rendermodules")==0)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    if (pxm->GetProxyName(group, proxy))
      {
      // render module is registered, don't re-register it.
      return;
      }
    }
  this->Superclass::RegisterProxyInternal(group, name, proxy);
}

//-----------------------------------------------------------------------------
void vtkSMPQStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MultiViewRenderModuleProxy: " 
     << this->MultiViewRenderModuleProxy << endl;
  os << indent << "UseExistingRenderModules: "
     << this->UseExistingRenderModules << endl;
}
