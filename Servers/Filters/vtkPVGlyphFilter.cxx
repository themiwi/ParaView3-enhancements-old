/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVGlyphFilter.cxx,v $
  Language:  C++
  Date:      $Date: 2003-11-13 19:12:11 $
  Version:   $Revision: 1.6 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGlyphFilter.h"

#include "vtkMaskPoints.h"
#include "vtkObjectFactory.h"
#include "vtkMultiProcessController.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkPVGlyphFilter, "$Revision: 1.6 $");
vtkStandardNewMacro(vtkPVGlyphFilter);

vtkPVGlyphFilter::vtkPVGlyphFilter()
{
  this->SetColorModeToColorByScalar();
  this->SetScaleModeToScaleByVector();
  this->MaskPoints = vtkMaskPoints::New();
  this->MaximumNumberOfPoints = 5000;
  this->NumberOfProcesses =
    vtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses();
  this->UseMaskPoints = 1;
}

vtkPVGlyphFilter::~vtkPVGlyphFilter()
{
  this->MaskPoints->Delete();
}

void vtkPVGlyphFilter::SetInput(vtkDataSet *input)
{
  this->MaskPoints->SetInput(input);
  this->Superclass::SetInput(this->MaskPoints->GetOutput());
}

void vtkPVGlyphFilter::SetRandomMode(int mode)
{
  this->MaskPoints->SetRandomMode(mode);
}

int vtkPVGlyphFilter::GetRandomMode()
{
  return this->MaskPoints->GetRandomMode();
}

void vtkPVGlyphFilter::Execute()
{
  if (this->UseMaskPoints)
    {
    this->Superclass::SetInput(this->MaskPoints->GetOutput());
    vtkIdType numPts = this->MaskPoints->GetInput()->GetNumberOfPoints();
    vtkIdType maxNumPts =
      this->MaximumNumberOfPoints / this->NumberOfProcesses;
    maxNumPts = (maxNumPts < 1) ? 1 : maxNumPts;
    this->MaskPoints->SetMaximumNumberOfPoints(maxNumPts);
    this->MaskPoints->SetOnRatio(numPts / maxNumPts);
    this->MaskPoints->Update();
    }
  else
    {
    this->Superclass::SetInput(this->MaskPoints->GetInput());
    }
  
  this->Superclass::Execute();
}

void vtkPVGlyphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputScalarsSelection: " 
     << (this->InputScalarsSelection ? this->InputScalarsSelection : "(none)")
     << endl;

  os << indent << "InputVectorsSelection: " 
     << (this->InputVectorsSelection ? this->InputVectorsSelection : "(none)")
     << endl;

  os << indent << "InputNormalsSelection: " 
     << (this->InputNormalsSelection ? this->InputNormalsSelection : "(none)")
     << endl;
  
  os << indent << "MaximumNumberOfPoints: " << this->GetMaximumNumberOfPoints()
     << endl;

  os << indent << "UseMaskPoints: " << (this->UseMaskPoints?"on":"off") << endl;

  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
}
