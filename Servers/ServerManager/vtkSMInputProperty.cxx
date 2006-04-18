/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMInputProperty.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInputProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMInputProperty);
vtkCxxRevisionMacro(vtkSMInputProperty, "$Revision: 1.13 $");

int vtkSMInputProperty::InputsUpdateImmediately = 1;

//---------------------------------------------------------------------------
vtkSMInputProperty::vtkSMInputProperty()
{
  this->ImmediateUpdate = vtkSMInputProperty::InputsUpdateImmediately;
  this->UpdateSelf = 1;
  this->MultipleInput = 0;
}

//---------------------------------------------------------------------------
vtkSMInputProperty::~vtkSMInputProperty()
{
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::GetInputsUpdateImmediately()
{
  return vtkSMInputProperty::InputsUpdateImmediately;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetInputsUpdateImmediately(int up)
{
  vtkSMInputProperty::InputsUpdateImmediately = up;

  vtkSMPropertyIterator* piter = vtkSMPropertyIterator::New();
  vtkSMProxyIterator* iter = vtkSMProxyIterator::New();
  while(!iter->IsAtEnd())
    {
    piter->SetProxy(iter->GetProxy());
    while(!piter->IsAtEnd())
      {
      vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
        piter->GetProperty());
      if (ip)
        {
        ip->SetImmediateUpdate(up);
        }
      piter->Next();
      }
    iter->Next();
    }
  iter->Delete();
  piter->Delete();
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::AppendCommandToStream(
  vtkSMProxy* cons, vtkClientServerStream* str, vtkClientServerID objectId )
{
  if (!this->Command || this->InformationOnly)
    {
    return;
    }

  this->RemoveConsumerFromPreviousProxies(cons);
  this->RemoveAllPreviousProxies();

  if (this->CleanCommand)
    {
    *str << vtkClientServerStream::Invoke 
         << objectId << "CleanInputs" << this->CleanCommand
         << vtkClientServerStream::End;
    }
  unsigned int numInputs = this->GetNumberOfProxies();
  for (unsigned int i=0; i<numInputs; i++)
    {
    vtkSMProxy* proxy = this->GetProxy(i) ;
    if (proxy)
      {
      this->AddPreviousProxy(proxy);
      proxy->AddConsumer(this, cons);

      *str << vtkClientServerStream::Invoke 
           << objectId 
           << "AddInput" 
           << proxy
           << this->Command;
      if (this->MultipleInput)
        {
        *str << 1;
        }
      else
        {
        *str << 0;
        }
      *str << vtkClientServerStream::End;
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::ReadXMLAttributes(vtkSMProxy* parent,
                                          vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(parent, element);

  int multiple_input;
  int retVal = element->GetScalarAttribute("multiple_input", &multiple_input);
  if(retVal) 
    { 
    this->SetMultipleInput(multiple_input); 
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MultipleInput: " << this->MultipleInput << endl;
  os << indent << "PortIndex: " << this->PortIndex << endl;
}
