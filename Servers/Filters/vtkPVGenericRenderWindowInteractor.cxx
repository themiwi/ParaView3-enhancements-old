/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVGenericRenderWindowInteractor.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommand.h"
#include "vtkInteractorObserver.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"

//-----------------------------------------------------------------------------
class vtkPVGenericRenderWindowInteractorObserver : public vtkCommand
{
public:
  static vtkPVGenericRenderWindowInteractorObserver* New()
    { return new vtkPVGenericRenderWindowInteractorObserver(); }

  void SetTarget(vtkPVGenericRenderWindowInteractor* target)
    {
    this->Target = target;
    }

  virtual void Execute(vtkObject* vtkNotUsed(caller), unsigned long event, void* data)
    {
    if (this->Target)
      {
      this->Target->InvokeEvent(event, data);
      }
    }

protected:
  vtkPVGenericRenderWindowInteractorObserver()
    {
    this->Target = 0;
    }
  vtkPVGenericRenderWindowInteractor* Target;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVGenericRenderWindowInteractor);
vtkCxxRevisionMacro(vtkPVGenericRenderWindowInteractor, "$Revision: 1.4 $");
vtkCxxSetObjectMacro(vtkPVGenericRenderWindowInteractor,Renderer,vtkRenderer);

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor::vtkPVGenericRenderWindowInteractor()
{
  this->PVRenderView = NULL;
  this->Renderer = NULL;
  this->InteractiveRenderEnabled = 0;
  this->Observer = vtkPVGenericRenderWindowInteractorObserver::New();
  this->Observer->SetTarget(this);
}

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor::~vtkPVGenericRenderWindowInteractor()
{
  this->Observer->SetTarget(0);
  this->Observer->Delete();

  this->SetPVRenderView(NULL);
  this->SetRenderer(NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetInteractorStyle(
  vtkInteractorObserver *style)
{
  if (this->GetInteractorStyle())
    {
    this->GetInteractorStyle()->RemoveObserver(this->Observer);
    }

  this->Superclass::SetInteractorStyle(style);

  if (this->GetInteractorStyle())
    {
    this->GetInteractorStyle()->AddObserver(
      vtkCommand::StartInteractionEvent, this->Observer);
    this->GetInteractorStyle()->AddObserver(
      vtkCommand::EndInteractionEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::ConfigureEvent()
{
  if (!this->Enabled) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::ConfigureEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetPVRenderView(vtkPVRenderViewProxy *view)
{
  if (this->PVRenderView != view)
    {
    if(this->PVRenderView)
      {
      this->PVRenderView->UnRegister(this);
      }
    // to avoid circular references
    this->PVRenderView = view;
    if (this->PVRenderView != NULL)
      {
      this->PVRenderView->Register(this);
      this->SetRenderWindow(this->PVRenderView->GetRenderWindow());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetMoveEventInformationFlipY(int x, int y)
{
  this->SetEventInformationFlipY(x, y, this->ControlKey, this->ShiftKey,
                                 this->KeyCode, this->RepeatCount,
                                 this->KeySym);
}

//----------------------------------------------------------------------------
vtkRenderer* vtkPVGenericRenderWindowInteractor::FindPokedRenderer(int,int)
{
  if (this->Renderer == NULL)
    {
    vtkErrorMacro("Renderer has not been set.");
    }

  return this->Renderer;
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::Render()
{
  if ( this->PVRenderView == NULL || this->RenderWindow == NULL)
    { // The case for interactors on the satellite processes.
    return;
    }

  // This should fix the problem of the plane widget render 
  if (this->InteractiveRenderEnabled)
    {
    this->InvokeEvent(vtkCommand::InteractionEvent);
    this->PVRenderView->Render();
    }
  else
    {
    this->PVRenderView->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetInteractiveRenderEnabled(int val)
{
  if (this->InteractiveRenderEnabled == val)
    {
    return;
    }
  this->Modified();
  this->InteractiveRenderEnabled = val;
  if (val == 0 && this->PVRenderView)
    {
    this->PVRenderView->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnLeftPress(int x, int y, 
                                                     int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::LeftButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnMiddlePress(int x, int y, 
                                                       int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::MiddleButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnRightPress(int x, int y, 
                                                      int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::RightButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnLeftRelease(int x, int y, 
                                                       int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnMiddleRelease(int x, int y, 
                                                         int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnRightRelease(int x, int y, 
                                                        int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnMove(int x, int y) 
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, 
                            this->ControlKey, this->ShiftKey,
                            this->KeyCode, this->RepeatCount,
                            this->KeySym);
  this->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnKeyPress(char keyCode, int x, int y) 
{
  this->SetEventPosition(x, this->RenderWindow->GetSize()[1]-y);
  this->KeyCode = keyCode;
  this->InvokeEvent(vtkCommand::CharEvent, NULL);
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PVRenderView: " << this->GetPVRenderView() << endl;
  os << indent << "InteractiveRenderEnabled: " 
     << this->InteractiveRenderEnabled << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
}
