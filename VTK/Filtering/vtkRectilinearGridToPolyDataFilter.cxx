/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkRectilinearGridToPolyDataFilter.cxx,v $
  Language:  C++
  Date:      $Date: 2002-09-03 20:33:58 $
  Version:   $Revision: 1.12 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRectilinearGridToPolyDataFilter.h"

#include "vtkRectilinearGrid.h"

vtkCxxRevisionMacro(vtkRectilinearGridToPolyDataFilter, "$Revision: 1.12 $");

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkRectilinearGridToPolyDataFilter::SetInput(vtkRectilinearGrid *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkRectilinearGrid *vtkRectilinearGridToPolyDataFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkRectilinearGrid *)(this->Inputs[0]);
}

