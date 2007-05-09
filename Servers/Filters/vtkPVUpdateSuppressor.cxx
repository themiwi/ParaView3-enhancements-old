/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVUpdateSuppressor.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVUpdateSuppressor.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCacheSizeKeeper.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataObject.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUpdateSuppressorPipeline.h"

#include <vtkstd/map>

class vtkPVUpdateSuppressorCacheMap : 
  public vtkstd::map<double, vtkSmartPointer<vtkDataObject> >
{
public:
  unsigned long GetActualMemorySize() 
    {
    unsigned long actual_size = 0;
    vtkPVUpdateSuppressorCacheMap::iterator iter;
    for (iter = this->begin(); iter != this->end(); ++iter)
      {
      actual_size += iter->second.GetPointer()->GetActualMemorySize();
      }
    return actual_size;
    }
};

vtkCxxRevisionMacro(vtkPVUpdateSuppressor, "$Revision: 1.49 $");
vtkStandardNewMacro(vtkPVUpdateSuppressor);
vtkCxxSetObjectMacro(vtkPVUpdateSuppressor, CacheSizeKeeper, vtkCacheSizeKeeper);
//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::vtkPVUpdateSuppressor()
{
  this->UpdatePiece = 0;
  this->UpdateNumberOfPieces = 1;

  this->UpdateTime = 0.0;
  this->UpdateTimeInitialized = false;

  this->CachedGeometry = NULL;
  this->CachedGeometryLength = 0;

  this->Cache = new vtkPVUpdateSuppressorCacheMap();

  this->Enabled = 1;

  this->SaveCacheOnCacheUpdate = 1;
  this->CacheSizeKeeper = 0;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (pm)
    {
    this->SetCacheSizeKeeper(
      vtkProcessModule::GetProcessModule()->GetCacheSizeKeeper());
    }
}

