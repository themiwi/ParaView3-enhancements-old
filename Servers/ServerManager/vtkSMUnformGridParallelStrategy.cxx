/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMUnformGridParallelStrategy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUnformGridParallelStrategy.h"

#include "vtkObjectFactory.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMUnformGridParallelStrategy);
vtkCxxRevisionMacro(vtkSMUnformGridParallelStrategy, "$Revision: 1.2 $");
//----------------------------------------------------------------------------
vtkSMUnformGridParallelStrategy::vtkSMUnformGridParallelStrategy()
{
  this->Collect = 0;
}

//----------------------------------------------------------------------------
vtkSMUnformGridParallelStrategy::~vtkSMUnformGridParallelStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMUnformGridParallelStrategy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Collect = vtkSMSourceProxy::SafeDownCast(
    this->GetSubProxy("Collect"));

  this->Collect->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);

  this->Superclass::CreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMUnformGridParallelStrategy::CreatePipeline(vtkSMSourceProxy* input)
{
  this->Connect(input, this->Collect);

  this->Superclass::CreatePipeline(this->Collect);
}

//----------------------------------------------------------------------------
void vtkSMUnformGridParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


