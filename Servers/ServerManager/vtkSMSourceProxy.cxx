/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSourceProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSourceProxy.h"

#include "vtkClientServerStream.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVNumberOfOutputsInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMPart.h"
#include "vtkSMProperty.h"
#include "vtkSmartPointer.h"
#include "vtkCollection.h"
#include "vtkPVXMLElement.h"
#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMSourceProxy);
vtkCxxRevisionMacro(vtkSMSourceProxy, "$Revision: 1.43 $");

struct vtkSMSourceProxyInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSMPart> > Parts;
};

//---------------------------------------------------------------------------
vtkSMSourceProxy::vtkSMSourceProxy()
{
  this->PInternals = new  vtkSMSourceProxyInternals;
  this->PartsCreated = 0;

  this->DataInformation = vtkPVDataInformation::New();
  this->DataInformationValid = 0;
  this->ExecutiveName = 0;
  this->SetExecutiveName("vtkCompositeDataPipeline");

  this->DoInsertExtractPieces = 1;
}

//---------------------------------------------------------------------------
vtkSMSourceProxy::~vtkSMSourceProxy()
{
  delete this->PInternals;

  this->DataInformation->Delete();
  this->SetExecutiveName(0);
}

//---------------------------------------------------------------------------
unsigned int vtkSMSourceProxy::GetNumberOfParts()
{
  return this->PInternals->Parts.size();
}

//---------------------------------------------------------------------------
vtkSMPart* vtkSMSourceProxy::GetPart(unsigned int idx)
{
  return this->PInternals->Parts[idx].GetPointer();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::UpdatePipelineInformation()
{
  int numIDs = this->GetNumberOfIDs();
  if (numIDs <= 0)
    {
    return;
    }
  
  vtkClientServerStream command;
  for(int i=0; i<numIDs; i++)
    {
    command << vtkClientServerStream::Invoke << this->GetID(i)
            << "UpdateInformation" << vtkClientServerStream::End;
    }
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->ConnectionID, this->Servers, command);

  // This simply iterates over subproxies and calls UpdatePropertyInformation();
  this->Superclass::UpdatePipelineInformation();

  this->MarkModified(this);  
}
//---------------------------------------------------------------------------
int vtkSMSourceProxy::ReadXMLAttributes(vtkSMProxyManager* pm, 
                                        vtkPVXMLElement* element)
{
  const char* executiveName = element->GetAttribute("executive");
  if (executiveName)
    {
    this->SetExecutiveName(executiveName);
    }
  return this->Superclass::ReadXMLAttributes(pm, element);
}

//---------------------------------------------------------------------------
// Call Update() on all sources
// TODO this should update information properties.
void vtkSMSourceProxy::UpdatePipeline()
{
  int i;
  int numIDs = this->GetNumberOfIDs();
  if (numIDs <= 0)
    {
    return;
    }

  if (strcmp(this->GetVTKClassName(), "vtkPVEnSightMasterServerReader") == 0)
    { 
    // Cannot set the update extent until we get the output.  Need to call
    // update before we can get the output.  Cannot not update whole extent
    // of every source.  Multiblock should fix this.
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream command;
    for(i=0; i<numIDs; i++)
      {
      command << vtkClientServerStream::Invoke 
              << this->GetID(i)
              << "Update" 
              << vtkClientServerStream::End;
      }
    pm->SendStream(this->ConnectionID, this->Servers, command);
    return;
    }
    
  this->CreateParts();
  int num = this->GetNumberOfParts();
  for (i=0; i < num; ++i)
    {
    this->GetPart(i)->UpdatePipeline();
    }

  this->InvalidateDataInformation();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects(numObjects);

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  int numIDs = this->GetNumberOfIDs();
  if (this->ExecutiveName)
    {
    vtkClientServerStream stream;
    for (int i=0; i<numIDs; i++)
      {
      vtkClientServerID execId = pm->NewStreamObject(
        this->ExecutiveName, stream);
      vtkClientServerID sourceID = this->GetID(i);
      stream << vtkClientServerStream::Invoke << sourceID
        << "SetExecutive" << execId <<  vtkClientServerStream::End;

      // Keep track of how long each filter takes to execute.
      ostrstream filterName_with_warning_C4701;
      filterName_with_warning_C4701 << "Execute " << this->VTKClassName
                                    << " id: " << sourceID.ID << ends;
      vtkClientServerStream start;
      start << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
            << "LogStartEvent" << filterName_with_warning_C4701.str()
            << vtkClientServerStream::End;
      vtkClientServerStream end;
      end << vtkClientServerStream::Invoke << pm->GetProcessModuleID() 
          << "LogEndEvent" << filterName_with_warning_C4701.str()
          << vtkClientServerStream::End;
      delete[] filterName_with_warning_C4701.str();
      
      stream << vtkClientServerStream::Invoke 
             << sourceID << "AddObserver" << "StartEvent" << start
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke 
             << sourceID << "AddObserver" << "EndEvent" << end
             << vtkClientServerStream::End;
      pm->DeleteStreamObject(execId, stream);
      }


    if (stream.GetNumberOfMessages() > 0)
      {
      pm->SendStream(this->ConnectionID, this->Servers, stream);
      }
    }
}



