/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVGlyphFilter.cxx,v $
  Language:  C++
  Date:      $Date: 2003-09-25 16:24:50 $
  Version:   $Revision: 1.3 $

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
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkPVGlyphFilter, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkPVGlyphFilter);

vtkPVGlyphFilter::vtkPVGlyphFilter()
{
  this->SetColorModeToColorByScalar();
  this->SetScaleModeToScaleByVector();
  this->MaskPoints = vtkMaskPoints::New();
  this->MaximumNumberOfPoints = 5000;
  this->NumberOfProcesses = 1;
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

void vtkPVGlyphFilter::Execute()
{
  vtkIdType numPts = this->MaskPoints->GetInput()->GetNumberOfPoints();
  vtkIdType maxNumPts = this->MaximumNumberOfPoints / this->NumberOfProcesses;
  maxNumPts = (maxNumPts < 1) ? 1 : maxNumPts;
  this->MaskPoints->SetMaximumNumberOfPoints(maxNumPts);
  this->MaskPoints->SetOnRatio(numPts / maxNumPts);
  this->MaskPoints->Update();
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
}
