/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkExplicitCell.cxx,v $
  Language:  C++
  Date:      $Date: 2002-03-06 15:43:41 $
  Version:   $Revision: 1.1 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExplicitCell.h"
#include "vtkCellArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkExplicitCell, "$Revision: 1.1 $");

vtkExplicitCell::vtkExplicitCell()
{
  this->CellId = -1;
}

void vtkExplicitCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Cell Id: " << this->CellId << "\n";
}
