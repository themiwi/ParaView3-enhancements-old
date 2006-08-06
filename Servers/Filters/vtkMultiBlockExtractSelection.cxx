/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMultiBlockExtractSelection.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockExtractSelection.h"

#include "vtkDataSet.h"
#include "vtkExtractSelection.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataExtractSelection.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkMultiBlockExtractSelection, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkMultiBlockExtractSelection);
vtkCxxSetObjectMacro(vtkMultiBlockExtractSelection, Selection,vtkSelection);

//----------------------------------------------------------------------------
vtkMultiBlockExtractSelection::vtkMultiBlockExtractSelection()
{
  this->SetNumberOfInputPorts(0);
  this->Selection = 0;
  this->PolyDataExtractFilter = vtkPolyDataExtractSelection::New();
  this->ExtractFilter = vtkExtractSelection::New();
} 

//----------------------------------------------------------------------------
vtkMultiBlockExtractSelection::~vtkMultiBlockExtractSelection()
{
  this->SetSelection(NULL);
  this->PolyDataExtractFilter->Delete();
  this->ExtractFilter->Delete();
}

//----------------------------------------------------------------------------
int vtkMultiBlockExtractSelection::RequestInformation(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkMultiBlockExtractSelection::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkSelection* sel = this->Selection;
  if ( ! sel )
    {
    vtkErrorMacro(<<"No selection specified");
    return 1;
    }

  vtkInformation* info = outputVector->GetInformationObject(0);

  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(
    info->Get(vtkDataObject::DATA_OBJECT()));

  vtkInformation* prop = sel->GetProperties();
  if (!prop->Has(vtkSelection::CONTENT_TYPE()) ||
      prop->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::SELECTIONS)
    {
    return 1;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  unsigned int numChildren = sel->GetNumberOfChildren();
  unsigned int idx=0;

  for (unsigned int i=0; i<numChildren; i++)
    {
    vtkDataSet* selDS = this->SelectFromDataSet(sel->GetChild(i));
    if (selDS)
      {
      mb->SetDataSet(idx, pm->GetPartitionId(), selDS);
      selDS->Delete();
      idx++;
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
vtkDataSet* vtkMultiBlockExtractSelection::SelectFromDataSet(
  vtkSelection* sel)
{
  vtkInformation* prop = sel->GetProperties();
  if (!prop->Has(vtkSelection::SOURCE_ID()))
    {
    return 0;
    }

  vtkClientServerID id;
  id.ID = prop->Get(vtkSelection::SOURCE_ID());

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(
    pm->GetObjectFromID(id));
  if (!alg)
    {
    return 0;
    }

  vtkDataSet* input = vtkDataSet::SafeDownCast(
    alg->GetOutputDataObject(0));
  if (!input)
    {
    return 0;
    }

  vtkDataSet* inputCopy = input->NewInstance();
  inputCopy->ShallowCopy(input);

  if (input->IsA("vtkPolyData"))
    {
    this->PolyDataExtractFilter->SetSelection(sel);
    this->PolyDataExtractFilter->SetInput(inputCopy);
    this->PolyDataExtractFilter->Update();
    this->PolyDataExtractFilter->SetInput(0);
    inputCopy->Delete();
    
    vtkPolyData* output = vtkPolyData::SafeDownCast(
      this->PolyDataExtractFilter->GetOutputDataObject(0));
    if (output)
      {
      vtkPolyData* outputCopy = vtkPolyData::New();
      outputCopy->ShallowCopy(output);
      output->Initialize();
      return outputCopy;
      }
    }
  else
    {
    this->ExtractFilter->SetSelection(sel);
    this->ExtractFilter->SetInput(inputCopy);
    this->ExtractFilter->Update();
    this->ExtractFilter->SetInput(0);
    inputCopy->Delete();
    
    vtkUnstructuredGrid* output = vtkUnstructuredGrid::SafeDownCast(
      this->ExtractFilter->GetOutputDataObject(0));
    if (output)
      {
      vtkUnstructuredGrid* outputCopy = vtkUnstructuredGrid::New();
      outputCopy->ShallowCopy(output);
      output->Initialize();
      return outputCopy;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkMultiBlockExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Selection: ";
  if (this->Selection)
    {
    this->Selection->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
unsigned long vtkMultiBlockExtractSelection::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long impFuncMTime;

  if ( this->Selection != NULL )
    {
    impFuncMTime = this->Selection->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
    }

  return mTime;
}
