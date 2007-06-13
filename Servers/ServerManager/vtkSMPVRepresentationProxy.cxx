/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPVRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

inline void vtkSMPVRepresentationProxySetInt(
  vtkSMProxy* proxy, const char* pname, int val)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    proxy->GetProperty(pname));
  if (ivp)
    {
    ivp->SetElement(0, val);
    }
  proxy->UpdateProperty(pname);
}

vtkStandardNewMacro(vtkSMPVRepresentationProxy);
vtkCxxRevisionMacro(vtkSMPVRepresentationProxy, "$Revision: 1.1 $");
//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::vtkSMPVRepresentationProxy()
{
  this->Representation = VTK_SURFACE;
  this->SurfaceRepresentation = 0;
  this->VolumeRepresentation = 0;
  this->OutlineRepresentation = 0;

  this->ActiveRepresentation = 0;
  this->SelectionVisibility = 1;

  this->SetSelectionSupported(true);
}

//----------------------------------------------------------------------------
vtkSMPVRepresentationProxy::~vtkSMPVRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::EndCreateVTKObjects()
{
  this->SurfaceRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("SurfaceRepresentation"));
  this->VolumeRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("VolumeRepresentation"));
  this->OutlineRepresentation = vtkSMDataRepresentationProxy::SafeDownCast(
    this->GetSubProxy("OutlineRepresentation"));

  this->Connect(this->GetInputProxy(), this->SurfaceRepresentation);
  this->Connect(this->GetInputProxy(), this->OutlineRepresentation);

  vtkSMPVRepresentationProxySetInt(this->SurfaceRepresentation, "Visibility", 0);
  vtkSMPVRepresentationProxySetInt(this->OutlineRepresentation, "Visibility", 0);


  this->SurfaceRepresentation->SetSelectionSupported(false);
  this->OutlineRepresentation->SetSelectionSupported(false);
  if (this->VolumeRepresentation)
    {
    this->Connect(this->GetInputProxy(), this->VolumeRepresentation);
    vtkSMPVRepresentationProxySetInt(this->VolumeRepresentation, "Visibility", 0);
    this->VolumeRepresentation->SetSelectionSupported(false);
    }

  // TODO: Add observer to fire StartEvent/EndEvent.

  // Setup the ActiveRepresentation pointer.
  int repr = this->Representation;
  this->Representation = -1;
  this->SetRepresentation(repr);

  this->LinkSelectionProp(this->SurfaceRepresentation);
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  this->SurfaceRepresentation->AddToView(view);
  this->OutlineRepresentation->AddToView(view);
  if (this->VolumeRepresentation)
    {
    this->VolumeRepresentation->AddToView(view);
    }

  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  this->SurfaceRepresentation->RemoveFromView(view);
  this->OutlineRepresentation->RemoveFromView(view);
  if (this->VolumeRepresentation)
    {
    this->VolumeRepresentation->RemoveFromView(view);
    }

  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetRepresentation(int repr)
{
  if (this->Representation != repr)
    {
    this->Representation = repr;
    if (this->ActiveRepresentation)
      {
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Visibility", 0);
      }

    switch (this->Representation)
      {
    case OUTLINE:
      this->ActiveRepresentation = this->OutlineRepresentation;
      break;

    case VOLUME:
      if (this->VolumeRepresentation)
        {
        this->ActiveRepresentation = this->VolumeRepresentation;
        break;
        }
      else
        {
        vtkErrorMacro("Volume representation not supported.");
        }

    case POINTS:
      this->ActiveRepresentation = this->SurfaceRepresentation;
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Representation", VTK_POINTS);
      break;

    case WIREFRAME:
      this->ActiveRepresentation = this->SurfaceRepresentation;
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Representation", VTK_WIREFRAME);
      break;

    case SURFACE:
    default:
      this->ActiveRepresentation = this->SurfaceRepresentation;
      vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Representation", VTK_SURFACE);
      }

    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Visibility", 
      this->GetVisibility());
    this->Modified();
    // TODO: Do we need to call MarkModified();
    }
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetVisibility(int visible)
{
  if (this->ActiveRepresentation)
    {
    vtkSMPVRepresentationProxySetInt(this->ActiveRepresentation, "Visibility", 
      visible);
    }
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMPVRepresentationProxy::GetDisplayedDataInformation()
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->GetDisplayedDataInformation();
    }

  return this->Superclass::GetDisplayedDataInformation();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMPVRepresentationProxy::GetFullResDataInformation()
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->GetFullResDataInformation();
    }

  return this->Superclass::GetFullResDataInformation();
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (this->ActiveRepresentation)
    {
    this->ActiveRepresentation->Update(view);
    }

  this->Superclass::Update(view);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::UpdateRequired()
{
  if (this->ActiveRepresentation)
    {
    if (this->ActiveRepresentation->UpdateRequired())
      {
      return true;
      }
    }

  return this->Superclass::UpdateRequired();
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetUpdateTime(double time)
{
  if (this->ActiveRepresentation)
    {
    this->ActiveRepresentation->SetUpdateTime(time);
    }

  this->Superclass::SetUpdateTime(time);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::SetUseViewTimeForUpdate(bool use)
{
  if (this->ActiveRepresentation)
    {
    this->ActiveRepresentation->SetUseViewTimeForUpdate(use);
    }

  this->Superclass::SetUseViewTimeForUpdate(use);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  if (modifiedProxy != this && this->ActiveRepresentation)
    {
    this->ActiveRepresentation->MarkModified(modifiedProxy);
    }

  this->Superclass::MarkModified(modifiedProxy);
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::GetActiveStrategies(
  vtkSMRepresentationStrategyVector& activeStrategies)
{
  if (this->ActiveRepresentation)
    {
    this->ActiveRepresentation->GetActiveStrategies(activeStrategies);
    }

  this->Superclass::GetActiveStrategies(activeStrategies);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPVRepresentationProxy::ConvertSelection(vtkSelection* input)
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->ConvertSelection(input);
    }

  return this->Superclass::ConvertSelection(input);
}

//----------------------------------------------------------------------------
bool vtkSMPVRepresentationProxy::GetOrderedCompositingNeeded()
{
  if (this->ActiveRepresentation)
    {
    return this->ActiveRepresentation->GetOrderedCompositingNeeded();
    }

  return this->Superclass::GetOrderedCompositingNeeded();
}

//----------------------------------------------------------------------------
void vtkSMPVRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


