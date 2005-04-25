/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSimpleRenderModuleProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSimpleRenderModuleProxy.h"
#include "vtkObjectFactory.h"
#include "vtkSMSimpleDisplayProxy.h"
#include "vtkSMProxyManager.h"

vtkStandardNewMacro(vtkSMSimpleRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMSimpleRenderModuleProxy, "$Revision: 1.3 $");
//-----------------------------------------------------------------------------
vtkSMSimpleRenderModuleProxy::vtkSMSimpleRenderModuleProxy()
{
  this->SetDisplayXMLName("SimpleDisplay");
}

//-----------------------------------------------------------------------------
vtkSMSimpleRenderModuleProxy::~vtkSMSimpleRenderModuleProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMSimpleRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