//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreateParts()
{
  this->CreatePartsInternal(this);
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::CreatePartsInternal(vtkSMProxy* op)
{
  if (this->PartsCreated && this->GetNumberOfParts())
    {
    return;
    }
  this->PartsCreated = 1;

  // This will only create objects if they are not already created.
  // This happens when connecting a filter to a source which is not
  // initialized. In other situations, SetInput() creates the VTK
  // objects before this gets called.
  op->CreateVTKObjects(1);


  this->PInternals->Parts.clear();

  int numIDs = op->GetNumberOfIDs();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkPVNumberOfOutputsInformation* info = vtkPVNumberOfOutputsInformation::New();

  // Create one part each output of each filter
  vtkClientServerStream stream;
  for (int i=0; i<numIDs; i++)
    {
    vtkClientServerID sourceID = op->GetID(i);
    // TODO replace this with UpdateInformation and OutputInformation
    // property.
    pm->GatherInformation(
      this->ConnectionID, this->Servers, info, sourceID);
    int numOutputs = info->GetNumberOfOutputs();
    for (int j=0; j<numOutputs; j++)
      {
      stream << vtkClientServerStream::Invoke << sourceID
             << "GetOutputPort" << j <<  vtkClientServerStream::End;
      vtkClientServerID portID = pm->GetUniqueID();
      stream << vtkClientServerStream::Assign << portID
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;

      vtkClientServerID producerID = pm->GetUniqueID();
      stream << vtkClientServerStream::Assign << producerID
             << sourceID
             << vtkClientServerStream::End;

      stream << vtkClientServerStream::Invoke << sourceID
             << "GetExecutive" <<  vtkClientServerStream::End;
      vtkClientServerID execID = pm->GetUniqueID();
      stream << vtkClientServerStream::Assign << execID
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;

      vtkSMPart* part = vtkSMPart::New();
      part->SetConnectionID(this->ConnectionID);
      part->SetServers(this->Servers);
      part->CreateVTKObjects(0);
      // Assign ids of various objects related to the output
      part->SetAlgorithmOutputID(portID);
      part->SetProducerID(producerID);
      part->SetExecutiveID(execID);
      part->SetPortIndex(j);
      this->PInternals->Parts.push_back(part);
      part->Delete();
      }
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->ConnectionID, op->GetServers(), stream);
    }
  info->Delete();

  vtkstd::vector<vtkSmartPointer<vtkSMPart> >::iterator it =
     this->PInternals->Parts.begin();

  if (this->DoInsertExtractPieces)
    {
    for(; it != this->PInternals->Parts.end(); it++)
      {
      it->GetPointer()->CreateTranslatorIfNecessary();
      if (strcmp(this->GetVTKClassName(), "vtkPVEnSightMasterServerReader"))
        {
        it->GetPointer()->InsertExtractPiecesIfNecessary();
        }
      }
    }

}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::CleanInputs(const char* method)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;
  int numSources = this->GetNumberOfIDs();

  for (int sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
    {
    vtkClientServerID sourceID = this->GetID(sourceIdx);
    stream << vtkClientServerStream::Invoke 
           << sourceID << method 
           << vtkClientServerStream::End;
    }

  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::AddInput(vtkSMSourceProxy *input, 
                                const char* method, 
                                int hasMultipleInputs)
{

  if (!input)
    {
    return;
    }

  input->CreateParts();
  int numInputs = input->GetNumberOfParts();
  if (!numInputs)
    {
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;
  if (hasMultipleInputs)
    {
    // One filter, multiple inputs
    this->CreateVTKObjects(1);
    vtkClientServerID sourceID = this->GetID(0);
    for (int partIdx = 0; partIdx < numInputs; ++partIdx)
      {
      vtkSMPart* part = input->GetPart(partIdx);
      stream << vtkClientServerStream::Invoke 
             << sourceID << method;
      stream << part->GetAlgorithmOutputID();
      stream << vtkClientServerStream::End;
      }
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }
  else
    {
    // n inputs, n filters
    this->CreateVTKObjects(numInputs);
    int numSources = this->GetNumberOfIDs();
    for (int sourceIdx = 0; sourceIdx < numSources; ++sourceIdx)
      {
      vtkClientServerID sourceID = this->GetID(sourceIdx);
      // This is to handle the case when there are multiple
      // inputs and the first one has multiple parts. For
      // example, in the Glyph filter, when the input has multiple
      // parts, the glyph source has to be applied to each.
      // NOTE: Make sure that you set the input which has as
      // many parts as there will be filters first. OR call
      // CreateVTKObjects() with the right number of inputs.
      int partIdx = sourceIdx % numInputs;
      vtkSMPart* part = input->GetPart(partIdx);
      stream << vtkClientServerStream::Invoke 
             << sourceID << method; 
      stream << part->GetAlgorithmOutputID();
      stream << vtkClientServerStream::End;
      }
    pm->SendStream(this->ConnectionID, (this->Servers & input->GetServers()), stream);
    }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  if (this->PartsCreated && !this->GetNumberOfParts())
    {
    this->UpdatePipeline();
    }

  this->Superclass::MarkModified(modifiedProxy);
  this->InvalidateDataInformation();
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::UpdateSelfAndAllInputs()
{
  this->Superclass::UpdateSelfAndAllInputs();
  this->UpdatePipelineInformation();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMSourceProxy::GetDataInformation()
{
  if (this->DataInformationValid == 0)
    {
    // Make sure the output filter is up-to-date before
    // getting information.
    this->UpdatePipeline();
    this->GatherDataInformation();
    }
  return this->DataInformation;
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::InvalidateDataInformation(int invalidateConsumers)
{
  if (invalidateConsumers)
    {
    unsigned int numConsumers = this->GetNumberOfConsumers();
    for (unsigned int i=0; i<numConsumers; i++)
      {
      vtkSMSourceProxy* cons = vtkSMSourceProxy::SafeDownCast(
        this->GetConsumerProxy(i));
      if (cons)
        {
        cons->InvalidateDataInformation(invalidateConsumers);
        }
      }
    }
  this->InvalidateDataInformation();
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::InvalidateDataInformation()
{
  this->DataInformationValid = 0;
  vtkstd::vector<vtkSmartPointer<vtkSMPart> >::iterator it =
    this->PInternals->Parts.begin();
  for (; it != this->PInternals->Parts.end(); it++)
    {
    it->GetPointer()->InvalidateDataInformation();
    }
}

//----------------------------------------------------------------------------
void vtkSMSourceProxy::GatherDataInformation()
{

  vtkProcessModule::GetProcessModule()->SendPrepareProgress(
    this->ConnectionID);
  this->DataInformation->Initialize();

  vtkstd::vector<vtkSmartPointer<vtkSMPart> >::iterator it =
    this->PInternals->Parts.begin();
  for (; it != this->PInternals->Parts.end(); it++)
    {
    this->DataInformation->AddInformation(
      it->GetPointer()->GetDataInformation(), 1);
    }
  this->DataInformationValid = 1;
  vtkProcessModule::GetProcessModule()->SendCleanupPendingProgress(
    this->ConnectionID);
}


//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSourceProxy::SaveRevivalState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* revivalElem = this->Superclass::SaveRevivalState(root);
  if (revivalElem)
    {
    vtkstd::vector<vtkSmartPointer<vtkSMPart> >::iterator it =
      this->PInternals->Parts.begin();
    for(; it != this->PInternals->Parts.end(); ++it)
      {
      vtkPVXMLElement* partsElement = vtkPVXMLElement::New();
      partsElement->SetName("Part");
      revivalElem->AddNestedElement(partsElement);
      partsElement->Delete();
      it->GetPointer()->SaveRevivalState(partsElement);
      }
    }
  return revivalElem;
}

//---------------------------------------------------------------------------
int vtkSMSourceProxy::LoadRevivalState(vtkPVXMLElement* revivalElem,
  vtkSMStateLoader* loader)
{
  if (!this->Superclass::LoadRevivalState(revivalElem, loader))
    {
    return 0;
    }

  unsigned int num_elems = revivalElem->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc <num_elems; cc++)
    {
    vtkPVXMLElement* curElement = revivalElem->GetNestedElement(cc);
    if (curElement->GetName() && strcmp(curElement->GetName(), "Part") == 0)
      {
      vtkSmartPointer<vtkSMPart> part = vtkSmartPointer<vtkSMPart>::New();
      part->SetConnectionID(this->ConnectionID);
      part->SetServers(this->Servers);
      if (part->LoadRevivalState(curElement->GetNestedElement(0), loader))
        {
        this->PInternals->Parts.push_back(part);
        }
      else
        {
        vtkErrorMacro("Failed to revive part");
        return 0;
        }
      }
    }
  this->PartsCreated = 1;
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
