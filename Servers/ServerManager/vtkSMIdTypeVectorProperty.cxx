/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMIdTypeVectorProperty.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIdTypeVectorProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMIdTypeVectorProperty);
vtkCxxRevisionMacro(vtkSMIdTypeVectorProperty, "$Revision: 1.9 $");

struct vtkSMIdTypeVectorPropertyInternals
{
  vtkstd::vector<vtkIdType> Values;
  vtkstd::vector<vtkIdType> UncheckedValues;
};

//---------------------------------------------------------------------------
vtkSMIdTypeVectorProperty::vtkSMIdTypeVectorProperty()
{
  this->Internals = new vtkSMIdTypeVectorPropertyInternals;
  this->ArgumentIsArray = 0;
}

//---------------------------------------------------------------------------
vtkSMIdTypeVectorProperty::~vtkSMIdTypeVectorProperty()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMIdTypeVectorProperty::AppendCommandToStream(
  vtkSMProxy*, vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command || this->InformationOnly)
    {
    return;
    }

  if (this->CleanCommand)
    {
    *str << vtkClientServerStream::Invoke
      << objectId << this->CleanCommand
      << vtkClientServerStream::End;
    }
  
  if (!this->RepeatCommand)
    {
    *str << vtkClientServerStream::Invoke << objectId << this->Command;
    int numArgs = this->GetNumberOfElements();
    if (this->ArgumentIsArray)
      {
      *str << vtkClientServerStream::InsertArray(
        &(this->Internals->Values[0]), numArgs);
      }
    else
      {
      for(int i=0; i<numArgs; i++)
        {
        *str << this->GetElement(i);
        }
      }
    *str << vtkClientServerStream::End;
    }
  else
    {
    int numArgs = this->GetNumberOfElements();
    int numCommands = numArgs / this->NumberOfElementsPerCommand;
    for(int i=0; i<numCommands; i++)
      {
      *str << vtkClientServerStream::Invoke << objectId << this->Command;
      if (this->UseIndex)
        {
        *str << i;
        }
      if (this->ArgumentIsArray)
        {
        *str << vtkClientServerStream::InsertArray(
          &(this->Internals->Values[i*this->NumberOfElementsPerCommand]),
          this->NumberOfElementsPerCommand);
        }
      else
        {
        for (int j=0; j<this->NumberOfElementsPerCommand; j++)
          {
          *str << this->GetElement(i*this->NumberOfElementsPerCommand+j);
          }
        }
      *str << vtkClientServerStream::End;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMIdTypeVectorProperty::SetNumberOfUncheckedElements(unsigned int num)
{
  this->Internals->UncheckedValues.resize(num);
}

//---------------------------------------------------------------------------
void vtkSMIdTypeVectorProperty::SetNumberOfElements(unsigned int num)
{
  this->Internals->Values.resize(num);
  this->Internals->UncheckedValues.resize(num);
  this->Modified();
}

//---------------------------------------------------------------------------
unsigned int vtkSMIdTypeVectorProperty::GetNumberOfUncheckedElements()
{
  return this->Internals->UncheckedValues.size();
}

//---------------------------------------------------------------------------
unsigned int vtkSMIdTypeVectorProperty::GetNumberOfElements()
{
  return this->Internals->Values.size();
}

//---------------------------------------------------------------------------
vtkIdType vtkSMIdTypeVectorProperty::GetElement(unsigned int idx)
{
  return this->Internals->Values[idx];
}

//---------------------------------------------------------------------------
vtkIdType vtkSMIdTypeVectorProperty::GetUncheckedElement(unsigned int idx)
{
  return this->Internals->UncheckedValues[idx];
}

//---------------------------------------------------------------------------
void vtkSMIdTypeVectorProperty::SetUncheckedElement(
  unsigned int idx, vtkIdType value)
{
  if (idx >= this->GetNumberOfUncheckedElements())
    {
    this->SetNumberOfUncheckedElements(idx+1);
    }
  this->Internals->UncheckedValues[idx] = value;
}

//---------------------------------------------------------------------------
int vtkSMIdTypeVectorProperty::SetElement(unsigned int idx, vtkIdType value)
{
  if ( vtkSMProperty::GetCheckDomains() )
    {
    int numArgs = this->GetNumberOfElements();
    memcpy(&this->Internals->UncheckedValues[0], 
           &this->Internals->Values[0], 
           numArgs*sizeof(int));
    
    this->SetUncheckedElement(idx, value);
    if (!this->IsInDomains())
      {
      this->SetNumberOfUncheckedElements(this->GetNumberOfElements());
      return 0;
      }
    }
  
  if (idx >= this->GetNumberOfElements())
    {
    this->SetNumberOfElements(idx+1);
    }
  this->Internals->Values[idx] = value;
  this->Modified();
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMIdTypeVectorProperty::SetElements1(vtkIdType value0)
{
  return this->SetElement(0, value0);
}

//---------------------------------------------------------------------------
int vtkSMIdTypeVectorProperty::SetElements2(vtkIdType value0, vtkIdType value1)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  return (retVal1 && retVal2);
}

//---------------------------------------------------------------------------
int vtkSMIdTypeVectorProperty::SetElements3(vtkIdType value0, 
                                            vtkIdType value1, 
                                            vtkIdType value2)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  int retVal3 = this->SetElement(2, value2);
  return (retVal1 && retVal2 && retVal3);
}

//---------------------------------------------------------------------------
int vtkSMIdTypeVectorProperty::SetElements(const vtkIdType* values)
{
  int numArgs = this->GetNumberOfElements();

  if ( vtkSMProperty::GetCheckDomains() )
    {
    memcpy(&this->Internals->UncheckedValues[0], 
           values, 
           numArgs*sizeof(vtkIdType));
    if (!this->IsInDomains())
      {
      return 0;
      }
    }

  memcpy(&this->Internals->Values[0], values, numArgs*sizeof(vtkIdType));
  this->Modified();
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMIdTypeVectorProperty::ReadXMLAttributes(vtkSMProxy* parent,
                                                 vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(parent, element);
  if (!retVal)
    {
    return retVal;
    }

  int arg_is_array;
  retVal = element->GetScalarAttribute("argument_is_array", &arg_is_array);
  if(retVal) 
    { 
    this->SetArgumentIsArray(arg_is_array); 
    }

  int numElems = this->GetNumberOfElements();
  if (numElems > 0)
    {
    int* initVal = new int[numElems];
    int numRead = element->GetVectorAttribute("default_values",
                                              numElems,
                                              initVal);

    if (numRead > 0)
      {
      if (numRead != numElems)
        {
        vtkErrorMacro("The number of default values does not match the number "
                      "of elements. Initialization failed.");
        delete[] initVal;
        return 0;
        }
      for(int i=0; i<numRead; i++)
        {
        this->SetElement(i, initVal[i]);
        }
      }
    else
      {
      vtkErrorMacro("No default value is specified for property: "
                    << this->GetXMLName()
                    << ". This might lead to stability problems");
      }
    delete[] initVal;
    }

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMIdTypeVectorProperty::LoadState(vtkPVXMLElement* element,
                                         vtkSMStateLoader* loader)
{
  int prevImUpdate = this->ImmediateUpdate;

  // Wait until all values are set before update (if ImmediateUpdate)
  this->ImmediateUpdate = 0;
  this->Superclass::LoadState(element, loader);

  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "Element") == 0)
      {
      int index;
      if (currentElement->GetScalarAttribute("index", &index))
        {
        vtkIdType value;
        if (currentElement->GetScalarAttribute("value", &value))
          {
          this->SetElement(index, value);
          }
        }
      }
    }

  // Do not immediately update. Leave it to the loader.
  this->Modified();
  this->ImmediateUpdate = prevImUpdate;

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMIdTypeVectorProperty::ChildSaveState(vtkPVXMLElement* propertyElement)
{
  this->Superclass::ChildSaveState(propertyElement);

  unsigned int size = this->GetNumberOfElements();
  if (size > 0)
    {
    propertyElement->AddAttribute("number_of_elements", size);
    }
  for (unsigned int i=0; i<size; i++)
    {
    vtkPVXMLElement* elementElement = vtkPVXMLElement::New();
    elementElement->SetName("Element");
    elementElement->AddAttribute("index", i);
    elementElement->AddAttribute("value", this->GetElement(i));
    propertyElement->AddNestedElement(elementElement);
    elementElement->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkSMIdTypeVectorProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  vtkSMIdTypeVectorProperty* dsrc = vtkSMIdTypeVectorProperty::SafeDownCast(
    src);
  if (dsrc)
    {
    int imUpdate = this->ImmediateUpdate;
    this->ImmediateUpdate = 0;
    this->SetNumberOfElements(dsrc->GetNumberOfElements());
    this->SetNumberOfUncheckedElements(dsrc->GetNumberOfUncheckedElements());
    memcpy(&this->Internals->Values[0], 
           &dsrc->Internals->Values[0], 
           this->GetNumberOfElements()*sizeof(vtkIdType));
    memcpy(&this->Internals->UncheckedValues[0], 
           &dsrc->Internals->UncheckedValues[0], 
           this->GetNumberOfUncheckedElements()*sizeof(vtkIdType));
    this->ImmediateUpdate = imUpdate;
    }

  this->Modified();
}

//---------------------------------------------------------------------------
void vtkSMIdTypeVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ArgumentIsArray: " << this->ArgumentIsArray << endl;
  os << indent << "Values: ";
  for (unsigned int i=0; i<this->GetNumberOfElements(); i++)
    {
    os << this->GetElement(i) << " ";
    }
  os << endl;
}
