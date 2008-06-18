/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMTwoDRenderViewProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTwoDRenderViewProxy.h"

#include "vtkSMSourceProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMTwoDRenderViewProxy);
vtkCxxRevisionMacro(vtkSMTwoDRenderViewProxy, "$Revision: 1.2 $");
//----------------------------------------------------------------------------
vtkSMTwoDRenderViewProxy::vtkSMTwoDRenderViewProxy()
{
  this->RenderView = 0;
}

//----------------------------------------------------------------------------
vtkSMTwoDRenderViewProxy::~vtkSMTwoDRenderViewProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::AddRepresentation(vtkSMRepresentationProxy* repr)
{
  if (this->RenderView)
    {
    this->RenderView->AddRepresentation(repr);
    }
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::RemoveRepresentation(vtkSMRepresentationProxy* repr)
{
  if (this->RenderView)
    {
    this->RenderView->RemoveRepresentation(repr);
    }
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::RemoveAllRepresentations()
{
  if (this->RenderView)
    {
    this->RenderView->RemoveAllRepresentations();
    }
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::SetViewUpdateTime(double time)
{
  this->Superclass::SetViewUpdateTime(time);
  if (this->RenderView)
    {
    this->RenderView->SetViewUpdateTime(time);
    }
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::SetCacheTime(double time)
{
  this->Superclass::SetCacheTime(time);
  if (this->RenderView)
    {
    this->RenderView->SetCacheTime(time);
    }
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::SetUseCache(int use)
{
  this->Superclass::SetUseCache(use);
  if (this->RenderView)
    {
    this->RenderView->SetUseCache(use);
    }
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::SetViewPosition(int x, int y)
{
  this->Superclass::SetViewPosition(x, y);
  if (this->RenderView)
    {
    this->RenderView->SetViewPosition(x, y);
    }
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::SetGUISize(int x, int y)
{
  this->Superclass::SetGUISize(x, y);
  if (this->RenderView)
    {
    this->RenderView->SetGUISize(x, y);
    }
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::StillRender()
{
  if (this->RenderView)
    {
    this->RenderView->StillRender();
    }
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::InteractiveRender()
{
  if (this->RenderView)
    {
    this->RenderView->InteractiveRender();
    }
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMTwoDRenderViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    sproxy->UpdatePipeline(this->GetViewUpdateTime());
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
    "ImageSliceRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "ImageSliceRepresentation"));
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      repr->GetProperty("UseXYPlane"));
    ivp->SetElement(0, 1);
    return repr;
    }

  // Currently only images can be shown 
  vtkErrorMacro("This view only supports showing images.");
  return 0;
}

//----------------------------------------------------------------------------
bool vtkSMTwoDRenderViewProxy::BeginCreateVTKObjects()
{
  this->RenderView = vtkSMRenderViewProxy::SafeDownCast(
    this->GetSubProxy("RenderView"));
  if (!this->RenderView)
    {
    vtkErrorMacro("Missing \"RenderView\" subproxy.");
    return false;
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderView->GetProperty("CameraParallelProjection"));
  ivp->SetElement(0, 1);

  return this->Superclass::BeginCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderView: " << this->RenderView << endl;
}


