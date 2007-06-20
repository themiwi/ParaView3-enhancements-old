/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPointLabelDisplayProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPointLabelDisplayProxy.h"

#include "vtkClientServerStream.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVOptions.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkUnstructuredGrid.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMPointLabelDisplayProxy);
vtkCxxRevisionMacro(vtkSMPointLabelDisplayProxy, "Revision: 1.1$");

//-----------------------------------------------------------------------------
vtkSMPointLabelDisplayProxy::vtkSMPointLabelDisplayProxy()
{
  this->CollectProxy = 0;
  this->UpdateSuppressorProxy = 0;
  this->MapperProxy = 0;
  this->ActorProxy = 0;
  this->TextPropertyProxy = 0;
  this->GeometryIsValid = 0;
}

//-----------------------------------------------------------------------------
vtkSMPointLabelDisplayProxy::~vtkSMPointLabelDisplayProxy()
{
  this->CollectProxy = 0;
  this->UpdateSuppressorProxy = 0;
  this->MapperProxy = 0;
  this->ActorProxy = 0;
  this->TextPropertyProxy = 0;
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::AddInput(unsigned int,
                                           vtkSMSourceProxy* input,
                                           unsigned int outputPort,
                                           const char*)
{
  this->SetInput(input, outputPort);
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::SetInput(vtkSMSourceProxy* input,
                                           unsigned int outputPort)
{
  vtkPVDataInformation *di=input->GetDataInformation();
  if(!di->DataSetTypeIsA("vtkDataSet") || di->GetCompositeDataClassName())
    {
    return;
    }

  this->InvalidateGeometry();
  this->CreateVTKObjects();

  this->SetupPipeline(); // Have to this earlier
  this->SetupDefaults(); 
  
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    this->CollectProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on UpdateSuppressorProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddInputConnection(input, outputPort);
  this->CollectProxy->UpdateProperty("Input");
}


//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
 
  this->CollectProxy = this->GetSubProxy("Collect");
  this->UpdateSuppressorProxy = this->GetSubProxy("UpdateSuppressor");
  this->MapperProxy = this->GetSubProxy("Mapper");
  this->ActorProxy = this->GetSubProxy("Prop2D");
  this->TextPropertyProxy =  this->GetSubProxy("Property");

  if (!this->CollectProxy || !this->UpdateSuppressorProxy || !this->MapperProxy
    || !this->ActorProxy || !this->TextPropertyProxy)
    {
    vtkErrorMacro("Not all required subproxies were defined.");
    return;
    }
  this->CollectProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->UpdateSuppressorProxy->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
  this->MapperProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->ActorProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->TextPropertyProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);

  this->Superclass::CreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::SetupPipeline()
{
  vtkSMInputProperty* ip;
  vtkSMProxyProperty* pp;

  vtkClientServerStream stream;

  vtkSMIntVectorProperty *otype = vtkSMIntVectorProperty::SafeDownCast(this->CollectProxy->GetProperty("OutputDataType"));
  if (otype != NULL)
    {
    otype->SetElement(0,4); //vtkUnstructuredGrid
    }
  
  stream << vtkClientServerStream::Invoke
         << this->CollectProxy->GetID() << "GetOutputPort"
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->UpdateSuppressorProxy->GetID() << "SetInputConnection"
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    this->UpdateSuppressorProxy->GetServers(), stream);

  ip = vtkSMInputProperty::SafeDownCast(
    this->MapperProxy->GetProperty("Input"));
  if (!ip)
    {
    vtkErrorMacro("Failed to find property Input on MapperProxy.");
    return;
    }
  ip->RemoveAllProxies();
  ip->AddProxy(this->UpdateSuppressorProxy);

  pp = vtkSMProxyProperty::SafeDownCast(
    this->MapperProxy->GetProperty("LabelTextProperty"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property LabelTextProperty.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->TextPropertyProxy);
  this->MapperProxy->UpdateVTKObjects();

  pp = vtkSMProxyProperty::SafeDownCast(
    this->ActorProxy->GetProperty("Mapper"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Mapper on ActorProxy.");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(this->MapperProxy);

  this->ActorProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::SetupDefaults()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkSMIntVectorProperty* ivp;

  // A rather complex mess to set the correct server variable 
  // on all of the remote duplication filters.
  if(pm->GetClientMode())
    {
    // We need this because the socket controller has no way of distinguishing
    // between processes.
    stream << vtkClientServerStream::Invoke
           << this->CollectProxy->GetID() << "SetServerToClient"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, 
                   vtkProcessModule::CLIENT, stream);
    }
  // pm->ClientMode is only set when there is a server.
  if(pm->GetClientMode())
    {
    stream << vtkClientServerStream::Invoke
           << this->CollectProxy->GetID() << "SetServerToDataServer"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
                   vtkProcessModule::DATA_SERVER, stream);
    }
  // if running in render server mode
  if(pm->GetRenderClientMode(this->GetConnectionID()))
    {
    stream << vtkClientServerStream::Invoke
           << this->CollectProxy->GetID() << "SetServerToRenderServer"
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
                   vtkProcessModule::RENDER_SERVER, stream);
    }  

  // Handle collection setup with client server.
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetSocketController"
         << pm->GetConnectionClientServerID(this->ConnectionID)
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << this->CollectProxy->GetID() 
         << "SetClientDataServerSocketController"
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
             vtkProcessModule::CLIENT|vtkProcessModule::DATA_SERVER, stream);

  stream << vtkClientServerStream::Invoke
         << this->CollectProxy->GetID() << "SetMPIMToNSocketConnection" 
         << pm->GetMPIMToNSocketConnectionID(this->ConnectionID)
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
       vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER, stream);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->CollectProxy->GetProperty("MoveMode"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property MoveMode on CollectProxy.");
    return;
    }
  ivp->SetElement(0, 2); // Clone mode.
  this->CollectProxy->UpdateVTKObjects();

  // Tell the update suppressor to produce the correct partition.
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetNumberOfLocalPartitions"
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << this->UpdateSuppressorProxy->GetID() << "SetUpdateNumberOfPieces"
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << pm->GetProcessModuleID() << "GetPartitionId"
         << vtkClientServerStream::End
         << vtkClientServerStream::Invoke
         << this->UpdateSuppressorProxy->GetID() << "SetUpdatePiece"
         << vtkClientServerStream::LastResult
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID,
                 this->UpdateSuppressorProxy->GetServers(), stream);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->TextPropertyProxy->GetProperty("FontSize"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property FontSize on TextPropertyProxy.");
    return;
    }
  ivp->SetElement(0, 24);
  this->TextPropertyProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::Update(vtkSMAbstractViewModuleProxy*)
{
  if (this->GeometryIsValid || !this->UpdateSuppressorProxy)
    {
    return;
    }
  vtkSMProperty* p = this->UpdateSuppressorProxy->GetProperty("ForceUpdate");
  p->Modified();
  this->UpdateSuppressorProxy->UpdateVTKObjects(); 
  this->GeometryIsValid = 1;
  this->InvokeEvent(vtkSMAbstractDisplayProxy::ForceUpdateEvent);
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::InvalidateGeometryInternal(int useCache)
{
  if (!useCache)
    {
    this->GeometryIsValid = 0;
    if (this->UpdateSuppressorProxy)
      {
      vtkSMProperty *p = 
        this->UpdateSuppressorProxy->GetProperty("RemoveAllCaches");
      p->Modified();
      this->UpdateSuppressorProxy->UpdateVTKObjects();
      }
    }
}

//-----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkSMPointLabelDisplayProxy::GetCollectedData()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();

  vtkMPIMoveData* dp = vtkMPIMoveData::SafeDownCast(
    pm->GetObjectFromID(this->CollectProxy->GetID()));
  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetUnstructuredGridOutput();
}

//----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::SetFontSizeCM(int size) 
{
  if (this->TextPropertyProxy)
    {
    
    vtkSMIntVectorProperty* ivp;
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->TextPropertyProxy->GetProperty("FontSize"));
    if (!ivp)
      {
      vtkErrorMacro("Failed to find property FontSize on TextPropertyProxy.");
      return;
      }
    ivp->SetElement(0, size);
    this->TextPropertyProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
int vtkSMPointLabelDisplayProxy::GetFontSizeCM() 
{
  if (this->TextPropertyProxy)
    {    
    vtkSMIntVectorProperty* ivp;
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->TextPropertyProxy->GetProperty("FontSize"));
    if (!ivp)
      {
      vtkErrorMacro("Failed to find property FontSize on TextPropertyProxy.");
      return 0;
      }
    return ivp->GetElement(0);
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSMPointLabelDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GeometryIsValid: " << this->GeometryIsValid << endl;
}

