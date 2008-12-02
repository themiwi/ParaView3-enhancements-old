/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVExtractSelection.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtractSelection.h"

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkHierarchicalBoxDataIterator.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

class vtkPVExtractSelection::vtkSelectionNodeVector : 
  public vtkstd::vector<vtkSmartPointer<vtkSelectionNode> >
{
};


vtkCxxRevisionMacro(vtkPVExtractSelection, "$Revision: 1.12 $");
vtkStandardNewMacro(vtkPVExtractSelection);

//----------------------------------------------------------------------------
vtkPVExtractSelection::vtkPVExtractSelection()
{
  this->SetNumberOfOutputPorts(3);
}

//----------------------------------------------------------------------------
vtkPVExtractSelection::~vtkPVExtractSelection()
{
}

//----------------------------------------------------------------------------
int vtkPVExtractSelection::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  if (port==0)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
    }
  else
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkSelection");
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVExtractSelection::RequestDataObject(
  vtkInformation* request,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestDataObject(request, inputVector, outputVector))
    {
    return 0;
    }

  // Second and third outputs are selections
  for (int i = 1; i <= 2; ++i)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkSelection *selOut = vtkSelection::GetData(info);
    if (!selOut || !selOut->IsA("vtkSelection")) 
      {
      vtkDataObject* newOutput = vtkSelection::New();
      if (!newOutput)
        {
        vtkErrorMacro("Could not create vtkSelectionOutput");
        return 0;
        }
      newOutput->SetPipelineInformation(info);
      this->GetOutputPortInformation(i)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      newOutput->Delete();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVExtractSelection::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector ,
  vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestData(request, inputVector, outputVector))
    {
    return 0;
    }


  vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
  vtkSelection* sel = vtkSelection::GetData(inputVector[1], 0);

  vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(inputDO);
  vtkCompositeDataSet* cdOutput = vtkCompositeDataSet::GetData(outputVector, 0);
  vtkDataSet *geomOutput = vtkDataSet::GetData(outputVector, 0);

  //make an ids selection for the second output
  //we can do this because all of the extractSelectedX filters produce 
  //arrays called "vtkOriginalXIds" that record what input cells produced 
  //each output cell, at least as long as PRESERVE_TOPOLOGY is off
  //when we start allowing PreserveTopology, this will have to instead run 
  //through the vtkInsidedNess arrays, and for every on entry, record the
  //entries index
  vtkSelection *output = vtkSelection::SafeDownCast(
    outputVector->GetInformationObject(1)->Get(vtkDataObject::DATA_OBJECT()));

  output->Initialize();

  if (!sel)
    {
    return 1;
    }

  // Simply pass the input selection into the third output
  vtkSelection *passThroughSelection = vtkSelection::SafeDownCast(
    outputVector->GetInformationObject(2)->Get(vtkDataObject::DATA_OBJECT()));
  passThroughSelection->ShallowCopy(sel);

  // If input selection content type is vtkSelectionNode::BLOCKS, then we simply
  // need to shallow copy the input as the output.
  if (this->GetContentType(sel) == vtkSelectionNode::BLOCKS)
    {
    output->ShallowCopy(sel);
    return 1;
    }

  vtkSelectionNodeVector oVector;
  if (cdOutput)
    {
    // For composite datasets, the output of this filter is
    // vtkSelectionNode::SELECTIONS instance with vtkSelection instances for some
    // nodes in the composite dataset. COMPOSITE_INDEX() or
    // HIERARCHICAL_LEVEL(), HIERARCHICAL_INDEX() keys are set on each of the
    // vtkSelection instances correctly to help identify the block they came
    // from.
    vtkCompositeDataIterator* iter = cdInput->NewIterator();
    vtkHierarchicalBoxDataIterator* hbIter = 
      vtkHierarchicalBoxDataIterator::SafeDownCast(iter);
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
      iter->GoToNextItem())
      {
      vtkSelectionNode* curSel = this->LocateSelection(iter->GetCurrentFlatIndex(),
        sel);
      if (!curSel && hbIter)
        {
        curSel = this->LocateSelection(hbIter->GetCurrentLevel(), hbIter->GetCurrentIndex(),
          sel);
        }

      geomOutput = vtkDataSet::SafeDownCast(cdOutput->GetDataSet(iter));
      if (curSel && geomOutput)
        {
        vtkSelectionNodeVector curOVector;
        vtkSelectionNodeVector::iterator viter;

        this->RequestDataInternal(curOVector, geomOutput, curSel);

        for (viter = curOVector.begin(); viter != curOVector.end(); ++viter)
          {
          // RequestDataInternal() will not set COMPOSITE_INDEX() for
          // hierarchical datasets.
          viter->GetPointer()->GetProperties()->Set(vtkSelectionNode::COMPOSITE_INDEX(),
            iter->GetCurrentFlatIndex());
          oVector.push_back(viter->GetPointer());
          }
        }
      }
    iter->Delete();
    }
  else if (geomOutput)
    {
    unsigned int numNodes = sel->GetNumberOfNodes();
    for (unsigned int i = 0; i < numNodes; i++)
      {
      this->RequestDataInternal(oVector, geomOutput, sel->GetNode(i));
      }
    }

  vtkSelectionNodeVector::iterator iter;
  for (iter = oVector.begin(); iter != oVector.end(); ++iter)
    {
    output->AddNode(iter->GetPointer());
    }

  return 1;
}

