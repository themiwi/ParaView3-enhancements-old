/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkExtractHistogramExtentTranslator.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractHistogramExtentTranslator.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkExtractHistogramExtentTranslator);
vtkCxxRevisionMacro(vtkExtractHistogramExtentTranslator, "$Revision: 1.1 $");
//-----------------------------------------------------------------------------
vtkExtractHistogramExtentTranslator::vtkExtractHistogramExtentTranslator()
{
}

//-----------------------------------------------------------------------------
vtkExtractHistogramExtentTranslator::~vtkExtractHistogramExtentTranslator()
{
}

//-----------------------------------------------------------------------------
void vtkExtractHistogramExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
