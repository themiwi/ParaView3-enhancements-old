/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSphereWidgetProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSphereWidgetProxy.h"

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSphereWidget.h"

vtkStandardNewMacro(vtkSMSphereWidgetProxy);
vtkCxxRevisionMacro(vtkSMSphereWidgetProxy, "$Revision: 1.15 $");

//----------------------------------------------------------------------------
vtkSMSphereWidgetProxy::vtkSMSphereWidgetProxy()
{
  this->Radius = 0.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->SetVTKClassName("vtkSphereWidget");
}

//----------------------------------------------------------------------------
vtkSMSphereWidgetProxy::~vtkSMSphereWidgetProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMSphereWidgetProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  vtkClientServerID id = this->GetID();
  str << vtkClientServerStream::Invoke << id
      << "SetCenter" 
      << this->Center[0]
      << this->Center[1]
      << this->Center[2]
      << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke << id
      << "SetRadius" << this->Radius
      << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID ,this->Servers,str);
}

//----------------------------------------------------------------------------
void vtkSMSphereWidgetProxy::ExecuteEvent(vtkObject *wdg, unsigned long event,void *p)
{
  vtkSphereWidget *widget = vtkSphereWidget::SafeDownCast(wdg);
  if ( !widget )
    {
    return;
    }
  //Update iVars to reflect the state of the VTK object
  double val[3];
  double rad = widget->GetRadius();
  widget->GetCenter(val); 
  
  if (event != vtkCommand::PlaceWidgetEvent || !this->IgnorePlaceWidgetChanges)
    {
    this->SetCenter(val);
    vtkSMDoubleVectorProperty* cp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("Center"));
    if (cp)
      {
      cp->SetElements(val);
      }
    this->SetRadius(rad);
    vtkSMDoubleVectorProperty* rp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->GetProperty("Radius"));
    if (rp)
      {
      rp->SetElements1(rad);
      }
    }
  this->Superclass::ExecuteEvent(wdg, event, p);
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSphereWidgetProxy::SaveState(vtkPVXMLElement* root)
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Center"));
  if (dvp)
    {
    dvp->SetElements(this->Center);
    }
    
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("Radius"));
  if (dvp)
    {
    dvp->SetElements1(this->Radius);
    }
  return this->Superclass::SaveState(root);
}

//----------------------------------------------------------------------------
void vtkSMSphereWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Center: " << this->Center[0]
        << ", " << this->Center[1] << ", " <<this->Center[2] << endl;
  os << indent << "Radius: " << this->Radius << endl;
}
