/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMMPIRenderModuleProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMPIRenderModuleProxy.h"

#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkRenderWindow.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"

vtkStandardNewMacro(vtkSMMPIRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMMPIRenderModuleProxy, "$Revision: 1.8 $");
//-----------------------------------------------------------------------------
vtkSMMPIRenderModuleProxy::vtkSMMPIRenderModuleProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMMPIRenderModuleProxy::~vtkSMMPIRenderModuleProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMMPIRenderModuleProxy::StillRender()
{
  if (this->CompositeManagerProxy)
    {
    this->LocalRender = this->GetLocalRenderDecision(
      this->GetTotalVisibleGeometryMemorySize(), 1);
    vtkSMIntVectorProperty* pr = vtkSMIntVectorProperty::SafeDownCast(
      this->CompositeManagerProxy->GetProperty("ParallelRendering"));
    vtkSMIntVectorProperty* rep = vtkSMIntVectorProperty::SafeDownCast(
      this->CompositeManagerProxy->GetProperty("RenderEventPropagation"));
    if (this->LocalRender)
      {
      if (pr) {pr->SetElement(0, 0);}
      if (rep) {rep->SetElement(0, 0);}
      }
    else
      {
      if (pr) {pr->SetElement(0, 1);}
      if (rep) {rep->SetElement(0, 1);}
      }
    this->CompositeManagerProxy->UpdateVTKObjects();
    }

  this->Superclass::StillRender();
}

//-----------------------------------------------------------------------------
void vtkSMMPIRenderModuleProxy::InteractiveRender()
{
  if (this->CompositeManagerProxy)
    {
    int useLOD = this->GetUseLODDecision();
    unsigned long totalMemory = 0;
    totalMemory = (useLOD)? this->GetTotalVisibleLODGeometryMemorySize() :
      this->GetTotalVisibleGeometryMemorySize();
    this->LocalRender = this->GetLocalRenderDecision(totalMemory, 0);

    vtkSMIntVectorProperty* pr = vtkSMIntVectorProperty::SafeDownCast(
      this->CompositeManagerProxy->GetProperty("ParallelRendering"));
    vtkSMIntVectorProperty* rep = vtkSMIntVectorProperty::SafeDownCast(
      this->CompositeManagerProxy->GetProperty("RenderEventPropagation"));
    if (this->LocalRender)
      {
      if (pr) {pr->SetElement(0, 0);}
      if (rep) {rep->SetElement(0, 0);}
      }
    else
      {
      if (pr) {pr->SetElement(0, 1);}
      if (rep) {rep->SetElement(0, 1);}
      }
    this->CompositeManagerProxy->UpdateVTKObjects();
    }

  this->Superclass::InteractiveRender();
}

//-----------------------------------------------------------------------------
void vtkSMMPIRenderModuleProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated )
    {
    return;
    }
  this->Superclass::CreateVTKObjects(numObjects);

  // By default, disable render propagation.
  vtkSMIntVectorProperty* pr = vtkSMIntVectorProperty::SafeDownCast(
    this->CompositeManagerProxy->GetProperty("ParallelRendering"));
  if (pr) {pr->SetElement(0, 0);}
  vtkSMIntVectorProperty* rep = vtkSMIntVectorProperty::SafeDownCast(
    this->CompositeManagerProxy->GetProperty("RenderEventPropagation"));
  if (rep) {rep->SetElement(0, 0);}
}

