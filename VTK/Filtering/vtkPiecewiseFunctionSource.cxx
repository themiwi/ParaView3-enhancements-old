/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPiecewiseFunctionSource.cxx,v $
  Language:  C++
  Date:      $Date: 2002-10-04 20:43:44 $
  Version:   $Revision: 1.3 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPiecewiseFunctionSource.h"

#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"

vtkCxxRevisionMacro(vtkPiecewiseFunctionSource, "$Revision: 1.3 $");

//----------------------------------------------------------------------------
vtkPiecewiseFunctionSource::vtkPiecewiseFunctionSource()
{
  this->vtkSource::SetNthOutput(0, vtkPiecewiseFunction::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
vtkPiecewiseFunction *vtkPiecewiseFunctionSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkPiecewiseFunction *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
vtkPiecewiseFunction *vtkPiecewiseFunctionSource::GetOutput(int idx)
{
  return (vtkPiecewiseFunction *) this->vtkSource::GetOutput(idx); 
};

//----------------------------------------------------------------------------
void vtkPiecewiseFunctionSource::SetOutput(vtkPiecewiseFunction *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
void vtkPiecewiseFunctionSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
