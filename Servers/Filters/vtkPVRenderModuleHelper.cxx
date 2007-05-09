/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVRenderModuleHelper.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderModuleHelper.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVRenderModuleHelper);
vtkCxxRevisionMacro(vtkPVRenderModuleHelper, "$Revision: 1.2 $");
//-----------------------------------------------------------------------------
vtkPVRenderModuleHelper::vtkPVRenderModuleHelper()
{
  this->LODFlag = 0;
  this->UseTriangleStrips = 0;
  this->UseImmediateMode = 0;
  this->CachingEnabled = false;
}

//-----------------------------------------------------------------------------
vtkPVRenderModuleHelper::~vtkPVRenderModuleHelper()
{
}

//-----------------------------------------------------------------------------
void vtkPVRenderModuleHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LODFlag: " << this->LODFlag << endl;
  os << indent << "UseTriangleStrips: " << this->UseTriangleStrips << endl;
  os << indent << "UseImmediateMode: " << this->UseImmediateMode << endl;
  os << indent << "CachingEnabled: " << this->CachingEnabled << endl;
}
