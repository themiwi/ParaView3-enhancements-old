/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"

vtkCxxRevisionMacro(vtkSMRepresentationProxy, "$Revision: 1.2 $");
//----------------------------------------------------------------------------
vtkSMRepresentationProxy::vtkSMRepresentationProxy()
{
  this->SelectionSupported = false;
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy::~vtkSMRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  if (!this->BeginCreateVTKObjects(numObjects))
    {
    // BeginCreateVTKObjects() requested an abortion of VTK object creation.
    return;
    }

  this->Superclass::CreateVTKObjects(numObjects);
  this->EndCreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
bool vtkSMRepresentationProxy::GetVisibility()
{
  if (!this->ObjectsCreated)
    {
    return false;
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Visibility"));
  if (ivp && ivp->GetNumberOfElements()== 1 && ivp->GetElement(0))
    {
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMRepresentationProxy::GetSelectionVisibility()
{
  if (!this->GetVisibility() || !this->GetSelectionSupported())
    {
    return false;
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("SelectionVisibility"));
  if (ivp && ivp->GetNumberOfElements()== 1 && ivp->GetElement(0))
    {
    return true;
    }

  return false;
}


//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::Connect(vtkSMProxy* producer,
  vtkSMProxy* consumer, const char* propertyname/*="Input"*/)
{
  if (!propertyname)
    {
    vtkErrorMacro("propertyname cannot be NULL.");
    return;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    consumer->GetProperty(propertyname));
  if (!pp)
    {
    vtkErrorMacro("Failed to locate property " << propertyname
      << " on the consumer " << consumer->GetXMLName());
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(producer);
  consumer->UpdateProperty(propertyname);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SelectionSupported : " << this->SelectionSupported << endl;
}


