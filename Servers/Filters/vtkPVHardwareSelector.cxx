/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVHardwareSelector.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVHardwareSelector.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProp.h"

vtkStandardNewMacro(vtkPVHardwareSelector);
vtkCxxRevisionMacro(vtkPVHardwareSelector, "$Revision: 1.1 $");
//----------------------------------------------------------------------------
vtkPVHardwareSelector::vtkPVHardwareSelector()
{
  this->NumberOfProcesses = 1;
  this->NumberOfIDs = 0;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm->GetNumberOfLocalPartitions() > 1)
    {
    this->ProcessID = pm->GetPartitionId();
    }
}

//----------------------------------------------------------------------------
vtkPVHardwareSelector::~vtkPVHardwareSelector()
{
}

//----------------------------------------------------------------------------
bool vtkPVHardwareSelector::PassRequired(int pass)
{
  switch (pass)
    {
  case PROCESS_PASS:
    return (this->NumberOfProcesses > 1);

  case ID_MID24:
    return (this->NumberOfIDs >= 0xffffff);

  case ID_HIGH16:
    return (this->NumberOfIDs >= 0xffffffffffff);
    }
  return true;
}

//----------------------------------------------------------------------------
int vtkPVHardwareSelector::GetPropID(int idx, vtkProp* prop)
{
  vtkClientServerID csId = 
    vtkProcessModule::GetProcessModule()->GetIDFromObject(prop);
  return static_cast<int>(csId.ID);
}

//----------------------------------------------------------------------------
void vtkPVHardwareSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


