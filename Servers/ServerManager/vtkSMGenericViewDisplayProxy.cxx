/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMGenericViewDisplayProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMGenericViewDisplayProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkClientServerStream.h"
#include "vtkSMSourceProxy.h"
#include "vtkDataObject.h"
#include "vtkMPIMoveData.h"
#include "vtkDataSet.h"
#include "vtkPVOptions.h"

vtkStandardNewMacro(vtkSMGenericViewDisplayProxy);
vtkCxxRevisionMacro(vtkSMGenericViewDisplayProxy, "$Revision: 1.10 $");

//-----------------------------------------------------------------------------
vtkSMGenericViewDisplayProxy::vtkSMGenericViewDisplayProxy()
{
  this->UpdateSuppressorProxy = 0;
  this->CollectProxy = 0;
  this->ReduceProxy = 0;

  // When created, collection is off.
  // I set these to -1 to ensure the decision is propagated.
  this->CollectionDecision = -1;
  this->CanCreateProxy = 0;
  this->Visibility = 1;

  this->Output = 0;
  this->UpdateRequiredFlag = 1;

  this->ReductionType = 0;
}

//-----------------------------------------------------------------------------
vtkSMGenericViewDisplayProxy::~vtkSMGenericViewDisplayProxy()
{
  if ( this->Output )
    {
    this->Output->Delete();
    this->Output = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  this->Superclass::MarkModified(modifiedProxy);
  if (modifiedProxy != this)
    {
    this->UpdateRequiredFlag= 1;
    }
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated || !this->CanCreateProxy)
    {
    return;
    }
  this->UpdateSuppressorProxy = this->GetSubProxy("UpdateSuppressor");
  this->UpdateSuppressorProxy->SetServers(
    this->Servers | vtkProcessModule::CLIENT);
  this->CollectProxy = this->GetSubProxy("Collect");
  this->CollectProxy->SetServers(
    this->Servers | vtkProcessModule::CLIENT);

  this->ReduceProxy =  this->GetSubProxy("Reduce");
  this->ReduceProxy->SetServers(this->Servers);

  this->PostProcessorProxy = this->GetSubProxy("PostProcessor");
  if (this->PostProcessorProxy)
    {
    this->PostProcessorProxy->SetServers(vtkProcessModule::CLIENT);
    }

  this->Superclass::CreateVTKObjects(numObjects);
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::SetReductionType(int type)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Cannot set reduction type before CreateVTKObjects().");
    return;
    }

  if (!this->ReduceProxy)
    {
    vtkErrorMacro("Could not locate the Reduction proxy.");
    return;
    }

  this->ReductionType = type;

  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  const char* classname = 0;
  switch (type)
    {
  case ADD:
    classname = "vtkAttributeDataReductionFilter";
    break;

  case POLYDATA_APPEND:
    classname = "vtkAppendPolyData";
    break;

  case UNSTRUCTURED_APPEND:
    classname = "vtkAppendFilter";
    break;

  case FIRST_NODE_ONLY:
    classname = 0;
    break;

  default:
    vtkErrorMacro("Unknown reduction type: " << type);
    return;
    }

  vtkClientServerID rfid = { 0 };
  if ( classname )
    {
    rfid = pm->NewStreamObject(classname, stream);
    }
  for (unsigned int i=0; i < this->ReduceProxy->GetNumberOfIDs(); i++)
    {
    stream
      << vtkClientServerStream::Invoke
      << this->ReduceProxy->GetID(i) << "SetReductionHelper"
      << rfid
      << vtkClientServerStream::End;
    }

  if ( classname )
    {
    pm->DeleteStreamObject(rfid, stream);
    }

  pm->SendStream(this->GetConnectionID(),
    this->ReduceProxy->GetServers(), stream);
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::SetInput(vtkSMProxy* sinput)
{
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(sinput);
  int num = 0;
  if (input)
    {
    num = input->GetNumberOfParts();
    if (!num)
      {
      input->CreateParts();
      num = input->GetNumberOfParts();
      }
    }
  if (num == 0)
    {
    vtkErrorMacro("Input proxy has no output! Cannot create the display");
    return;
    }

  // This will create all the subproxies with correct number of parts.
  if (input)
    {
    this->CanCreateProxy = 1;
    }

  this->CreateVTKObjects(num);
  unsigned int i;
  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMInputProperty* ip = 0;

  ip = vtkSMInputProperty::SafeDownCast(
    this->ReduceProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(input);
  this->ReduceProxy->UpdateVTKObjects();

  for (i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
    stream
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetController"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << this->ReduceProxy->GetID(i) << "SetController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->ConnectionID, 
      this->ReduceProxy->GetServers(), stream);
    }

  ip = vtkSMInputProperty::SafeDownCast(
    this->CollectProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->ReduceProxy);
  this->CollectProxy->UpdateVTKObjects();


  for (i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
    {
    stream
      << vtkClientServerStream::Invoke
      << this->CollectProxy->GetID(i) << "SetProcessModuleConnection"
      << pm->GetConnectionClientServerID(this->GetConnectionID())
      << vtkClientServerStream::End;
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->ConnectionID, 
      this->CollectProxy->GetServers(), stream);
    }

  ip = vtkSMInputProperty::SafeDownCast(
    this->UpdateSuppressorProxy->GetProperty("Input"));
  ip->RemoveAllProxies();
  ip->AddProxy(this->CollectProxy);
  this->UpdateSuppressorProxy->UpdateVTKObjects();


  if ( vtkProcessModule::GetProcessModule()->IsRemote(this->GetConnectionID()))
    {
    for (i=0; i < this->CollectProxy->GetNumberOfIDs(); i++)
      {
      vtkClientServerStream cmd;
      cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogStartEvent" << "Execute Collect"
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "AddObserver" << "StartEvent" << cmd
        << vtkClientServerStream::End;
      cmd.Reset();
      cmd << vtkClientServerStream::Invoke
        << pm->GetProcessModuleID() << "LogEndEvent" << "Execute Collect"
        << vtkClientServerStream::End;
      stream
        << vtkClientServerStream::Invoke
        << this->CollectProxy->GetID(i) << "AddObserver" << "EndEvent" << cmd
        << vtkClientServerStream::End;
      pm->SendStream(this->ConnectionID, this->CollectProxy->GetServers(),
                     stream);
      }
    }

  if (this->PostProcessorProxy)
    {
    ip = vtkSMInputProperty::SafeDownCast(
      this->PostProcessorProxy->GetProperty("Input"));
    ip->RemoveAllProxies();
    ip->AddProxy(this->CollectProxy);
    this->PostProcessorProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::SetupCollectionFilter(
  vtkSMProxy* collectProxy)
{ 
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  int i, num;

  vtkClientServerStream stream;

  num = collectProxy->GetNumberOfIDs();
  for (i = 0; i < num; ++i)
    {
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMoveModeToCollect"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetServerToDataServer"
      << vtkClientServerStream::End;
    int mask = ~vtkProcessModule::CLIENT;
    pm->SendStream(this->ConnectionID,
                   collectProxy->GetServers() & mask,
                   stream);
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetMoveModeToCollect"
      << vtkClientServerStream::End;
    stream
      << vtkClientServerStream::Invoke
      << collectProxy->GetID(i) << "SetServerToClient"
      << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID,
                   vtkProcessModule::CLIENT,
                   stream);
    }
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::Update()
{
  this->UpdateSuppressorProxy->InvokeCommand("ForceUpdate");
  this->Superclass::Update();
  this->UpdateRequiredFlag = 0;

  if (this->PostProcessorProxy)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkAlgorithm* dp = vtkAlgorithm::SafeDownCast(
      pm->GetObjectFromID(this->PostProcessorProxy->GetID(0))); 
    if (!dp)
      {
      vtkErrorMacro("Failed to get algorithm for PostProcessorProxy.");
      }
    else
      {
      dp->Update();
      }
    }
}

//-----------------------------------------------------------------------------
int vtkSMGenericViewDisplayProxy::UpdateRequired()
{
  return this->UpdateRequiredFlag;
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::AddInput(vtkSMSourceProxy* input,
  const char* vtkNotUsed(method), int vtkNotUsed(hasMultipleInputs))
{
  this->SetInput(input);
}
//-----------------------------------------------------------------------------
vtkDataObject* vtkSMGenericViewDisplayProxy::GetOutput()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if (!pm || !this->CollectProxy)
    {
    return NULL;
    }

  vtkAlgorithm* dp;
  if (this->PostProcessorProxy)
    {
    dp = vtkAlgorithm::SafeDownCast(
      pm->GetObjectFromID(this->PostProcessorProxy->GetID(0))); 
    }
  else
    {
    dp = vtkAlgorithm::SafeDownCast(
      pm->GetObjectFromID(this->CollectProxy->GetID(0)));
    }

  if (dp == NULL)
    {
    return NULL;
    }

  return dp->GetOutputDataObject(0);
}

//-----------------------------------------------------------------------------
void vtkSMGenericViewDisplayProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Visibility: " << this->Visibility << endl;
}

