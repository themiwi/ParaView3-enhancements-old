/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMFileListDomain.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMFileListDomain.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMFileListDomain);
vtkCxxRevisionMacro(vtkSMFileListDomain, "$Revision: 1.1 $");

//---------------------------------------------------------------------------
vtkSMFileListDomain::vtkSMFileListDomain()
{
}

//---------------------------------------------------------------------------
vtkSMFileListDomain::~vtkSMFileListDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMFileListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