//----------------------------------------------------------------------------
vtkPVUpdateSuppressor::~vtkPVUpdateSuppressor()
{
  this->RemoveAllCaches();

  // Unset cache keeper only after having cleared the cache.
  this->SetCacheSizeKeeper(0);

  delete this->Cache;
  this->Cache = 0;
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::SetUpdateTime(double utime)
{
  this->UpdateTimeInitialized = true;
  if (this->UpdateTime != utime)
    {
    this->Modified();
    this->UpdateTime = utime;
    }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::SetEnabled(int enable)
{
  if (this->Enabled == enable)
    {
    return;
    }
  this->Enabled = enable;
  this->Modified();
  vtkUpdateSuppressorPipeline* executive = 
    vtkUpdateSuppressorPipeline::SafeDownCast(this->GetExecutive());
  if (executive)
    {
    executive->SetEnabled(enable);
    }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::ForceUpdate()
{  
  if (vtkProcessModule::GetStreamBlock())
    {
    return;
    }
  
  // Make sure that output type matches input type
  this->UpdateInformation();

  vtkDataObject *input = this->GetInput();
  if (input == 0)
    {
    vtkErrorMacro("No valid input.");
    return;
    }
  vtkDataObject *output = this->GetOutput();

  // int fixme; // I do not like this hack.  How can we get rid of it?
  // Assume the input is the collection filter.
  // Client needs to modify the collection filter because it is not
  // connected to a pipeline.
  vtkAlgorithm *source = input->GetProducerPort()->GetProducer();
  if (source &&
      (source->IsA("vtkMPIMoveData") ||
       source->IsA("vtkCollectPolyData") ||
       source->IsA("vtkMPIDuplicatePolyData") ||
       source->IsA("vtkM2NDuplicate") ||
       source->IsA("vtkM2NCollect") ||
       source->IsA("vtkMPIDuplicateUnstructuredGrid") ||
       source->IsA("vtkPVDuplicatePolyData") ||
       source->IsA("vtkOrderedCompositeDistributor") || 
       source->IsA("vtkClientServerMoveData")))
    {
    source->Modified();
    }

  vtkInformation* info = input->GetPipelineInformation();
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(
      info->GetExecutive(vtkExecutive::PRODUCER()));
  if (sddp)
    {
    sddp->SetUpdateExtent(info,
                          this->UpdatePiece, 
                          this->UpdateNumberOfPieces, 
                          0);
    }
  else
    {
    input->SetUpdatePiece(this->UpdatePiece);
    input->SetUpdateNumberOfPieces(this->UpdateNumberOfPieces);
    input->SetUpdateGhostLevel(0);
    }
  if (this->UpdateTimeInitialized)
    {
    info->Set(vtkCompositeDataPipeline::UPDATE_TIME_STEPS(), &this->UpdateTime, 1);
    }

  input->Update();
  // Input may have changed, we obtain the pointer again.
  input = this->GetInput();

  output->ShallowCopy(input);
  this->PipelineUpdateTime.Modified();
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::RemoveAllCaches()
{
  int idx;

  unsigned long freed_size = 0;
  for (idx = 0; idx < this->CachedGeometryLength; ++idx)
    {
    if (this->CachedGeometry[idx])
      {
      freed_size += this->CachedGeometry[idx]->GetActualMemorySize();
      this->CachedGeometry[idx]->Delete();
      this->CachedGeometry[idx] = NULL;
      }
    }

  if (this->CachedGeometry)
    {
    delete [] this->CachedGeometry;
    this->CachedGeometry = NULL;
    }
  this->CachedGeometryLength = 0;

  if (freed_size > 0 && this->CacheSizeKeeper)
    {
    this->CacheSizeKeeper->FreeCacheSize(freed_size);
    }


  // Clean new style cache.
  freed_size = this->Cache->GetActualMemorySize();
  this->Cache->clear();
  if (freed_size > 0 && this->CacheSizeKeeper)
    {
    this->CacheSizeKeeper->FreeCacheSize(freed_size);
    }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::CacheUpdate()
{
  if (this->UpdateTimeInitialized)
    {
    // No caching without update time.
    this->ForceUpdate();
    return;
    }

  vtkPVUpdateSuppressorCacheMap::iterator iter = this->Cache->find(
    this->UpdateTime);
  if (iter == this->Cache->end())
    {
    // No cache present, force update.

    this->ForceUpdate();
  
    if (this->SaveCacheOnCacheUpdate)
      {
      vtkDataObject* output = this->GetOutput();
      vtkSmartPointer<vtkDataObject> cache;
      cache.TakeReference(output->NewInstance());
      cache->ShallowCopy(output);

      // FIXME: I am commenting this code as I don't know what it's doing.
      // // Compositing seems to update the input properly.
      // // But this update is needed when doing animation without compositing.
      // cache->Update();
      (*this->Cache)[this->UpdateTime] = cache;
      }
    }
  else
    {
    // Using the cached data.
    vtkDataObject* output = this->GetOutput();
    output->ShallowCopy(iter->second.GetPointer());
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::CacheUpdate(int idx, int num)
{
  vtkDataObject *pd;
  vtkDataObject *output;
  int j;

  if (num == -1)
    {
    return;
    }

  if (idx < 0 || idx >= num)
    {
    vtkErrorMacro("Bad cache index: " << idx << " of " << num);
    return;
    }

  if (num != this->CachedGeometryLength)
    {
    this->RemoveAllCaches();
    this->CachedGeometry = new vtkDataObject*[num];
    for (j = 0; j < num; ++j)
      {
      this->CachedGeometry[j] = NULL;
      }
    this->CachedGeometryLength = num;
    }

  output = this->GetOutput();
  pd = this->CachedGeometry[idx];
  if (pd == NULL)
    { // we need to update and save.
    this->ForceUpdate();

    pd = output->NewInstance();
    pd->ShallowCopy(output);
    //  Compositing seems to update the input properly.
    //  But this update is needed when doing animation without compositing.
    pd->Update(); 
    if (this->SaveCacheOnCacheUpdate)
      {
      this->CachedGeometry[idx] = pd;
      if (this->CacheSizeKeeper)
        {
        this->CacheSizeKeeper->AddCacheSize(pd->GetActualMemorySize());
        }
      pd->Register(this);
      }
    pd->Delete();
    }
  else
    { // Output generated previously.
    output->ShallowCopy(pd);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVUpdateSuppressor::CreateDefaultExecutive()
{
  vtkUpdateSuppressorPipeline* executive = vtkUpdateSuppressorPipeline::New();
  executive->SetEnabled(this->Enabled);
  return executive;
}


//----------------------------------------------------------------------------
int vtkPVUpdateSuppressor::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* vtkNotUsed(outputVector))
{
  if (!this->Enabled)
    {
    vtkInformation* info = inputVector[0]->GetInformationObject(0);
    vtkStreamingDemandDrivenPipeline* sddp = 
      vtkStreamingDemandDrivenPipeline::SafeDownCast(
        info->GetExecutive(vtkExecutive::PRODUCER()));
    if (sddp)
      {
      sddp->SetUpdateExtent(info, this->UpdatePiece, 
        this->UpdateNumberOfPieces, 0);
      return 1;
      }
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVUpdateSuppressor::RequestDataObject(
  vtkInformation* vtkNotUsed(reqInfo), 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* outInfo = outputVector->GetInformationObject(i);
      vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
    
      if (!output || !output->IsA(input->GetClassName())) 
        {
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation(outInfo);
        newOutput->Delete();
        this->GetOutputPortInformation(0)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
        }
      }
    return 1;
    }
  return 0;

}

//----------------------------------------------------------------------------
int vtkPVUpdateSuppressor::RequestData(vtkInformation *request,
                                       vtkInformationVector **inputVector,
                                       vtkInformationVector *outputVector)
{
  // RequestData is not normally called. If it is called under a special
  // condition (for example, streaming), shallow copy input to output.
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());

  if (!this->Enabled 
    || outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()) > 1)
    {
    output->ShallowCopy(input);
    return 1;
    }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkPVUpdateSuppressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "UpdatePiece: " << this->UpdatePiece << endl;
  os << indent << "UpdateNumberOfPieces: " << this->UpdateNumberOfPieces << endl;
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "CacheSizeKeeper: " << this->CacheSizeKeeper << endl;
  os << indent << "SaveCacheOnCacheUpdate: " << this->SaveCacheOnCacheUpdate << endl;
  os << indent << "UpdateTime: " << this->UpdateTime << endl;
}
