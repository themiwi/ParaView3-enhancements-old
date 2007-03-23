/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPVLookupTableProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPVLookupTableProxy.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMPVLookupTableProxy);
vtkCxxRevisionMacro(vtkSMPVLookupTableProxy, "$Revision: 1.1 $");
//-----------------------------------------------------------------------------
vtkSMPVLookupTableProxy::vtkSMPVLookupTableProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMPVLookupTableProxy::~vtkSMPVLookupTableProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMPVLookupTableProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();
  this->InvokeCommand("Build");
}

//-----------------------------------------------------------------------------
void vtkSMPVLookupTableProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
