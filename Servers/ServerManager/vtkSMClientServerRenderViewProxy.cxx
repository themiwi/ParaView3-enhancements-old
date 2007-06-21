/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMClientServerRenderViewProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMClientServerRenderViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMClientServerRenderSyncManagerHelper.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMClientServerRenderViewProxy);
vtkCxxRevisionMacro(vtkSMClientServerRenderViewProxy, "$Revision: 1.6 $");

//----------------------------------------------------------------------------
vtkSMClientServerRenderViewProxy::vtkSMClientServerRenderViewProxy()
{
  this->RenderSyncManager = 0;
  this->SquirtLevel = 0;
}

//----------------------------------------------------------------------------
vtkSMClientServerRenderViewProxy::~vtkSMClientServerRenderViewProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::InitializeForMultiView(
  vtkSMViewProxy* view)
{
  vtkSMClientServerRenderViewProxy* otherView =
    vtkSMClientServerRenderViewProxy::SafeDownCast(view);
  if (!otherView)
    {
    vtkErrorMacro("Other view must be a vtkSMClientServerRenderViewProxy.");
    return;
    }

  if (this->ObjectsCreated)
    {
    vtkErrorMacro("InitializeForMultiView must be called before CreateVTKObjects.");
    return;
    }

  otherView->UpdateVTKObjects();

  this->SharedServerRenderSyncManagerID =
    otherView->SharedServerRenderSyncManagerID.IsNull()?
    otherView->RenderSyncManager->GetID():
    otherView->SharedServerRenderSyncManagerID;

  this->SharedRenderWindowID = otherView->SharedRenderWindowID.IsNull()?
    otherView->RenderWindowProxy->GetID() : otherView->SharedRenderWindowID;

  this->Superclass::InitializeForMultiView(view);
}

//----------------------------------------------------------------------------
bool vtkSMClientServerRenderViewProxy::BeginCreateVTKObjects()
{
  this->RenderWindowProxy = this->GetSubProxy("RenderWindow");
  if (!this->RenderWindowProxy)
    {
    vtkErrorMacro("RenderWindow subproxy must be defined.");
    return false;
    }

  vtkSMClientServerRenderSyncManagerHelper::CreateRenderWindow(
    this->RenderWindowProxy, this->SharedRenderWindowID);

  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->RenderSyncManager = this->GetSubProxy("RenderSyncManager");
  if (!this->RenderSyncManager)
    {
    vtkErrorMacro("RenderSyncManager subproxy must be defined.");
    return false;
    }

  // RenderSyncManager proxy represents vtkPVDesktopDeliveryClient on the client
  // side and vtkPVDesktopDeliveryServer on the server root.
  // Additionally, if SharedServerRenderSyncManagerID is set, then the server side
  // vtkPVDesktopDeliveryServer instance is shared among all views.
 
  vtkSMClientServerRenderSyncManagerHelper::CreateRenderSyncManager(
    this->RenderSyncManager, this->SharedServerRenderSyncManagerID,
    "vtkPVDesktopDeliveryServer");

  return true;
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  // * Initialize the RenderSyncManager.
  this->InitializeRenderSyncManager();
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::InitializeRenderSyncManager()
{
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkSMClientServerRenderSyncManagerHelper::InitializeRenderSyncManager(
    this->RenderSyncManager, this->RenderWindowProxy);

  // Make the render sync manager aware of our renderers.
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "AddRenderer" 
          << (int)this->GetSelfID().ID
          << this->RendererProxy->GetID() 
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER_ROOT, stream);

  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "AddRenderer" 
          << this->RendererProxy->GetID()
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetId" 
          << (int)this->GetSelfID().ID
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::BeginStillRender()
{
  this->Superclass::BeginStillRender();

  // Disable squirt compression.
  this->SetSquirtLevelInternal(0);
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::BeginInteractiveRender()
{
  this->Superclass::BeginInteractiveRender();

  // Use user-specified squirt compression.
  this->SetSquirtLevelInternal(this->SquirtLevel);
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::SetSquirtLevelInternal(int level)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderSyncManager->GetProperty("SquirtLevel"));
  if (ivp)
    {
    ivp->SetElement(0, level);
    this->RenderSyncManager->UpdateProperty("SquirtLevel");
    }
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::SetUseCompositing(bool usecompositing)
{
  // We don't need to set the UseCompositing flag on the
  // ParallelRenderManager, but on the RenderSyncManager.
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderSyncManager->GetProperty("UseCompositing"));
  if (ivp)
    {
    ivp->SetElement(0, usecompositing? 1 : 0);
    this->RenderSyncManager->UpdateProperty("UseCompositing");
    } 

  // Update the view information so that all representations/strategies will be
  // made aware of the new UseCompositing state.
  this->Information->Set(USE_COMPOSITING(), usecompositing? 1: 0);
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::SetGUISize(int x, int y)
{
  this->vtkSMRenderViewProxy::SetGUISize(x, y);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetGUISize" << x << y
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::SetViewPosition(int x, int y)
{
  this->Superclass::SetViewPosition(x, y);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke 
          << this->RenderSyncManager->GetID()
          << "SetWindowPosition" << x << y
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, stream);
}

//----------------------------------------------------------------------------
void vtkSMClientServerRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SquirtLevel: " << this->SquirtLevel << endl;
}


