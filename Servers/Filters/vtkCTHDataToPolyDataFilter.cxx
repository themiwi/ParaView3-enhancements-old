/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkCTHDataToPolyDataFilter.cxx,v $
  Language:  C++
  Date:      $Date: 2003-09-05 20:07:52 $
  Version:   $Revision: 1.1 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHDataToPolyDataFilter.h"

#include "vtkCTHData.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkCTHDataToPolyDataFilter, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkCTHDataToPolyDataFilter);

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkCTHDataToPolyDataFilter::SetInput(vtkCTHData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkCTHData *vtkCTHDataToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkCTHData *)(this->Inputs[0]);
}


//----------------------------------------------------------------------------
// Copy the update information across
void vtkCTHDataToPolyDataFilter::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataObject *input = this->GetInput();

  if (input)
    {
    this->vtkPolyDataSource::ComputeInputUpdateExtents(output);
    input->RequestExactExtentOn();
    }
}

//----------------------------------------------------------------------------
void vtkCTHDataToPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
