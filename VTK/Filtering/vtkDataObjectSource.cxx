/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkDataObjectSource.cxx,v $
  Language:  C++
  Date:      $Date: 2002-12-17 02:07:36 $
  Version:   $Revision: 1.14 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataObjectSource.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkDataObjectSource, "$Revision: 1.14 $");

vtkDataObjectSource::vtkDataObjectSource()
{
  this->SetOutput(vtkDataObject::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}


//----------------------------------------------------------------------------
vtkDataObject *vtkDataObjectSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataObject *)(this->Outputs[0]);
}

//----------------------------------------------------------------------------
void vtkDataObjectSource::SetOutput(vtkDataObject *output)
{
  this->vtkSource::SetNthOutput(0, output);
}


//----------------------------------------------------------------------------
void vtkDataObjectSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
