/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVDisplay.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDisplay.h"


//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPVDisplay, "$Revision: 1.1 $");

//----------------------------------------------------------------------------
vtkPVDisplay::vtkPVDisplay()
{
}

//----------------------------------------------------------------------------
vtkPVDisplay::~vtkPVDisplay()
{
}

//----------------------------------------------------------------------------
void vtkPVDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

