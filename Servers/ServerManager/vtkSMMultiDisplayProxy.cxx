/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMMultiDisplayProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMMultiDisplayProxy.h"
#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"
#include "vtkClientServerID.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMMultiDisplayProxy);
vtkCxxRevisionMacro(vtkSMMultiDisplayProxy, "$Revision: 1.5 $");
//-----------------------------------------------------------------------------
vtkSMMultiDisplayProxy::vtkSMMultiDisplayProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMMultiDisplayProxy::~vtkSMMultiDisplayProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMMultiDisplayProxy::Update()
{
  this->SetLODCollectionDecision(1);
  this->Superclass::Update();
  this->UpdateLODPipeline(); // Since for Multi Display Render modules, 
  // the client always renders using LOD. Hence we keep the LOD pipeline
  // in sync also.
}

//-----------------------------------------------------------------------------
void vtkSMMultiDisplayProxy::SetLODCollectionDecision(int)
{
  // Always collect LOD.
  this->Superclass::SetLODCollectionDecision(1);

}

//-----------------------------------------------------------------------------
void vtkSMMultiDisplayProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated || !this->CanCreateProxy)
    {
    return;
    }
  
  this->Superclass::CreateVTKObjects(numObjects);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int i;
  
  vtkClientServerStream stream;
  for (i = 0; i < numObjects; ++i)
    {
    // This little hack causes collect mode to be iditical to clone mode.
    // This allows the superclass to treat tiled display like normal compositing.
    stream << vtkClientServerStream::Invoke
           << this->CollectProxy->GetID(i) << "DefineCollectAsCloneOn"
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->LODCollectProxy->GetID(i) << "DefineCollectAsCloneOn"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
      this->CollectProxy->GetServers(), stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMMultiDisplayProxy::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
