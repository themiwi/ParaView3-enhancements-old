/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPickPointWidget.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPickPointWidget.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkCellPicker.h"
#include "vtkCommand.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"

#include "vtkPVRenderModule.h"


vtkStandardNewMacro(vtkPickPointWidget);
vtkCxxRevisionMacro(vtkPickPointWidget, "$Revision: 1.3 $");
vtkCxxSetObjectMacro(vtkPickPointWidget,RenderModule,vtkPVRenderModule);



//----------------------------------------------------------------------------
vtkPickPointWidget::vtkPickPointWidget()
{
  this->EventCallbackCommand->SetCallback(vtkPickPointWidget::ProcessEvents);
  this->RenderModule = 0;
}

//----------------------------------------------------------------------------
vtkPickPointWidget::~vtkPickPointWidget()
{
  this->SetRenderModule(NULL);
}

//----------------------------------------------------------------------------
void vtkPickPointWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RenderModule: (" << this->RenderModule << ")\n";
}


//----------------------------------------------------------------------------
void vtkPickPointWidget::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }

  if ( enabling && ! this->Enabled)
    {
    // listen for the following events
    vtkRenderWindowInteractor *i = this->Interactor;
    i->AddObserver(vtkCommand::KeyPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    }

  this->Superclass::SetEnabled(enabling);
}

//----------------------------------------------------------------------------
void vtkPickPointWidget::ProcessEvents(vtkObject* object, 
                                       unsigned long event,
                                       void* clientdata, 
                                       void* calldata)
{
  vtkInteractorObserver* self 
    = reinterpret_cast<vtkInteractorObserver *>( clientdata );

  vtkPointWidget::ProcessEvents(object, event, clientdata, calldata);

  //look for char and delete events
  switch(event)
    {
    case vtkCommand::CharEvent:
      self->OnChar();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkPickPointWidget::OnChar()
{
  if (this->Interactor->GetKeyCode() == 'p' ||
      this->Interactor->GetKeyCode() == 'P' )
    {
    if (this->RenderModule == NULL)
      {
      vtkErrorMacro("Cannot pick without a render module.");
      return;
      }
    int X = this->Interactor->GetEventPosition()[0];
    int Y = this->Interactor->GetEventPosition()[1];
    float z = this->RenderModule->GetZBufferValue(X, Y);
    double pt[4];
    this->ComputeDisplayToWorld(double(X),double(Y),double(z),pt);
    this->Cursor3D->SetFocalPoint(pt);
    this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    return;
    }
}



