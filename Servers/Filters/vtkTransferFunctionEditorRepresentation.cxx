/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTransferFunctionEditorRepresentation.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorRepresentation.h"

#include "vtkColorTransferFunction.h"
#include "vtkImageActor.h"

vtkCxxRevisionMacro(vtkTransferFunctionEditorRepresentation, "$Revision: 1.4 $");

vtkCxxSetObjectMacro(vtkTransferFunctionEditorRepresentation,
                     ColorFunction, vtkColorTransferFunction);

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation::vtkTransferFunctionEditorRepresentation()
{
  this->HistogramActor = vtkImageActor::New();
  this->HistogramActor->SetPosition(0, 0, -10);

  this->HistogramVisibility = 1;
  this->ScalarBinRange[0] = 1;
  this->ScalarBinRange[1] = 0;

  this->HistogramColor[0] = this->HistogramColor[1] = this->HistogramColor[2] =
    0.8;
  this->ColorFunction = NULL;
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentation::~vtkTransferFunctionEditorRepresentation()
{
  this->HistogramActor->Delete();
  this->SetColorFunction(NULL);
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentation::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  if (this->HistogramVisibility)
    {
    return this->HistogramActor->RenderOpaqueGeometry(viewport);
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentation::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  if (this->HistogramVisibility)
    {
    return this->HistogramActor->RenderTranslucentPolygonalGeometry(viewport);
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentation::HasTranslucentPolygonalGeometry()
{
  if (this->HistogramVisibility)
    {
    return this->HistogramActor->HasTranslucentPolygonalGeometry();
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentation::RenderOverlay(
  vtkViewport *viewport)
{
  if (this->HistogramVisibility)
    {
    return this->HistogramActor->RenderOverlay(viewport);
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DisplaySize: " << this->DisplaySize[0] << " "
     << this->DisplaySize[1] << endl;
}
