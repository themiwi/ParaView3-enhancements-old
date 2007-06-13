/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPropRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPropRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyLink.h"

vtkStandardNewMacro(vtkSMPropRepresentationProxy);
vtkCxxRevisionMacro(vtkSMPropRepresentationProxy, "$Revision: 1.3 $");
//----------------------------------------------------------------------------
vtkSMPropRepresentationProxy::vtkSMPropRepresentationProxy()
{
  this->SelectionRepresentation = 0;
  this->SelectionPropLink = vtkSMProxyLink::New();
  this->SelectionPropLink->AddException("LODMapper");
  this->SelectionPropLink->AddException("Mapper");
  this->SelectionPropLink->AddException("Pickable");
  this->SelectionPropLink->AddException("Property");
  this->SelectionPropLink->AddException("Texture");
  this->SelectionPropLink->AddException("Visibility");

}

//----------------------------------------------------------------------------
vtkSMPropRepresentationProxy::~vtkSMPropRepresentationProxy()
{
  this->SelectionPropLink->Delete();
}

//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (this->SelectionRepresentation)
    {
    int visible = this->GetSelectionVisibility();
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->SelectionRepresentation->GetProperty("Visibility"));
    ivp->SetElement(0, visible);
    this->SelectionRepresentation->UpdateProperty("Visibility");
    }

  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
bool vtkSMPropRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  if (!this->Superclass::AddToView(view))
    {
    return false;
    }

  if (this->GetSubProxy("Prop3D"))
    {
    renderView->AddPropToRenderer(this->GetSubProxy("Prop3D"));
    }
  if (this->GetSubProxy("Prop2D"))
    {
    renderView->AddPropToRenderer(this->GetSubProxy("Prop2D"));
    }

  if (this->GetSelectionSupported() && this->SelectionRepresentation)
    {
    this->SelectionRepresentation->AddToView(view);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPropRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  if (this->GetSubProxy("Prop3D"))
    {
    renderView->RemovePropFromRenderer(this->GetSubProxy("Prop3D"));
    }
  if (this->GetSubProxy("Prop2D"))
    {
    renderView->RemovePropFromRenderer(this->GetSubProxy("Prop2D"));
    }

  if (this->SelectionRepresentation)
    {
    this->SelectionRepresentation->RemoveFromView(view);
    }
  return this->Superclass::RemoveFromView(view);
}


//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::LinkSelectionProp(vtkSMProxy* prop)
{
  if (this->SelectionRepresentation && prop)
    {
    this->SelectionPropLink->AddLinkedProxy(prop, vtkSMLink::INPUT);
    }
}

//----------------------------------------------------------------------------
bool vtkSMPropRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  this->SelectionRepresentation = 
    vtkSMDataRepresentationProxy::SafeDownCast(this->GetSubProxy(
        "SelectionRepresentation"));
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPropRepresentationProxy::EndCreateVTKObjects()
{
  if (this->SelectionRepresentation)
    {
    // Setup selection pipeline connections.
    this->Connect(this->GetInputProxy(), this->SelectionRepresentation);

    // Link actor properties with the seleciton actor so that actor
    // transformations work.
    this->SelectionPropLink->AddLinkedProxy(this->SelectionRepresentation,
      vtkSMLink::OUTPUT);
    }
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::GetActiveStrategies(
  vtkSMRepresentationStrategyVector& activeStrategies)
{
  this->Superclass::GetActiveStrategies(activeStrategies);
  if (this->SelectionRepresentation && this->GetSelectionSupported())
    {
    this->SelectionRepresentation->GetActiveStrategies(activeStrategies);
    }
}

//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::SetUpdateTime(double time)
{
  this->Superclass::SetUpdateTime(time);
  if (this->SelectionRepresentation && this->GetSelectionSupported())
    {
    this->SelectionRepresentation->SetUpdateTime(time);
    }
}

//----------------------------------------------------------------------------
void vtkSMPropRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


