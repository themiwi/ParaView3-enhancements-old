/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMLookupTableProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLookupTableProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSMCommunicationModule.h"

vtkStandardNewMacro(vtkSMLookupTableProxy);
vtkCxxRevisionMacro(vtkSMLookupTableProxy, "$Revision: 1.1 $");

//---------------------------------------------------------------------------
vtkSMLookupTableProxy::vtkSMLookupTableProxy()
{
  this->SetVTKClassName("vtkLookupTable");
}

//---------------------------------------------------------------------------
vtkSMLookupTableProxy::~vtkSMLookupTableProxy()
{
}

//---------------------------------------------------------------------------
void vtkSMLookupTableProxy::Build()
{
  vtkClientServerStream stream;

  int numObjects = this->GetNumberOfIDs();

 for (int i=0; i<numObjects; i++)
    {
    stream << vtkClientServerStream::Invoke << this->GetID(i)
           << "Build" << vtkClientServerStream::End;
    }

 vtkSMCommunicationModule* cm = this->GetCommunicationModule();
 cm->SendStreamToServers(&stream, 
                         this->GetNumberOfServerIDs(),
                         this->GetServerIDs());
 stream.Print(cout);
}

//---------------------------------------------------------------------------
void vtkSMLookupTableProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}







