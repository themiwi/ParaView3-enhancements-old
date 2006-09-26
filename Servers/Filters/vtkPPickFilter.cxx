/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPPickFilter.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPPickFilter.h"

#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

vtkStandardNewMacro(vtkPPickFilter);
vtkCxxRevisionMacro(vtkPPickFilter, "$Revision: 1.3 $");
//-----------------------------------------------------------------------------
vtkPPickFilter::vtkPPickFilter()
{
}

//-----------------------------------------------------------------------------
vtkPPickFilter::~vtkPPickFilter()
{
}

//-----------------------------------------------------------------------------
void vtkPPickFilter::IdExecute(vtkInformationVector **inputVector,
                               vtkInformationVector *outputVector)
{
  this->Superclass::IdExecute(inputVector, outputVector);
  
  // Collect the data on root node.
  if (!this->Controller)
    {
    return;
    }
  int procid = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  if (numProcs <= 1)
    {
    return;
    }
  
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector);
  int numPoints = output->GetNumberOfPoints();
  int dataReceived = numPoints;
  if (procid > 0)
    {
    // satellites --- send data to root.
    this->Controller->Send(&numPoints, 1, 0, 1020);
    if (numPoints>0)
      {
      this->Controller->Send(this->GetOutput(), 0, 1021);
      }
    this->GetOutput()->ReleaseData();
    }
  else
    {
    // root --  collect the data from satellites.
    for (int i=1; i < numProcs; i++)
      {
      int count =0;
      this->Controller->Receive(&count, 1, i, 1020);
      if (count > 0)
        {
        vtkUnstructuredGrid* tmp = vtkUnstructuredGrid::New();
        this->Controller->Receive(tmp, i,  1021);
        if (!dataReceived)
          {
          output->CopyStructure(tmp);
          output->GetPointData()->PassData(tmp->GetPointData());
          output->GetCellData()->PassData(tmp->GetCellData());
          output->GetFieldData()->PassData(tmp->GetFieldData());
          dataReceived = 1;
          }
        tmp->Delete();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPPickFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
