/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVCompositeBuffer.cxx,v $
  Language:  C++
  Date:      $Date: 2003-05-20 20:07:28 $
  Version:   $Revision: 1.2 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVCompositeBuffer.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVCompositeBuffer, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkPVCompositeBuffer);

//-------------------------------------------------------------------------
vtkPVCompositeBuffer::vtkPVCompositeBuffer()
{
  this->PData = NULL;
  this->ZData = NULL;
  this->UncompressedLength = -1;
}

  
//-------------------------------------------------------------------------
vtkPVCompositeBuffer::~vtkPVCompositeBuffer()
{
  if (this->PData)
    {
    this->PData->Delete();
    this->PData = NULL;
    }
  if (this->ZData)
    {
    this->ZData->Delete();
    this->ZData = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeBuffer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
  
  os << indent << "PData: " << this->PData << endl;
  os << indent << "ZData: " << this->ZData << endl;
  os << indent << "UncompressedLength: " << this->UncompressedLength << endl;
}



