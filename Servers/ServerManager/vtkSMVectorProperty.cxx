/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMVectorProperty.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMVectorProperty.h"

#include "vtkPVXMLElement.h"

vtkCxxRevisionMacro(vtkSMVectorProperty, "$Revision: 1.1 $");

//---------------------------------------------------------------------------
vtkSMVectorProperty::vtkSMVectorProperty()
{
  this->RepeatCommand = 0;
  this->NumberOfElementsPerCommand = 1;
  this->UseIndex = 0;
}

//---------------------------------------------------------------------------
vtkSMVectorProperty::~vtkSMVectorProperty()
{
}

//---------------------------------------------------------------------------
int vtkSMVectorProperty::ReadXMLAttributes(vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(element);
  if (!retVal)
    {
    return retVal;
    }

  int use_index;
  retVal = element->GetScalarAttribute("use_index", &use_index);
  if(retVal) 
    { 
    this->SetUseIndex(use_index); 
    }
  int repeat_command;
  retVal = element->GetScalarAttribute("repeat_command", &repeat_command);
  if(retVal) 
    { 
    this->SetRepeatCommand(repeat_command); 
    }
  int numElsPerCommand;
  retVal = 
    element->GetScalarAttribute(
      "number_of_elements_per_command", &numElsPerCommand);
  if (retVal)
    {
    this->SetNumberOfElementsPerCommand(numElsPerCommand);
    }
  int numEls;
  retVal = element->GetScalarAttribute("number_of_elements", &numEls);
  if (retVal)
    {
    this->SetNumberOfElements(numEls);
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "NumberOfElements: " << this->GetNumberOfElements() << endl;
  os << indent << "NumberOfElementsPerCommand: " 
     << this->GetNumberOfElementsPerCommand() << endl;
  os << indent << "RepeatCommand: " << this->RepeatCommand << endl;
}