//----------------------------------------------------------------------------
vtkSelectionNode* vtkPVExtractSelection::LocateSelection(unsigned int level,
  unsigned int index, vtkSelection* sel)
{
  unsigned int numNodes = sel->GetNumberOfNodes();
  for (unsigned int cc=0; cc < numNodes; cc++)
    {
    vtkSelectionNode* node = sel->GetNode(cc);
    if (node)
      {
      if (node->GetProperties()->Has(vtkSelectionNode::HIERARCHICAL_LEVEL()) &&
          node->GetProperties()->Has(vtkSelectionNode::HIERARCHICAL_INDEX()) &&
          static_cast<unsigned int>(node->GetProperties()->Get(vtkSelectionNode::HIERARCHICAL_LEVEL())) == 
          level &&
          static_cast<unsigned int>(node->GetProperties()->Get(vtkSelectionNode::HIERARCHICAL_INDEX())) == 
          index)
        {
        return node;
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkSelectionNode* vtkPVExtractSelection::LocateSelection(unsigned int composite_index,
  vtkSelection* sel)
{
  unsigned int numNodes = sel->GetNumberOfNodes();
  for (unsigned int cc=0; cc < numNodes; cc++)
    {
    vtkSelectionNode* node = sel->GetNode(cc);
    if (node)
      {
      if (node->GetProperties()->Has(vtkSelectionNode::COMPOSITE_INDEX()) &&
          node->GetProperties()->Get(vtkSelectionNode::COMPOSITE_INDEX()) == 
          static_cast<int>(composite_index))
        {
        return node;
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::RequestDataInternal(vtkSelectionNodeVector& outputs,
  vtkDataSet* geomOutput, vtkSelectionNode* sel)
{
  // DON'T CLEAR THE outputs.

  int ft = vtkSelectionNode::CELL;
  if (sel && sel->GetProperties()->Has(vtkSelectionNode::FIELD_TYPE()))
    {
    ft = sel->GetProperties()->Get(vtkSelectionNode::FIELD_TYPE());
    }

  if (geomOutput && ft == vtkSelectionNode::CELL)
    {
    vtkSelectionNode* output = vtkSelectionNode::New();
    output->GetProperties()->Copy(sel->GetProperties(), /*deep=*/1);
    output->SetContentType(vtkSelectionNode::INDICES);
    vtkIdTypeArray *oids = vtkIdTypeArray::SafeDownCast(
      geomOutput->GetCellData()->GetArray("vtkOriginalCellIds"));
    if (oids)
      {
      output->SetSelectionList(oids);
      outputs.push_back(output);
      }
    output->Delete();
    }

  // no else, since original point indices are always passed.
  if (geomOutput)
    {
    vtkSelectionNode* output = vtkSelectionNode::New();
    output->GetProperties()->Copy(sel->GetProperties(), /*deep=*/1);
    output->SetFieldType(vtkSelectionNode::POINT);
    output->SetContentType(vtkSelectionNode::INDICES);
    vtkIdTypeArray* oids = vtkIdTypeArray::SafeDownCast(
      geomOutput->GetPointData()->GetArray("vtkOriginalPointIds"));
    if (oids)
      {
      output->SetSelectionList(oids);
      outputs.push_back(output);
      }
    output->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkPVExtractSelection::GetContentType(vtkSelection* sel)
{
  int ctype = -1;
  unsigned int numNodes = sel->GetNumberOfNodes();
  for (unsigned int cc=0; cc < numNodes; cc++)
    {
    vtkSelectionNode* node = sel->GetNode(cc);
    int nodeCType = node->GetContentType();
    if (ctype == -1)
      {
      ctype = nodeCType;
      }
    else if (nodeCType != ctype)
      {
      return 0;
      }
    }
  return ctype;
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

