/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkGenericDataSetToPolyDataFilter.cxx,v $
  Language:  C++
  Date:      $Date: 2004-08-20 13:39:36 $
  Version:   $Revision: 1.1 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericDataSetToPolyDataFilter.h"

#include "vtkGenericDataSet.h"

vtkCxxRevisionMacro(vtkGenericDataSetToPolyDataFilter, "$Revision: 1.1 $");

//----------------------------------------------------------------------------
void vtkGenericDataSetToPolyDataFilter::PrintSelf(ostream& os,
                                                  vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkGenericDataSetToPolyDataFilter::SetInput(vtkGenericDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkGenericDataSet *vtkGenericDataSetToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return static_cast<vtkGenericDataSet *>(this->Inputs[0]);
}


//----------------------------------------------------------------------------
// Copy the update information across
void vtkGenericDataSetToPolyDataFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataObject *input = this->GetInput();

  if (input == NULL)
    {
    return;
    }
  
  this->vtkPolyDataSource::ComputeInputUpdateExtents(output);
  input->RequestExactExtentOn();
}
