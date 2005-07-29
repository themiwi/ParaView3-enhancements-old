/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSM3DWidgetProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSM3DWidgetProxy.h"

#include "vtk3DWidget.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVProcessModule.h"
#include "vtkRenderer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMRenderModuleProxy.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSM3DWidgetProxy, "$Revision: 1.12 $");
//===========================================================================
//***************************************************************************
class vtkSM3DWidgetProxyObserver : public vtkCommand
{
public:
  static vtkSM3DWidgetProxyObserver *New() 
    {return new vtkSM3DWidgetProxyObserver;};

  vtkSM3DWidgetProxyObserver()
    {
      this->Target = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event, void* calldata)
    {
      if ( this->Target )
        {
        this->Target->ExecuteEvent(wdg, event, calldata);
        }
    }
  vtkSM3DWidgetProxy* Target;
};
//***************************************************************************
//----------------------------------------------------------------------------
vtkSM3DWidgetProxy::vtkSM3DWidgetProxy()
{
  this->Placed = 1;
  this->IgnorePlaceWidgetChanges = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 0.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;

  this->Observer = vtkSM3DWidgetProxyObserver::New();
  this->Observer->Target = this;
  this->Enabled = 0;
  this->CurrentRenderModuleProxy = 0;
}

//----------------------------------------------------------------------------
vtkSM3DWidgetProxy::~vtkSM3DWidgetProxy()
{
  this->Observer->Target = NULL;
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::SetEnabled(int e)
{ 
  this->Enabled = e;

  if (!this->CurrentRenderModuleProxy)
    {
    return; // widgets are not enabled till rendermodule is set.
    }
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  unsigned int numObjects = this->GetNumberOfIDs();
  for(cc=0;cc < numObjects; cc++)
    {
    str << vtkClientServerStream::Invoke << this->GetID(cc)
      << "SetEnabled" << this->Enabled << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers,str,0);
    } 
}


