/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkAttributeDataToTableFilter.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAttributeDataToTableFilter.h"

#include "vtkObjectFactory.h"
#include "vtkGraph.h"
#include "vtkTable.h"
#include "vtkDataSet.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"

vtkStandardNewMacro(vtkAttributeDataToTableFilter);
vtkCxxRevisionMacro(vtkAttributeDataToTableFilter, "$Revision: 1.2 $");
//----------------------------------------------------------------------------
vtkAttributeDataToTableFilter::vtkAttributeDataToTableFilter()
{
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  this->AddMetaData = false;
}

//----------------------------------------------------------------------------
vtkAttributeDataToTableFilter::~vtkAttributeDataToTableFilter()
{
}

//----------------------------------------------------------------------------
vtkExecutive* vtkAttributeDataToTableFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
int vtkAttributeDataToTableFilter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}

//----------------------------------------------------------------------------
int vtkAttributeDataToTableFilter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  vtkFieldData* fieldData = this->GetSelectedField(input);

  if (fieldData)
    {
    vtkTable* output = vtkTable::GetData(outputVector);
    output->GetRowData()->ShallowCopy(fieldData);
    if (this->AddMetaData)
      {
      this->Decorate(output, input);
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
#define vtkAttributeDataToTableFilterValidate(type, method)\
{\
  type* __temp = type::SafeDownCast(input);\
  if (__temp)\
    {\
    return __temp->method();\
    }\
  return 0;\
}

//----------------------------------------------------------------------------
vtkFieldData* vtkAttributeDataToTableFilter::GetSelectedField(vtkDataObject* input)
{
  if (input)
    {
    switch (this->FieldAssociation)
      {
    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      vtkAttributeDataToTableFilterValidate(vtkDataSet, GetPointData);

    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      vtkAttributeDataToTableFilterValidate(vtkDataSet, GetCellData);

    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      return input->GetFieldData();

    case vtkDataObject::FIELD_ASSOCIATION_VERTICES:
      vtkAttributeDataToTableFilterValidate(vtkGraph, GetVertexData);

    case vtkDataObject::FIELD_ASSOCIATION_EDGES:
      vtkAttributeDataToTableFilterValidate(vtkGraph, GetEdgeData);

    case vtkDataObject::FIELD_ASSOCIATION_ROWS:
      vtkAttributeDataToTableFilterValidate(vtkTable, GetRowData);
      }
    }
  return 0;
}

#define VTK_MAX(x, y) ((x) > (y))? (x) : (y)
//----------------------------------------------------------------------------
void vtkAttributeDataToTableFilter::Decorate(vtkTable* output,
  vtkDataObject* input)
{
  vtkPointSet* psInput = vtkPointSet::SafeDownCast(input);
  vtkRectilinearGrid* rgInput = vtkRectilinearGrid::SafeDownCast(input);
  vtkImageData* idInput = vtkImageData::SafeDownCast(input);
  vtkStructuredGrid* sgInput = vtkStructuredGrid::SafeDownCast(input);
  const int* dimensions = 0;
  if (rgInput)
    {
    dimensions = rgInput->GetDimensions();
    }
  else if (idInput)
    {
    dimensions = idInput->GetDimensions();
    }
  else if (sgInput)
    {
    dimensions = sgInput->GetDimensions();
    }

  int cellDims[3];
  if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_CELLS &&
    dimensions)
    {
    cellDims[0] = VTK_MAX(1, (dimensions[0] -1));
    cellDims[1] = VTK_MAX(1, (dimensions[1] -1));
    cellDims[2] = VTK_MAX(1, (dimensions[2] -1));
    dimensions = cellDims;
    }

  if (this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS &&
    psInput && psInput->GetPoints())
    {
    output->GetRowData()->AddArray(psInput->GetPoints()->GetData());
    }

  if (dimensions)
    {
    // I cannot decide if this should be put in the vtkInformation associated
    // with the vtkTable or in FieldData. I'd rather the former but not sure
    // how that's going to be propagated through the pipeline.
    vtkIntArray* dArray = vtkIntArray::New();
    dArray->SetName("STRUCTURED_DIMENSIONS");
    dArray->SetNumberOfComponents(3);
    dArray->SetNumberOfTuples(1);
    dArray->SetTupleValue(0, dimensions);
    output->GetFieldData()->AddArray(dArray);
    dArray->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkAttributeDataToTableFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldAssociation: " << this->FieldAssociation << endl;
  os << indent << "AddMetaData: " << this->AddMetaData << endl;
}


