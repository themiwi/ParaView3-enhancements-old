/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPickLineWidgetProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPickLineWidgetProxy.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkProcessModule.h"
#include "vtkLineWidget.h"

vtkStandardNewMacro(vtkSMPickLineWidgetProxy);
vtkCxxRevisionMacro(vtkSMPickLineWidgetProxy, "$Revision: 1.3 $");
//-----------------------------------------------------------------------------
vtkSMPickLineWidgetProxy::vtkSMPickLineWidgetProxy()
{
  this->EventTag = 0;
  this->Interactor = 0;
  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this);
  this->EventCallbackCommand->SetCallback(vtkSMPickLineWidgetProxy::ProcessEvents);
  this->LastPicked = 0;
   
}

//-----------------------------------------------------------------------------
vtkSMPickLineWidgetProxy::~vtkSMPickLineWidgetProxy()
{
  this->EventCallbackCommand->Delete();
}

//-----------------------------------------------------------------------------
/*static*/
void vtkSMPickLineWidgetProxy::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                          unsigned long event,
                                          void* clientdata, 
                                          void* vtkNotUsed(calldata))
{
  vtkSMPickLineWidgetProxy* self = reinterpret_cast<vtkSMPickLineWidgetProxy*>(
    clientdata);
  if (!self)
    {
    vtkGenericWarningMacro("ProcessEvents received from unknown object.");
    return;
    }

  switch (event) 
    {
  case vtkCommand::CharEvent:
    self->OnChar();
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkSMPickLineWidgetProxy::OnChar()
{
  if (!this->ObjectsCreated || this->GetNumberOfIDs() < (unsigned int)1)
    {
    vtkErrorMacro("LineWidgetProxy not created yet.");
    return;
    }

  vtkRenderer* ren = this->CurrentRenderModuleProxy->GetRenderer();
  
  if (ren && this->Interactor->GetKeyCode() == 'p' || 
    this->Interactor->GetKeyCode() == 'P' )
    {
    if (this->CurrentRenderModuleProxy == NULL)
      {
      vtkErrorMacro("Cannot pick without a render module.");
      return;
      }
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    float z = this->CurrentRenderModuleProxy->GetZBufferValue(X, Y);
    double pt[4];

    // ComputeDisplayToWorld
    ren->SetDisplayPoint(double(X), double(Y), z);
    ren->DisplayToWorld();
    ren->GetWorldPoint(pt);

    if (this->LastPicked == 0)
      { // Choose the closest point.
      const double *pt1 = this->GetPoint1();
      const double *pt2 = this->GetPoint2();
      double d1, d2, tmp[3];
      tmp[0] = pt1[0]-pt[0]; 
      tmp[1] = pt1[1]-pt[1]; 
      tmp[2] = pt1[2]-pt[2];
      d1 = tmp[0]*tmp[0] + tmp[1]*tmp[1] + tmp[2]*tmp[2];
      tmp[0] = pt2[0]-pt[0]; 
      tmp[1] = pt2[1]-pt[1]; 
      tmp[2] = pt2[2]-pt[2];
      d2 = tmp[0]*tmp[0] + tmp[1]*tmp[1] + tmp[2]*tmp[2];
      this->LastPicked = 1;
      if (d2 < d1)
        {
        this->LastPicked = 2;
        }
      }
    else
      { // toggle point
      if (this->LastPicked == 1)
        {
        this->LastPicked = 2;
        }
      else
        {
        this->LastPicked = 1;
        }
      }

    if (this->LastPicked == 1)
      {
      this->SetPoint1(pt[0], pt[1], pt[2]);
      }
    else
      {
      this->SetPoint2(pt[0], pt[1], pt[2]);
      }
    this->UpdateVTKObjects(); // This will push down the values on to the
      // server objects (and client objects).
    this->InvokeEvent(vtkCommand::WidgetModifiedEvent); //So that the GUI
      // knows that the widget has been modified.
    this->Interactor->Render();
    }
}

//-----------------------------------------------------------------------------
void vtkSMPickLineWidgetProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  this->Superclass::AddToRenderModule(rm);
  if (this->Interactor || !this->ObjectsCreated || this->GetNumberOfIDs() < 1)
    {
    // already added to a render module.
    return;
    }
  this->Interactor = rm->GetInteractor();
  
  if (this->Interactor)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkLineWidget* wdg = vtkLineWidget::SafeDownCast(
      pm->GetObjectFromID(this->GetID(0)));
    
    this->EventTag = this->Interactor->AddObserver(vtkCommand::CharEvent,
      this->EventCallbackCommand, wdg->GetPriority()); 
    }
}


//-----------------------------------------------------------------------------
void vtkSMPickLineWidgetProxy::RemoveFromRenderModule(
  vtkSMRenderModuleProxy* rm)
{
  this->Superclass::RemoveFromRenderModule(rm);

  if (this->Interactor && this->EventTag)
    {
    this->Interactor->RemoveObserver(this->EventTag);
    this->EventTag = 0;
    }
  this->Interactor = 0;
}
//-----------------------------------------------------------------------------
void vtkSMPickLineWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