//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::AddToRenderModule(vtkSMRenderModuleProxy* rm)
{
  this->SetInteractor(this->GetInteractorProxy(rm));
  this->SetCurrentRenderer(this->GetRendererProxy(rm));
  this->SetCurrentRenderModuleProxy(rm);
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::RemoveFromRenderModule(vtkSMRenderModuleProxy* rm)
{
  if (this->CurrentRenderModuleProxy == rm )
    {
    this->SetInteractor(0);
    this->SetCurrentRenderer(0);
    this->SetCurrentRenderModuleProxy(0);
    }
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::InitializeObservers(vtk3DWidget* wdg) 
{
  if(wdg)
    {
    wdg->AddObserver(vtkCommand::InteractionEvent, this->Observer);
    wdg->AddObserver(vtkCommand::StartInteractionEvent, this->Observer);
    wdg->AddObserver(vtkCommand::EndInteractionEvent, this->Observer);
    wdg->AddObserver(vtkCommand::PlaceWidgetEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::UpdateVTKObjects()
{
  this->Placed = 1;
  this->Superclass::UpdateVTKObjects();
  if (!this->Placed)
    {
    // We send a PlaceWidget message only when the bounds have been 
    // changed (achieved by the this->Placed flag).
    unsigned int cc;
    vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    for(cc=0; cc < this->GetNumberOfIDs(); cc++)
      {
      stream << vtkClientServerStream::Invoke << this->GetID(cc)
             << "PlaceWidget" 
             << this->Bounds[0] << this->Bounds[1] << this->Bounds[2] 
             << this->Bounds[3] 
             << this->Bounds[4] << this->Bounds[5] << vtkClientServerStream::End;
      pm->SendStream(this->Servers, stream);
      } 
    this->Placed = 1;
    }
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::PlaceWidget(double bds[6])
{
  if (this->Bounds[0] == bds[0] &&
    this->Bounds[1] == bds[1] &&
    this->Bounds[2] == bds[2] &&
    this->Bounds[3] == bds[3] &&
    this->Bounds[4] == bds[4] &&
    this->Bounds[5] == bds[5])
    {
    return;
    }

  this->Bounds[0] = bds[0];
  this->Bounds[1] = bds[1];
  this->Bounds[2] = bds[2];
  this->Bounds[3] = bds[3];
  this->Bounds[4] = bds[4];
  this->Bounds[5] = bds[5];

  this->Placed = 0;
  
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::CreateVTKObjects(int numObjects)
{
  if(this->ObjectsCreated)
    {
    return;
    }
  //Superclass creates the actual VTK objects
  this->Superclass::CreateVTKObjects(numObjects);
  
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  unsigned int cc;
  //additional initialization 
  for (cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    vtk3DWidget* widget = vtk3DWidget::SafeDownCast(
      pm->GetObjectFromID(this->GetID(cc)));
    this->InitializeObservers(widget);
    }

  vtkClientServerStream stream;
  for (cc=0; cc <this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    stream << vtkClientServerStream::Invoke << id
           << "SetPlaceFactor" << 1.0
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke << id 
           << "PlaceWidget"
           << 0 << 1 << 0 << 1 << 0 << 1 
           << vtkClientServerStream::End;
    // this->Bounds have already been initialized to 0,1,0,1,0,1
    pm->SendStream(this->GetServers(), stream);
    }
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::ExecuteEvent(vtkObject*, unsigned long event, void*)
{
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent);
  vtkPVGenericRenderWindowInteractor* iren = 0;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->CurrentRenderModuleProxy)
    {
    iren = vtkPVGenericRenderWindowInteractor::SafeDownCast(
      pm->GetObjectFromID( 
        this->GetInteractorProxy(this->CurrentRenderModuleProxy)->GetID(0)));
    }
  if ( event == vtkCommand::StartInteractionEvent && iren)
    {
    iren->InteractiveRenderEnabledOn();
    }
  else if ( event == vtkCommand::EndInteractionEvent && iren)
    {
    this->UpdateVTKObjects();
    iren->InteractiveRenderEnabledOff();
    }
  else if ( event == vtkCommand::PlaceWidgetEvent )
    {
    this->InvokeEvent(vtkCommand::PlaceWidgetEvent);
    }
  else
    {
    // So the the client object changes are sent over to the Servers
    this->UpdateVTKObjects();
    }

  if (iren)
    {
    iren->Render();
    }
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::SetCurrentRenderer(vtkSMProxy *renderer)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID null = {0 };
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    stream << vtkClientServerStream::Invoke << this->GetID(cc)
           << "SetCurrentRenderer" 
           << ( (renderer)? renderer->GetID(0) : null )
           << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream, 1);
    }
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::SetInteractor(vtkSMProxy* interactor)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID null = {0 };
  for(unsigned int cc=0; cc < this->GetNumberOfIDs(); cc++)
    {
    stream << vtkClientServerStream::Invoke << this->GetID(cc)
           << "SetInteractor" 
           << ((interactor)? interactor->GetID(0) : null)
           << vtkClientServerStream::End;
    pm->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER, stream, 1);
    } 
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::SetCurrentRenderModuleProxy(
  vtkSMRenderModuleProxy* rm)
{
  if (this->CurrentRenderModuleProxy && rm != this->CurrentRenderModuleProxy
    && rm)
    {
    vtkErrorMacro("CurrentRenderModuleProxy already set.");
    return;
    }
  this->CurrentRenderModuleProxy = rm;
  // since enabling is delayed until CurrentRenderModule is set,
  // we must update the widget enable state once rendermodule is set.
  // if rm==NULL, this will automatically disable the 3D widget.
  this->SetEnabled(this->Enabled);
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::SaveInBatchScript(ofstream *file)
{
  for (unsigned int cc=0;cc < this->GetNumberOfIDs(); cc++)
    {
    vtkClientServerID id = this->GetID(cc);
    *file << endl;
    *file << "set pvTemp" << id.ID
      <<" [$proxyManager NewProxy 3d_widgets " 
      << this->GetXMLName()
      << "]"
      <<endl;
    *file << "  $proxyManager RegisterProxy 3d_widgets pvTemp"
      << id.ID << " $pvTemp" << id.ID
      << endl;
    *file << "  $pvTemp" << id.ID << " UnRegister {}" << endl;

    *file << "  [$pvTemp" << id.ID << " GetProperty IgnorePlaceWidgetChanges]"
      << " SetElements1 0" << endl;
    for(int i=0;i < 6; i++)
      {
      *file << "  [$pvTemp" << id.ID << " GetProperty PlaceWidget] "
        << "SetElement " << i << " "
        << this->Bounds[i] 
        << endl;
      }

    *file << "  [$pvTemp" << id.ID << " GetProperty Visibility] "
      << "SetElements1 " << this->Enabled << endl;

    *file << "  $pvTemp" << id.ID << " UpdateVTKObjects" << endl;
    *file << endl;
    }

}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::SaveState(const char* name, ostream* file,
  vtkIndent indent)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("IgnorePlaceWidgetChanges"));
  int old_ipc = ivp->GetElement(0);
  ivp->SetElement(0, 1);
  this->Superclass::SaveState(name, file, indent);
  ivp->SetElement(0, old_ipc);
}

//----------------------------------------------------------------------------
void vtkSM3DWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "CurrentRenderModuleProxy: " << 
    this->CurrentRenderModuleProxy << endl;  
  os << indent << "IgnorePlaceWidgetChanges: " << 
    this->IgnorePlaceWidgetChanges << endl;
}
