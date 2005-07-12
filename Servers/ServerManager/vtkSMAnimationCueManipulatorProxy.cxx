/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMAnimationCueManipulatorProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationCueManipulatorProxy.h"

#include "vtkObjectFactory.h"
#include "vtkClientServerID.h"

vtkCxxRevisionMacro(vtkSMAnimationCueManipulatorProxy, "$Revision: 1.3 $");

//----------------------------------------------------------------------------
vtkSMAnimationCueManipulatorProxy::vtkSMAnimationCueManipulatorProxy()
{
  this->ObjectsCreated = 1; //since this class create no serverside objects.
}

//----------------------------------------------------------------------------
vtkSMAnimationCueManipulatorProxy::~vtkSMAnimationCueManipulatorProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueManipulatorProxy::Copy(vtkSMProxy* src, 
  const char* exceptionClass, int proxyPropertyCopyFlag)
{
  this->Superclass::Copy(src, exceptionClass, proxyPropertyCopyFlag);
  this->MarkAllPropertiesAsModified();
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueManipulatorProxy::SaveInBatchScript(ofstream* file)
{
  vtkClientServerID id = this->SelfID;
  *file << endl;
  *file << "set pvTemp" << id
    << " [$proxyManager NewProxy " << this->GetXMLGroup()
    << " " << this->GetXMLName() << "]" << endl;
  *file << "  $proxyManager RegisterProxy "
    << this->GetXMLGroup() << " pvTemp" << id
    << " $pvTemp" << id << endl;
  *file << "  $pvTemp" << id << " UnRegister {}" << endl;
  *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
}

//----------------------------------------------------------------------------
void vtkSMAnimationCueManipulatorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
