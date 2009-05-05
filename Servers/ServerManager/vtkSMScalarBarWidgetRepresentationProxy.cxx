/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMScalarBarWidgetRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMScalarBarWidgetRepresentationProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkScalarBarRepresentation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMViewProxy.h"

vtkStandardNewMacro(vtkSMScalarBarWidgetRepresentationProxy);
vtkCxxRevisionMacro(vtkSMScalarBarWidgetRepresentationProxy, "$Revision: 1.10 $");

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetRepresentationProxy::vtkSMScalarBarWidgetRepresentationProxy()
{
  this->ActorProxy = NULL;
  this->ViewProxy = NULL;
  this->Enabled = 0;
}

//----------------------------------------------------------------------------
vtkSMScalarBarWidgetRepresentationProxy::~vtkSMScalarBarWidgetRepresentationProxy()
{
  this->ActorProxy = NULL;
  this->ViewProxy = NULL;
}

//----------------------------------------------------------------------------
bool vtkSMScalarBarWidgetRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  if (!this->Superclass::AddToView(view))
    {
    return false;
    }
  
  this->ViewProxy = view;
  this->SetEnabled(this->Enabled);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMScalarBarWidgetRepresentationProxy::RemoveFromView(
                                                           vtkSMViewProxy* view)
{
  if (!this->Superclass::RemoveFromView(view))
    {
    return false;
    }

  this->ViewProxy = 0;

  return true;
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::SetVisibility(int visible)
{
  vtkSMPropertyHelper(this->ActorProxy, "Visibility").Set(visible);
  this->ActorProxy->UpdateVTKObjects();
  this->SetEnabled(visible);
}

//-----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->ActorProxy = this->GetSubProxy("Prop2DActor");
  if (!this->ActorProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2DActor.");
    return;
    }

  this->ActorProxy->SetServers(
                    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects();

  if (!this->RepresentationProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2D.");
    return;
    }

  vtkSMPropertyHelper(this->RepresentationProxy, "ScalarBarActor").Set(
    this->ActorProxy);
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::SetEnabled(int enable)
{
  if (this->ViewProxy)
    {
    this->Superclass::SetEnabled(enable);
    }

  this->Enabled = enable;
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::ExecuteEvent(unsigned long event)
{
  if (event == vtkCommand::InteractionEvent)
    {
    // BUG #5399. If the widget's position is beyond the viewport, fix it.
    vtkScalarBarRepresentation* repr = vtkScalarBarRepresentation::SafeDownCast(
      this->RepresentationProxy->GetClientSideObject());
    if (repr)
      {
      double position[2];
      position[0] = repr->GetPosition()[0];
      position[1] = repr->GetPosition()[1];
      if (position[0] < 0.0)
        {
        position[0] = 0.0;
        }
      if (position[0] > 0.97)
        {
        position[0] = 0.97;
        }
      if (position[1] < 0.0)
        {
        position[1] = 0.0;
        }
      if (position[1] > 0.97)
        {
        position[1] = 0.97;
        }
      repr->SetPosition(position);
      }
    }
  this->Superclass::ExecuteEvent(event);
}

//----------------------------------------------------------------------------
void vtkSMScalarBarWidgetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


