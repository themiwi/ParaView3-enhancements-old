/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMExtractLocationsProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtractLocationsProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSelection.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkDoubleArray.h"

vtkStandardNewMacro(vtkSMExtractLocationsProxy);
vtkCxxRevisionMacro(vtkSMExtractLocationsProxy, "$Revision: 1.1 $");
//-----------------------------------------------------------------------------
vtkSMExtractLocationsProxy::vtkSMExtractLocationsProxy()
{
  this->SelectionFieldType = vtkSelection::CELL;
  this->Locations = NULL;
}

//-----------------------------------------------------------------------------
vtkSMExtractLocationsProxy::~vtkSMExtractLocationsProxy()
{
  if (this->Locations)
    {
    this->Locations->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::AddLocation(double x, double y, double z)
{
  if (this->Locations == NULL)
    {
    this->Locations = vtkDoubleArray::New();
    this->Locations->SetNumberOfComponents(3);
    this->Locations->SetNumberOfTuples(0);
    }
  this->Locations->InsertNextTuple3(x,y,z);
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::RemoveAllLocations()
{
  if (this->Locations)
    {
    this->Locations->Reset();
    }
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::CreateVTKObjects(int num)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects(num);

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkSMSourceProxy* selectionSource = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("SelectionSource"));
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }
  
  this->AddInput(selectionSource, "SetSelectionConnection", false);
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  vtkSMProxy* selectionSource = this->GetSubProxy("SelectionSource");
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    selectionSource->GetProperty("Locations"));
  int nlocations = 0;
  if (this->Locations)
    {
    nlocations = this->Locations->GetNumberOfTuples();
    }
  dvp->SetNumberOfElements(nlocations*3);
  if (nlocations)
    {
    dvp->SetElements((double*)this->Locations->GetVoidPointer(0));
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("FieldType"));
  ivp->SetElement(0, this->SelectionFieldType);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("ContentType"));
  ivp->SetElement(0, vtkSelection::LOCATIONS);

  selectionSource->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMExtractLocationsProxy::ReadXMLAttributes(
  vtkSMProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
    {
    return 0;
    }
  const char* type = element->GetAttribute("selection_field_type");
  if (type && strcmp(type,"POINT") == 0)
    {
    this->SelectionFieldType  = vtkSelection::POINT;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMExtractLocationsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SelectionFieldType: " << this->SelectionFieldType << endl;
}