//-----------------------------------------------------------------------------
void vtkSMMPIRenderModuleProxy::CreateCompositeManager()
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // Composite Manager is vtkClientCompositeManager in client/server/renderserver mode.
  // Otherwise, vtkPVTreeComposite is used.
  const char* manager_name = 0;
  if (pm->GetOptions()->GetClientMode() || pm->GetOptions()->GetServerMode())
    {
    manager_name = "ClientCompositeManager";
    }
  else
    {
    manager_name = "TreeComposite";
    }

  vtkSMProxy* cm = pxm->NewProxy("composite_managers", manager_name);
  if (!cm)
    {
    vtkErrorMacro("Failed to create CompositeManagerProxy.");
    return;
    }
  cm->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);  
  this->AddSubProxy("CompositeManager", cm);

  cm->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMMPIRenderModuleProxy::InitializeCompositingPipeline()
{
  vtkSMIntVectorProperty* ivp;
  vtkSMProxyProperty* pp;

  if (!this->CompositeManagerProxy)
    {
    vtkErrorMacro("CompositeManagerProxy not set.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  unsigned int i;
  vtkClientServerStream stream;

  // We had trouble with SGI/aliasing with compositing.
  if (this->GetRenderWindow()->IsA("vtkOpenGLRenderWindow") &&
      (pm->GetNumberOfPartitions() > 1))
    {
    for (i=0; i < this->RenderWindowProxy->GetNumberOfIDs(); i++)
      {
      stream << vtkClientServerStream::Invoke
        << this->RenderWindowProxy->GetID(i) 
        << "SetMultiSamples" << 0
        << vtkClientServerStream::End;
      }
    pm->SendStream(this->ConnectionID, 
      this->RenderWindowProxy->GetServers(), stream);
    }

  if (pm->GetOptions()->GetClientMode() || pm->GetOptions()->GetServerMode())
    {
    // using vtkClientCompositeManager. 
    for (i=0; i < this->CompositeManagerProxy->GetNumberOfIDs(); i++)
      {
      // Clean up this mess !!!!!!!!!!!!!
      // Even a cast to vtkPVClientServerModule would be better than this.
      // How can we syncronize the process modules and render modules?
      stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
        << "GetRenderServerSocketController" 
        << pm->GetConnectionClientServerID(this->ConnectionID)
        << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke 
        << this->CompositeManagerProxy->GetID(i)
        << "SetClientController" << vtkClientServerStream::LastResult
        << vtkClientServerStream::End;

      stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
        << "GetClientMode" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke 
        << this->CompositeManagerProxy->GetID(i) 
        << "SetClientFlag"
        << vtkClientServerStream::LastResult << vtkClientServerStream::End;
      } 
    pm->SendStream(this->ConnectionID,
      this->CompositeManagerProxy->GetServers(), stream);
    }

  ivp = vtkSMIntVectorProperty::SafeDownCast(
         this->CompositeManagerProxy->GetProperty("SyncRenderWindowRenderers"));
  if (!ivp)
    {
    vtkErrorMacro("Falied to find property SyncRenderWindowRenderers");
    return;
    }
  ivp->SetElement(0, 0);

  pp = vtkSMProxyProperty::SafeDownCast(
                         this->CompositeManagerProxy->GetProperty("Renderers"));
  pp->RemoveAllProxies();
  pp->AddProxy(this->RendererProxy);
  pp->AddProxy(this->Renderer2DProxy);

  this->CompositeManagerProxy->UpdateVTKObjects();

//  this->SetCompositer("CompressCompositer");
  this->SetCompositer("TreeCompositer");
  this->Superclass::InitializeCompositingPipeline();
}

//-----------------------------------------------------------------------------
void vtkSMMPIRenderModuleProxy::SetCompositer(const char* proxyname)
{
  this->RemoveSubProxy("Compositer");
  vtkSMProxy* compositer = vtkSMObject::GetProxyManager()
    ->NewProxy("compositers", proxyname);
  
  if (!compositer)
    {
    vtkErrorMacro("Failed to create compositer " << proxyname);
    return;
    }
  compositer->SetServers(this->GetServers());
  compositer->UpdateVTKObjects();

  this->AddSubProxy("Compositer", compositer);
  
  compositer->Delete();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->CompositeManagerProxy->GetProperty("Compositer"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Compositer on CompositeManagerProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(compositer);
  this->CompositeManagerProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMMPIRenderModuleProxy::SetUseCompositeCompression(int val)
{
  if (!this->CompositeManagerProxy)
    {
    return;
    }
  this->SetCompositer( (val? "CompressCompositer" : "TreeCompositer") );
}

//-----------------------------------------------------------------------------
void vtkSMMPIRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
}
