/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVMultiDisplayPartDisplay.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMultiDisplayPartDisplay.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkPVOptions.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMultiDisplayPartDisplay);
vtkCxxRevisionMacro(vtkPVMultiDisplayPartDisplay, "$Revision: 1.2 $");


//----------------------------------------------------------------------------
vtkPVMultiDisplayPartDisplay::vtkPVMultiDisplayPartDisplay()
{
}

//----------------------------------------------------------------------------
vtkPVMultiDisplayPartDisplay::~vtkPVMultiDisplayPartDisplay()
{
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayPartDisplay::SetLODCollectionDecision(int)
{
  // Always colect LOD.
  this->Superclass::SetLODCollectionDecision(1);
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayPartDisplay::Update()
{
  // Update like normal, but make sure the LOD is collected.
  // I encountered a bug. First render was missing the LOD on the client.
  this->Superclass::SetLODCollectionDecision(1);
  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void 
vtkPVMultiDisplayPartDisplay::CreateParallelTclObjects(vtkPVProcessModule *pm)
{
  this->Superclass::CreateParallelTclObjects(pm);

  // This little hack causes collect mode to be iditical to clone mode.
  // This allows the superclass to treat tiled display like normal compositing.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->CollectID << "DefineCollectAsCloneOn"
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->LODCollectID << "DefineCollectAsCloneOn"
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT_AND_SERVERS);

  if(pm->GetOptions()->GetClientMode())
    {
    // We need this because the socket controller has no way of distinguishing
    // between processes.
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "SetServerToDataServer"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetServerToDataServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER);
    }
  else
    {
    vtkErrorMacro("Cannot run tile display without client-server mode.");
    }

  // if running in render server mode
  if(pm->GetOptions()->GetRenderServerMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CollectID << "SetServerToRenderServer"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->LODCollectID << "SetServerToRenderServer"
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }  
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayPartDisplay::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


  



