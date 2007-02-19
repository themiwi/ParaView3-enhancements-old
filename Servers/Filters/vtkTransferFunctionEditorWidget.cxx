/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTransferFunctionEditorWidget.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorWidget.h"

#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkTransferFunctionEditorRepresentation.h"

vtkCxxRevisionMacro(vtkTransferFunctionEditorWidget, "$Revision: 1.7 $");

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidget::vtkTransferFunctionEditorWidget()
{
  this->NumberOfScalarBins = 10000;
  this->WholeScalarRange[0] = this->VisibleScalarRange[0] = 1;
  this->WholeScalarRange[1] = this->VisibleScalarRange[1] = 0;
  this->ModificationType = OPACITY;
  this->OpacityFunction = vtkPiecewiseFunction::New();
  this->ColorFunction = vtkColorTransferFunction::New();
  this->Histogram = NULL;
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidget::~vtkTransferFunctionEditorWidget()
{
  this->OpacityFunction->Delete();
  this->ColorFunction->Delete();
  this->SetHistogram(NULL);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::SetVisibleScalarRange(double min,
                                                            double max)
{
  if (min == this->VisibleScalarRange[0] && max == this->VisibleScalarRange[1])
    {
    return;
    }

  if (this->WholeScalarRange[0] > this->WholeScalarRange[1])
    {
    this->SetWholeScalarRange(min, max);
    }

  this->VisibleScalarRange[0] = min;
  this->VisibleScalarRange[1] = max;
  this->Modified();

  vtkTransferFunctionEditorRepresentation *rep =
    vtkTransferFunctionEditorRepresentation::SafeDownCast(this->WidgetRep);
  if (this->Histogram)
    {
    double histogramRange[2];
    int numBins;
    vtkDataArray *dataArray = this->Histogram->GetXCoordinates();
    if (dataArray)
      {
      dataArray->GetRange(histogramRange);
      numBins = dataArray->GetNumberOfTuples() - 1;

      if (rep)
        {
        rep->SetScalarBinRange(
          static_cast<int>((min - histogramRange[0]) * numBins /
                           (histogramRange[1] - histogramRange[0])),
          static_cast<int>((max - histogramRange[0]) * numBins /
                           (histogramRange[1] - histogramRange[0])));
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::ShowWholeScalarRange()
{
  if (this->Histogram)
    {
    double range[2];
    vtkDataArray *dataArray = this->Histogram->GetXCoordinates();
    if (dataArray)
      {
      dataArray->GetRange(range);
      this->SetVisibleScalarRange(range);
      }
    }
  else
    {
    this->SetVisibleScalarRange(this->WholeScalarRange);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::Configure(int size[2])
{
  vtkTransferFunctionEditorRepresentation *rep =
    vtkTransferFunctionEditorRepresentation::SafeDownCast(this->WidgetRep);
  if (rep)
    {
    rep->SetDisplaySize(size);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::SetHistogram(
  vtkRectilinearGrid *histogram)
{
  if (this->Histogram != histogram)
    {
    vtkRectilinearGrid *tempHist = this->Histogram;
    this->Histogram = histogram;
    if (this->Histogram != NULL)
      {
      this->Histogram->Register(this);
      vtkDataArray *hist = this->Histogram->GetXCoordinates();
      if (hist &&
          this->VisibleScalarRange[0] == 1 && this->VisibleScalarRange[1] == 0)
        {
        double range[2];
        hist->GetRange(range);
        this->SetVisibleScalarRange(range);
        }
      }
    if (tempHist != NULL)
      {
      tempHist->UnRegister(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "VisibleScalarRange: " << this->VisibleScalarRange[0] << " "
     << this->VisibleScalarRange[1] << endl;
  os << indent << "WholeScalarRange: " << this->WholeScalarRange[0] << " "
     << this->WholeScalarRange[1] << endl;
}
