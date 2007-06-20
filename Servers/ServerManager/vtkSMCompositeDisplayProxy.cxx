/*=========================================================================

Program:   ParaView
Module:    $RCSfile: vtkSMCompositeDisplayProxy.cxx,v $

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompositeDisplayProxy.h"

#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVOptions.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPart.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMCompositeDisplayProxy);
vtkCxxRevisionMacro(vtkSMCompositeDisplayProxy, "$Revision: 1.34 $");
//-----------------------------------------------------------------------------
vtkSMCompositeDisplayProxy::vtkSMCompositeDisplayProxy()
{
  this->CollectProxy = 0;
  this->LODCollectProxy = 0;
  this->VolumeCollectProxy = 0;

  this->DistributorProxy = 0;
  this->LODDistributorProxy = 0;
  this->VolumeDistributorProxy = 0;

  this->DistributorSuppressorProxy = 0;
  this->LODDistributorSuppressorProxy = 0;
  this->VolumeDistributorSuppressorProxy = 0;

  // When created, collection is off.
  // I set these to -1 to ensure the decision is propagated.
  this->CollectionDecision = -1;
  this->LODCollectionDecision = -1;

  this->DistributedGeometryIsValid = 0;
  this->DistributedLODGeometryIsValid = 0;
  this->DistributedVolumeGeometryIsValid = 0;

  this->OrderedCompositing = -1;
  this->OrderedCompositingTree = NULL;
}

//-----------------------------------------------------------------------------
vtkSMCompositeDisplayProxy::~vtkSMCompositeDisplayProxy()
{
  this->SetOrderedCompositingTreeInternal(NULL);

  this->CollectProxy = 0;
  this->LODCollectProxy = 0;
  this->VolumeCollectProxy = 0;

  this->DistributorProxy = 0;
  this->LODDistributorProxy = 0;
  this->VolumeDistributorProxy = 0;
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated || !this->CanCreateProxy)
    {
    return;
    }

  this->CollectProxy = this->GetSubProxy("Collect");
  this->LODCollectProxy = this->GetSubProxy("LODCollect");

  if (!this->CollectProxy)
    {
    vtkErrorMacro("Failed to find SubProxy Collect.");
    return;
    }

  if (!this->LODCollectProxy)
    {
    vtkErrorMacro("Failed to find SubProxy LODCollect.");
    return;
    }
  this->CollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->LODCollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);

  this->DistributorProxy = this->GetSubProxy("Distributor");
  this->LODDistributorProxy = this->GetSubProxy("LODDistributor");

  if (!this->DistributorProxy)
    {
    vtkErrorMacro("Failed to find SubProxy Distributor.");
    return;
    }
  if (!this->LODDistributorProxy)
    {
    vtkErrorMacro("Failed to find SubProxy LODDistributor.");
    return;
    }

  this->DistributorProxy->SetServers(vtkProcessModule::RENDER_SERVER);
  this->LODDistributorProxy->SetServers(vtkProcessModule::RENDER_SERVER);

  this->DistributorSuppressorProxy = 
    this->GetSubProxy("DistributorSuppressor");
  this->LODDistributorSuppressorProxy = 
    this->GetSubProxy("LODDistributorSuppressor");

  if (!this->DistributorSuppressorProxy)
    {
    vtkErrorMacro("Failed to find SubProxy DistributorSuppressor.");
    return;
    }
  if (!this->LODDistributorSuppressorProxy)
    {
    vtkErrorMacro("Failed to find SubProxy LODDistributorSuppressor.");
    return;
    }

  this->DistributorSuppressorProxy->SetServers(
    vtkProcessModule::CLIENT_AND_SERVERS);
  this->LODDistributorSuppressorProxy->SetServers(
    vtkProcessModule::CLIENT_AND_SERVERS);
  
  if (this->VolumePipelineType != NONE)
    {
    this->VolumeCollectProxy = this->GetSubProxy("VolumeCollect");
    if (!this->VolumeCollectProxy)
      {
      vtkErrorMacro("Failed to find SubProxy VolumeCollect.");
      return;
      }
    this->VolumeCollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
    }
  else
    {
    this->RemoveSubProxy("VolumeCollect");
    }

  this->VolumeDistributorProxy = this->GetSubProxy("VolumeDistributor");
  if (!this->VolumeDistributorProxy)
    {
    vtkErrorMacro("Failed to find SubProxy VolumeDistributor.");
    return;
    }
  this->VolumeDistributorProxy->SetServers(vtkProcessModule::RENDER_SERVER);

  this->VolumeDistributorSuppressorProxy = 
    this->GetSubProxy("VolumeDistributorSuppressor");
  if (!this->VolumeDistributorSuppressorProxy)
    {
    vtkErrorMacro("Failed to find SubProxy VolumeDistributorSuppressor.");
    return;
    }
  this->VolumeDistributorSuppressorProxy->SetServers(
    vtkProcessModule::CLIENT_AND_SERVERS);

  this->Superclass::CreateVTKObjects();

  // We activate the update suppressor after the distributor and make
  // the one before the distributor behave like a simply pass through 
  // filter.
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("Enabled"));
  ivp->SetElement(0, 0);
  this->UpdateSuppressorProxy->UpdateProperty("Enabled");
  this->CacherProxy = this->DistributorSuppressorProxy;
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupDefaults()
{
  this->Superclass::SetupDefaults();
  this->SetupCollectionFilter(this->CollectProxy);
  this->SetupCollectionFilter(this->LODCollectProxy);

  vtkClientServerStream cmd;
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent" << "Execute Collect"
      << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << this->CollectProxy->GetID() << "AddObserver" << "StartEvent" << cmd
    << vtkClientServerStream::End;
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent" << "Execute Collect"
      << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << this->CollectProxy->GetID() << "AddObserver" << "EndEvent" << cmd
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::CLIENT_AND_SERVERS, stream);
  
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent" << "Execute LODCollect"
      << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << this->LODCollectProxy->GetID() << "AddObserver" << "StartEvent" << cmd
    << vtkClientServerStream::End;
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent" << "Execute LODCollect"
      << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << this->LODCollectProxy->GetID() << "AddObserver" << "EndEvent" << cmd
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::CLIENT_AND_SERVERS, stream);
  
  // Handle collection setup with client server.
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetSocketController"
    << pm->GetConnectionClientServerID(this->ConnectionID)
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->CollectProxy->GetID() << "SetSocketController"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetSocketController"
    << pm->GetConnectionClientServerID(this->ConnectionID)
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->LODCollectProxy->GetID() << "SetSocketController"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::CLIENT_AND_SERVERS, stream);
  
  // Special condition to signal the client.
  // Because both processes of the Socket controller think they are 0!!!!
  if (pm->GetClientMode())
    {
    stream
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID() << "SetController" << 0
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->LODCollectProxy->GetID() << "SetController" << 0
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
                   vtkProcessModule::CLIENT, stream);
    }

  this->SetOrderedCompositing(0);

  cmd.Reset();
  stream.Reset();
  
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent"
      << "Execute OrderedCompositeDistribute"
      << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->DistributorProxy->GetID() << "AddObserver"
         << "StartEvent" << cmd
         << vtkClientServerStream::End;
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent"
      << "Execute OrderedCompositeDistribute"
      << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->DistributorProxy->GetID() << "AddObserver"
         << "EndEvent" << cmd
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::RENDER_SERVER, stream);
  
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent"
      << "Execute LODOrderedCompositeDistribute"
      << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->LODDistributorProxy->GetID() << "AddObserver"
         << "StartEvent" << cmd
         << vtkClientServerStream::End;
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent"
      << "Execute LODOrderedCompositeDistribute"
      << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->LODDistributorProxy->GetID() << "AddObserver"
         << "EndEvent" << cmd
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
                 vtkProcessModule::RENDER_SERVER, stream);
  
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetController"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->DistributorProxy->GetID() << "SetController"
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetController"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->LODDistributorProxy->GetID() << "SetController"
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, 
                 stream);
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupPipeline()
{
  this->Superclass::SetupPipeline();
  vtkSMStringVectorProperty *svp = 0;
  vtkClientServerStream stream;

  this->Connect(this->LODCollectProxy, this->LODDecimatorProxy, 0);
  this->Connect(this->CollectProxy, this->GeometryFilterProxy, 0);

  this->LODCollectProxy->UpdateVTKObjects();
  this->CollectProxy->UpdateVTKObjects();
  
  if (this->CollectProxy)
    {
    stream
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID() << "GetPolyDataOutput"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->UpdateSuppressorProxy->GetID() << "SetInput"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  
  if (this->LODCollectProxy)
    {
    stream
      << vtkClientServerStream::Invoke
      << this->LODCollectProxy->GetID() << "GetPolyDataOutput"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << this->LODUpdateSuppressorProxy->GetID() << "SetInput"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID, vtkProcessModule::CLIENT_AND_SERVERS, stream);

  // On the render server, insert a distributor.
  this->Connect(this->DistributorProxy, this->UpdateSuppressorProxy, 0);
  this->Connect(this->LODDistributorProxy, this->LODUpdateSuppressorProxy, 0);

  // On the render server, attach an update suppressor to the distributor.  On
  // the client side (since the distributor is not there) attach it to the other
  // update suppressor.  We cannot do this through the server manager interface.
  stream << vtkClientServerStream::Invoke
         << this->UpdateSuppressorProxy->GetID() << "GetOutputPort" << 0
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->DistributorSuppressorProxy->GetID()
         << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  
  stream << vtkClientServerStream::Invoke
         << this->LODUpdateSuppressorProxy->GetID() << "GetOutputPort" << 0
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->LODDistributorSuppressorProxy->GetID()
         << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    vtkProcessModule::CLIENT | vtkProcessModule::DATA_SERVER, stream);

  stream << vtkClientServerStream::Invoke
         << this->DistributorProxy->GetID() << "GetOutputPort" << 0
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->DistributorSuppressorProxy->GetID()
         << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  
  stream << vtkClientServerStream::Invoke
         << this->LODDistributorProxy->GetID() << "GetOutputPort" << 0
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->LODDistributorSuppressorProxy->GetID()
         << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    vtkProcessModule::RENDER_SERVER, stream);

  this->Connect(this->MapperProxy, this->DistributorSuppressorProxy, 0);
  this->Connect(this->LODMapperProxy, this->LODDistributorSuppressorProxy, 0);

  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->DistributorProxy->GetProperty("OutputType"));
  svp->SetElement(0, "vtkPolyData");

  svp = vtkSMStringVectorProperty::SafeDownCast(
    this->LODDistributorProxy->GetProperty("OutputType"));
  svp->SetElement(0, "vtkPolyData");

  this->DistributorProxy->UpdateVTKObjects();
  this->LODDistributorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupVolumePipeline()
{
  if (this->VolumePipelineType == NONE)
    {
    return;
    }

  this->Superclass::SetupVolumePipeline();

  vtkSMInputProperty *ip;
  vtkSMStringVectorProperty *svp;
  vtkClientServerStream stream;
  if (this->VolumePipelineType == IMAGE_DATA)
    {
    vtkSMInputProperty* usInput = vtkSMInputProperty::SafeDownCast(
      this->VolumeUpdateSuppressorProxy->GetProperty("Input"));
    
    ip = vtkSMInputProperty::SafeDownCast(
      this->VolumeCollectProxy->GetProperty("Input"));
    ip->RemoveAllProxies();
    ip->AddProxy(usInput->GetProxy(0));
    this->VolumeCollectProxy->UpdateVTKObjects();

    stream
      << vtkClientServerStream::Invoke
      << this->VolumeCollectProxy->GetID() 
      << "SetOutputDataType"
      << VTK_IMAGE_DATA
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->VolumeCollectProxy->GetID() << "GetOutputPort"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->VolumeUpdateSuppressorProxy->GetID() 
      << "SetInputConnection"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, 
      this->VolumeUpdateSuppressorProxy->GetServers(), 
      stream);
    }
  else if (this->VolumePipelineType == UNSTRUCTURED_GRID)
    {
    this->Connect(this->VolumeCollectProxy, this->VolumeFilterProxy, 0);
    
    stream
      << vtkClientServerStream::Invoke
      << this->VolumeCollectProxy->GetID() 
      << "SetOutputDataType"
      << VTK_UNSTRUCTURED_GRID
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->VolumeCollectProxy->GetID() << "GetOutputPort"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->VolumeUpdateSuppressorProxy->GetID() << "SetInputConnection"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, vtkProcessModule::CLIENT_AND_SERVERS, stream);
    
    // On the render server, insert a distributor.
    this->Connect(this->VolumeDistributorProxy, this->VolumeUpdateSuppressorProxy, 0);
    
    // On the render server, attach an update suppressor to the
    // distributor.  On the client side (since the distributor is not
    // there) attach it to the other update suppressor.  We cannot do this
    // through the server manager interface.
    stream 
      << vtkClientServerStream::Invoke
      << this->VolumeUpdateSuppressorProxy->GetID()<< "GetOutputPort" << 0
      << vtkClientServerStream::End;
    stream 
      << vtkClientServerStream::Invoke
      << this->VolumeDistributorSuppressorProxy->GetID()
      << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID,
      vtkProcessModule::CLIENT | vtkProcessModule::DATA_SERVER, stream);

    stream << vtkClientServerStream::Invoke
           << this->VolumeDistributorProxy->GetID() << "GetOutputPort" << 0
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << this->VolumeDistributorSuppressorProxy->GetID()
           << "SetInputConnection" << 0 << vtkClientServerStream::LastResult
           << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID,
      vtkProcessModule::RENDER_SERVER, stream);

    this->Connect(this->VolumePTMapperProxy, 
                  this->VolumeDistributorSuppressorProxy, 0);
    this->Connect(this->VolumeHAVSMapperProxy, 
                  this->VolumeDistributorSuppressorProxy, 0);
    this->Connect(this->VolumeBunykMapperProxy, 
                  this->VolumeDistributorSuppressorProxy, 0);
    this->Connect(this->VolumeZSweepMapperProxy, 
                  this->VolumeDistributorSuppressorProxy, 0);
    
    svp = vtkSMStringVectorProperty::SafeDownCast(
      this->VolumeDistributorProxy->GetProperty("OutputType"));
    svp->SetElement(0, "vtkUnstructuredGrid");
    
    this->VolumeDistributorProxy->UpdateVTKObjects();
    }

  // The VolumeMapper for Image Data is directly connected to the 
  // VolumeUpdateSuppressorProxy, and not to VolumeDistributorSuppressorProxy
  // as is the case with UNSTRUCTURED_GRID volume rendering.
  if (this->VolumePipelineType != IMAGE_DATA && 
      this->VolumeDistributorSuppressorProxy)
    {
    this->VolumeCacherProxy = this->VolumeDistributorSuppressorProxy;

    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->VolumeUpdateSuppressorProxy->GetProperty("Enabled"));
    ivp->SetElement(0, 0);
    this->VolumeUpdateSuppressorProxy->UpdateProperty("Enabled");
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupVolumeDefaults()
{
  if (this->VolumePipelineType == NONE)
    {
    return;
    }
  this->Superclass::SetupVolumeDefaults();

  if (this->VolumePipelineType == IMAGE_DATA)
    {
    return;
    }

  this->SetupCollectionFilter(this->VolumeCollectProxy);

  vtkClientServerStream cmd;
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent"
      << "Execute VolumeCollect"
      << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << this->VolumeCollectProxy->GetID() << "AddObserver" << "StartEvent"
    << cmd << vtkClientServerStream::End;
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent"
      << "Execute VolumeCollect"
      << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << this->VolumeCollectProxy->GetID() << "AddObserver" << "EndEvent"
    << cmd << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::CLIENT_AND_SERVERS, stream);
  
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetSocketController"
    << pm->GetConnectionClientServerID(this->ConnectionID)
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << this->VolumeCollectProxy->GetID() << "SetSocketController"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER_ROOT|
                 vtkProcessModule::RENDER_SERVER_ROOT, stream);
  
  
  // Special condition to signal the client.
  // Because both processes of the Socket controller think they are 0!!!!
  if (pm->GetClientMode())
    {
    stream
      << vtkClientServerStream::Invoke
      << this->VolumeCollectProxy->GetID() << "SetController" << 0
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
                   vtkProcessModule::CLIENT, stream);
    }

  cmd.Reset();
  stream.Reset();
  
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogStartEvent"
      << "Execute LODOrderedCompositeDistribute"
      << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->VolumeDistributorProxy->GetID() << "AddObserver"
         << "StartEvent" << cmd
         << vtkClientServerStream::End;
  cmd.Reset();
  cmd << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "LogEndEvent"
      << "Execute LODOrderedCompositeDistribute"
      << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->VolumeDistributorProxy->GetID() << "AddObserver"
         << "EndEvent" << cmd
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::RENDER_SERVER, stream);
  
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetController"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->VolumeDistributorProxy->GetID() << "SetController"
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::RENDER_SERVER, stream);
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetupCollectionFilter(vtkSMProxy* collectProxy)
{ 
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;

  // Default is pass through because it executes fastest.  
  stream
    << vtkClientServerStream::Invoke
    << collectProxy->GetID() << "SetMoveModeToPassThrough"
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::CLIENT_AND_SERVERS, stream);
  stream
    << vtkClientServerStream::Invoke
    << collectProxy->GetID() << "SetMPIMToNSocketConnection" 
    << pm->GetMPIMToNSocketConnectionID(this->ConnectionID)
    << vtkClientServerStream::End;
  // create, SetPassThrough, and set the mToN connection
  // object on all servers and client
  pm->SendStream(this->ConnectionID,
      vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);

  // always set client mode
  stream
    << vtkClientServerStream::Invoke
    << collectProxy->GetID() << "SetServerToClient"
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 vtkProcessModule::CLIENT, stream);
  // if running in client mode
  // then set the server to be servermode
  if(pm->GetClientMode())
    {
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID() << "SetServerToDataServer"
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
                   vtkProcessModule::DATA_SERVER, stream);
    }
  // if running in render server mode
  if(pm->GetRenderClientMode(this->GetConnectionID()))
    {
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID() << "SetServerToRenderServer"
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
                   vtkProcessModule::RENDER_SERVER, stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetInputInternal(vtkSMSourceProxy *input,
                                                  unsigned int outputPort)
{
  this->Superclass::SetInputInternal(input, outputPort);

  if (this->VolumePipelineType == UNSTRUCTURED_GRID)
    {
    // Find out how many processes are in the render server.  This should really
    // be done by the process module itself.
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() 
           << "GetNumberOfLocalPartitions"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, vtkProcessModule::RENDER_SERVER, stream);
    int numRenderServerProcs;
    if (!pm->GetLastResult(this->ConnectionID,
                           vtkProcessModule::RENDER_SERVER)
        .GetArgument(0, 0, &numRenderServerProcs))
      {
      vtkErrorMacro("Could not get the size of the render server.");
      }

    vtkIdType numCells = 
      input->GetDataInformation(outputPort)->GetNumberOfCells();
    if (numCells/numRenderServerProcs < 1000000)
      {
      this->SupportsZSweepMapper = 1;
      }
    if (numCells/numRenderServerProcs < 500000)
      {
      this->SupportsBunykMapper = 1;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::InvalidateDistributedGeometry()
{
  this->DistributedGeometryIsValid = 0;
  this->DistributedLODGeometryIsValid = 0;
  this->DistributedVolumeGeometryIsValid = 0;
}

//-----------------------------------------------------------------------------
int vtkSMCompositeDisplayProxy::IsDistributedGeometryValid()
{
  if (this->VolumeRenderMode)
    {
    return (this->DistributedVolumeGeometryIsValid && 
            this->VolumeGeometryIsValid );
    }
  else
    {
    return (this->DistributedGeometryIsValid && this->GeometryIsValid);
    }
}

//-----------------------------------------------------------------------------
vtkPVLODPartDisplayInformation* vtkSMCompositeDisplayProxy::GetLODInformation()
{
  if (!this->ObjectsCreated)
    {
    return 0;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!this->LODGeometryIsValid &&
      (pm->GetOptions()->GetClientMode() || 
       pm->GetNumberOfPartitions(this->ConnectionID) >= 2))
    { 
    // Update but with collection filter off.
    this->CollectionDecision = 0;
    this->LODCollectionDecision = 0;

    // Changing the collection decision can change the ordered compositing
    // distribution.
    int oc = this->OrderedCompositing;
    this->OrderedCompositing = 0;
    this->SetOrderedCompositing(oc);

    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->CollectProxy->GetProperty("MoveMode"));
    if (!ivp)
      {
      vtkErrorMacro("Failed to find property MoveMode on CollectProxy.");
      return 0;
      }
    ivp->SetElement(0, 0); // Pass Through.
    vtkSMProperty *p = this->UpdateSuppressorProxy->GetProperty("ForceUpdate");
    if (!p)
      {
      vtkErrorMacro("Failed to find property ForceUpdate on "
                    "UpdateSuppressorProxy.");
      return 0;
      }
    p->Modified();
    this->UpdateVTKObjects();
    }
  return this->Superclass::GetLODInformation();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetCollectionDecision(int v)
{
  if (v == this->CollectionDecision || !this->CollectProxy)
    {
    return;
    }
  this->CollectionDecision = v;
  // TODO: old codes only supports mode PassThru and Collect. Why?
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("MoveMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property MoveMode on CollectProxy.");
    return;
    }
  ivp->SetElement(0, this->CollectionDecision);
  this->InvalidateGeometryInternal(0);
  this->UpdateVTKObjects();

  // Changing the collection decision can change the ordered compositing
  // distribution.
  int oc = this->OrderedCompositing;
  this->OrderedCompositing = 0;
  this->SetOrderedCompositing(oc);
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetLODCollectionDecision(int v)
{
  if (!this->ObjectsCreated || v == this->LODCollectionDecision)
    {
    return;
    }
  this->LODCollectionDecision = v;
  // TODO: old codes only supports mode PassThru and Clone. Why?
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->LODCollectProxy->GetProperty("MoveMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property MoveMode on LODCollectProxy.");
    return;
    }
  if (!this->LODCollectionDecision)
    {
    ivp->SetElement(0, this->LODCollectionDecision);
    }
  else
    {
    ivp->SetElement(0, 2);
    }
  //ivp->SetElement(0, this->LODCollectionDecision);
  this->InvalidateLODGeometry(0);
  this->UpdateVTKObjects();

  // Changing the collection decision can change the ordered compositing
  // distribution.
  int oc = this->OrderedCompositing;
  this->OrderedCompositing = 0;
  this->SetOrderedCompositing(oc);
}

//-----------------------------------------------------------------------------

void vtkSMCompositeDisplayProxy::SetOrderedCompositing(int val)
{
  if (!this->ObjectsCreated || (this->OrderedCompositing == val))
    {
    return;
    }

  this->OrderedCompositing = val;

  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->DistributorProxy->GetProperty("PassThrough"));
  ivp->SetElements1(!this->OrderedCompositing || this->CollectionDecision);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->LODDistributorProxy->GetProperty("PassThrough"));
  ivp->SetElements1(!this->OrderedCompositing || this->LODCollectionDecision);

  if (this->VolumeDistributorProxy)
    {
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->VolumeDistributorProxy->GetProperty("PassThrough"));
    ivp->SetElements1(!this->OrderedCompositing);
    }

  this->UpdateVTKObjects();

  this->InvalidateDistributedGeometry();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetOrderedCompositingTreeInternal(
  vtkSMProxy* tree)
{
  vtkSetObjectBodyMacro(OrderedCompositingTree, vtkSMProxy, tree);
}

//-----------------------------------------------------------------------------

void vtkSMCompositeDisplayProxy::SetOrderedCompositingTree(vtkSMProxy *tree)
{
  if (this->OrderedCompositingTree == tree)
    {
    return;
    }

  if (this->OrderedCompositingTree)
    {
    this->RemoveGeometryFromCompositingTree();
    }

  this->SetOrderedCompositingTreeInternal(tree);

  if (this->OrderedCompositingTree)
    {
    this->AddGeometryToCompositingTree();
    }

  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->DistributorProxy->GetProperty("PKdTree"));
  pp->RemoveAllProxies();
  pp->AddProxy(this->OrderedCompositingTree);

  this->DistributorProxy->UpdateVTKObjects();
  this->LODDistributorProxy->UpdateVTKObjects();
  if (this->VolumeDistributorProxy)
    {
    this->VolumeDistributorProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------

void vtkSMCompositeDisplayProxy::RemoveGeometryFromCompositingTree()
{
  unsigned int i;

  vtkSMInputProperty *ip = vtkSMInputProperty::SafeDownCast(
    this->DistributorProxy->GetProperty("Input"));
  if (ip->GetNumberOfProxies() < 1) 
    {
    return;
    }

  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->OrderedCompositingTree->GetProperty("DataSets"));

  vtkSMSourceProxy *input = vtkSMSourceProxy::SafeDownCast(ip->GetProxy(0));
  for (i = 0; i < input->GetNumberOfParts(); i++)
    {
    pp->RemoveProxy(input->GetPart(i)->GetDataObjectProxy(0));
    }

  if (this->VolumeDistributorProxy)
    {
    ip = vtkSMInputProperty::SafeDownCast(
      this->VolumeDistributorProxy->GetProperty("Input"));
    if (ip->GetNumberOfProxies()> 0)
      {
      input = vtkSMSourceProxy::SafeDownCast(ip->GetProxy(0));
      for (i = 0; i < input->GetNumberOfParts(); i++)
        {
        pp->RemoveProxy(input->GetPart(i)->GetDataObjectProxy(0));
        }
      }
    }

  this->OrderedCompositingTree->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------

void vtkSMCompositeDisplayProxy::AddGeometryToCompositingTree()
{
  this->RemoveGeometryFromCompositingTree();

  if (this->Visibility)
    {
    vtkSMInputProperty *ip = 0;
    if (!this->VolumeRenderMode)
      {
      ip = vtkSMInputProperty::SafeDownCast(
        this->DistributorProxy->GetProperty("Input"));
      }
    else if (this->VolumePipelineType == UNSTRUCTURED_GRID)
      {
      ip = vtkSMInputProperty::SafeDownCast(
        this->VolumeDistributorProxy->GetProperty("Input"));
      }
    else if (this->VolumePipelineType == IMAGE_DATA)
      {
      ip = vtkSMInputProperty::SafeDownCast(
        this->VolumeUpdateSuppressorProxy->GetProperty("Input"));
      }
    if (!ip || ip->GetNumberOfProxies() < 1) 
      {
      return;
      }
    vtkSMSourceProxy *input = vtkSMSourceProxy::SafeDownCast(ip->GetProxy(0));

    vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
      this->OrderedCompositingTree->GetProperty("DataSets"));

    for (unsigned int i = 0; i < input->GetNumberOfParts(); i++)
      {
      pp->AddProxy(input->GetPart(i)->GetDataObjectProxy(0));
      }

    this->OrderedCompositingTree->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::BuildKdTreeUsingDataPartitions(
  vtkSMProxy* kdGenerator)
{
   vtkClientServerStream stream;
   stream << vtkClientServerStream::Invoke
     << this->VolumeUpdateSuppressorProxy->GetID()
     << "GetInput"
     << vtkClientServerStream::End;
   stream << vtkClientServerStream::Invoke
     << kdGenerator->GetID()
     << "BuildTree"
     << vtkClientServerStream::LastResult
     << vtkClientServerStream::End;
   vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
   pm->SendStream(kdGenerator->GetConnectionID(),
     kdGenerator->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::SetVisibility(int visible)
{
  this->Superclass::SetVisibility(visible);

  if (this->OrderedCompositingTree)
    {
    this->AddGeometryToCompositingTree();
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::CacheUpdate(int idx, int total)
{
  // Since cache is kept by the DistributorSuppressor,
  // the UpdateSuppressor connected to the Collect filter doesn't
  // get any chance to  mark the collect filter modified on the client
  // side, hence we explicitly mark it modified.
  vtkClientServerStream stream;
  if (this->VolumeCollectProxy && this->VolumeRenderMode)
    {
    stream << vtkClientServerStream::Invoke
           << this->VolumeCollectProxy->GetID()
           << "Modified"
           << vtkClientServerStream::End;
    }
  else if (this->CollectProxy)
    {
    stream << vtkClientServerStream::Invoke
           << this->CollectProxy->GetID()
           << "Modified"
           << vtkClientServerStream::End;    
    }
  vtkProcessModule::GetProcessModule()->SendStream(
    this->GetConnectionID(), vtkProcessModule::CLIENT_AND_SERVERS,
    stream);
  this->Superclass::CacheUpdate(idx, total);

  this->DistributedLODGeometryIsValid = 0;
}

//-----------------------------------------------------------------------------
int vtkSMCompositeDisplayProxy::UpdateRequired()
{
  if (this->VolumeRenderMode)
    {
    if (!this->DistributedVolumeGeometryIsValid && this->VolumeGeometryIsValid)
      {
      return 1;
      }
    }
  else
    {
    if (!this->DistributedGeometryIsValid && this->GeometryIsValid)
      {
      return 1;
      }
    }

  if (!this->DistributedLODGeometryIsValid && this->LODGeometryIsValid)
    {
    return 1;
    }
  return this->Superclass::UpdateRequired();
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::UpdateDistributedGeometry(
  vtkSMAbstractViewModuleProxy* view)
{
  this->Update(view);

  if (this->VolumeRenderMode)
    {
    // The unstructured data is distributed only when rendering unstructured
    // grids. Therefore, we don't have to update anything
    // However, we must update the flag, since otherwise the render module
    // keeps on thinking that this distributed geometry is not valid
    // resulting in unnecessary updated on every still render.
    if (this->VolumePipelineType == IMAGE_DATA)
      {
      this->DistributedVolumeGeometryIsValid = 1;
      }
    else if (this->VolumePipelineType == UNSTRUCTURED_GRID)
      {
      if (!this->DistributedVolumeGeometryIsValid && this->VolumeGeometryIsValid)
        {
        vtkSMProperty *p
          = this->VolumeDistributorSuppressorProxy->GetProperty("ForceUpdate");
        p->Modified();
        this->DistributedVolumeGeometryIsValid = 1;
        // Make sure any ForceUpates are called in the correct order.  That is,
        // the superclasses' suppressors should be called before ours.
        this->VolumeUpdateSuppressorProxy->UpdateVTKObjects();
        this->VolumeDistributorSuppressorProxy->UpdateVTKObjects();
        }
      }
    }
  else
    {
    if (!this->DistributedGeometryIsValid && this->GeometryIsValid)
      {
      vtkSMProperty *p
        = this->DistributorSuppressorProxy->GetProperty("ForceUpdate");
      p->Modified();
      this->DistributedGeometryIsValid = 1;
      // Make sure any ForceUpates are called in the correct order.  That is,
      // the superclasses' suppressors should be called before ours.
      this->UpdateSuppressorProxy->UpdateVTKObjects();
      this->DistributorSuppressorProxy->UpdateVTKObjects();
      }
    }

  if (!this->DistributedLODGeometryIsValid && this->LODGeometryIsValid)
    {
    vtkSMProperty *p
      = this->LODDistributorSuppressorProxy->GetProperty("ForceUpdate");
    p->Modified();
    this->DistributedLODGeometryIsValid = 1;
    // Make sure any ForceUpates are called in the correct order.  That is,
    // the superclasses' suppressors should be called before ours.
    this->LODUpdateSuppressorProxy->UpdateVTKObjects();
    this->LODDistributorSuppressorProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::Update(vtkSMAbstractViewModuleProxy* view)
{
  // If any geometry is going to be updated, make sure we invalidate the
  // distributed geometry.
  this->DistributedGeometryIsValid
    = this->DistributedGeometryIsValid && this->GeometryIsValid;
  this->DistributedLODGeometryIsValid =
    this->DistributedLODGeometryIsValid && this->LODGeometryIsValid;
  this->DistributedVolumeGeometryIsValid
    = this->DistributedVolumeGeometryIsValid && this->VolumeGeometryIsValid;

  this->Superclass::Update(view);
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::MarkModified(vtkSMProxy *modifiedProxy)
{
  if (modifiedProxy == this->OrderedCompositingTree)
    {
    // If modified proxy is the PkdTree proxy, then we only invalidate
    // the geometry after the distributor, which uses the 
    // PkdTree, no need to invalidate geometry before the distributor.
    // That's why we don't call Superclass::MarkModified.
    this->InvalidateDistributedGeometry();
    this->vtkSMDisplayProxy::MarkModified(modifiedProxy);
    }
  else
    {
    this->Superclass::MarkModified(modifiedProxy);
    }
}

//-----------------------------------------------------------------------------
void vtkSMCompositeDisplayProxy::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CollectionDecision: " << this->CollectionDecision << endl;
  os << indent << "LODCollectionDecision: " << this->LODCollectionDecision 
     << endl;
  os << indent << "OrderedCompositing: " << this->OrderedCompositing << endl;
  os << indent << "OrderedCompositingTree: "
     << this->OrderedCompositingTree << endl;
}
