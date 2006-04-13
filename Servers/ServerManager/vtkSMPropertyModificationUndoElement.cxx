/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPropertyModificationUndoElement.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPropertyModificationUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDefaultStateLoader.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkSMPropertyModificationUndoElement);
vtkCxxRevisionMacro(vtkSMPropertyModificationUndoElement, "$Revision: 1.1 $");
vtkCxxSetObjectMacro(vtkSMPropertyModificationUndoElement, XMLElement,
  vtkPVXMLElement);
//-----------------------------------------------------------------------------
vtkSMPropertyModificationUndoElement::vtkSMPropertyModificationUndoElement()
{
  this->XMLElement = 0;
}

//-----------------------------------------------------------------------------
vtkSMPropertyModificationUndoElement::~vtkSMPropertyModificationUndoElement()
{
  this->SetXMLElement(0);
}

//-----------------------------------------------------------------------------
int vtkSMPropertyModificationUndoElement::Undo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No State present to undo.");
    return 0;
    }
  int proxy_id;
  this->XMLElement->GetScalarAttribute("id", &proxy_id);
  const char* property_name = this->XMLElement->GetAttribute("name");
  vtkSMDefaultStateLoader* stateLoader = vtkSMDefaultStateLoader::New();

  vtkSMProxy* proxy = stateLoader->NewProxy(proxy_id);
  vtkSMProperty* property = (proxy? proxy->GetProperty(property_name): NULL);
  if (property)
    {
    return property->LoadState(this->XMLElement->GetNestedElement(0),  
      stateLoader, 1);
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkSMPropertyModificationUndoElement::Redo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No State present to redo.");
    return 0;
    }
  int proxy_id;
  this->XMLElement->GetScalarAttribute("id", &proxy_id);
  const char* property_name = this->XMLElement->GetAttribute("name");
  vtkSMDefaultStateLoader* stateLoader = vtkSMDefaultStateLoader::New();

  vtkSMProxy* proxy = stateLoader->NewProxy(proxy_id);
  vtkSMProperty* property = (proxy? proxy->GetProperty(property_name): NULL);
  if (property)
    {
    return property->LoadState(this->XMLElement->GetNestedElement(0),  
      stateLoader, 0);
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSMPropertyModificationUndoElement::SaveStateInternal(vtkPVXMLElement* root)
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No state present to save.");
    return;
    }
  root->AddNestedElement(this->XMLElement);
}

//-----------------------------------------------------------------------------
void vtkSMPropertyModificationUndoElement::LoadStateInternal(
  vtkPVXMLElement* pmElement)
{
  if (strcmp(pmElement->GetName(), "PropertyModification") != 0)
    {
    vtkErrorMacro("Invalid element name: " << pmElement->GetName()
      << ". Must be PropertyModification.");
    return;
    }
  this->SetXMLElement(pmElement);
}

//-----------------------------------------------------------------------------
void vtkSMPropertyModificationUndoElement::ModifiedProperty(vtkSMProxy* proxy, 
  const char* propertyname)
{
  vtkSMProperty* property = proxy->GetProperty(propertyname);
  if (!property)
    {
    vtkErrorMacro("Failed to locate property with name : " << propertyname
      << " on the proxy. Cannot note its modification state for undo/redo.");
    return;
    }
  
  vtkPVXMLElement* pmElement = vtkPVXMLElement::New();
  pmElement->SetName("PropertyModification");
  pmElement->AddAttribute("id", proxy->GetSelfIDAsString());
  pmElement->AddAttribute("name", propertyname);

  property->SaveState(pmElement, propertyname, proxy->GetSelfIDAsString(),
    /*saveDomains=*/0, /*saveLastPushedValues=*/1);

  this->SetXMLElement(pmElement);
  pmElement->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMPropertyModificationUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "XMLElement: " << this->XMLElement << endl;
}
