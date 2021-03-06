/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSimpleIntInformationHelper.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSimpleIntInformationHelper.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMSimpleIntInformationHelper);
vtkCxxRevisionMacro(vtkSMSimpleIntInformationHelper, "$Revision: 1.7 $");

//---------------------------------------------------------------------------
vtkSMSimpleIntInformationHelper::vtkSMSimpleIntInformationHelper()
{
}

//---------------------------------------------------------------------------
vtkSMSimpleIntInformationHelper::~vtkSMSimpleIntInformationHelper()
{
}

//---------------------------------------------------------------------------
void vtkSMSimpleIntInformationHelper::UpdateProperty(
  vtkIdType connectionId, int serverIds, vtkClientServerID objectId, 
  vtkSMProperty* prop)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (!ivp)
    {
    vtkErrorMacro("A null property or a property of a different type was "
                  "passed when vtkSMIntVectorProperty was needed.");
    return;
    }

  if (!prop->GetCommand())
    {
    return;
    }

  // Invoke property's method on the root node of the server
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke 
      << objectId << prop->GetCommand()
      << vtkClientServerStream::End;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str);

  // Get the result
  const vtkClientServerStream& res = 
    pm->GetLastResult(connectionId, vtkProcessModule::GetRootId(serverIds));

  int numMsgs = res.GetNumberOfMessages();
  if (numMsgs < 1)
    {
    return;
    }

  int numArgs = res.GetNumberOfArguments(0);
  if (numArgs < 1)
    {
    return;
    }

  int argType = res.GetArgumentType(0, 0);

  // If single value, all int types
  if (argType == vtkClientServerStream::int32_value ||
      argType == vtkClientServerStream::int16_value ||
      argType == vtkClientServerStream::int8_value ||
      argType == vtkClientServerStream::bool_value)
    {
    int ires;
    int retVal = res.GetArgument(0, 0, &ires);
    if (!retVal)
      {
      vtkErrorMacro("Error getting argument.");
      return;
      }
    ivp->SetNumberOfElements(1);
    ivp->SetElement(0, ires);
    }
  // if array, only 32 bit ints work
  else if (argType == vtkClientServerStream::int32_array)
    {
    vtkTypeUInt32 length;
    res.GetArgumentLength(0, 0, &length);
    if (length >= 128)
      {
      vtkErrorMacro("Only arguments of length 128 or less are supported");
      return;
      }
    int values[128];
    int retVal = res.GetArgument(0, 0, values, length);
    if (!retVal)
      {
      vtkErrorMacro("Error getting argument.");
      return;
      }
    ivp->SetNumberOfElements(length);
    ivp->SetElements(values);
    }
}

//---------------------------------------------------------------------------
void vtkSMSimpleIntInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
