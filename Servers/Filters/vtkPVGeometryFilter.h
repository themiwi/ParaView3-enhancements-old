/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVGeometryFilter.h,v $
  Language:  C++
  Date:      $Date: 2003-10-31 17:08:34 $
  Version:   $Revision: 1.8 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGeometryFilter - Geometry filter that does outlines for volumes.
// .SECTION Description
// This filter defaults to using the outline filter unless the input
// is a structured volume.

#ifndef __vtkPVGeometryFilter_h
#define __vtkPVGeometryFilter_h

#include "vtkDataSetSurfaceFilter.h"

class vtkImageData;
class vtkStructuredGrid;
class vtkRectilinearGrid;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkPVGeometryFilter : public vtkDataSetSurfaceFilter
{
public:
  static vtkPVGeometryFilter *New();
  vtkTypeRevisionMacro(vtkPVGeometryFilter,vtkDataSetSurfaceFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This flag is set during the execute method.  It indicates
  // that the input was 3d and an outline representation was used.
  vtkGetMacro(OutlineFlag, int);

  // Description:
  // Set/get whether to produce outline (vs. surface).
  vtkSetMacro(UseOutline, int);
  vtkGetMacro(UseOutline, int);
  
protected:
  vtkPVGeometryFilter();
  ~vtkPVGeometryFilter();

  void Execute();
  void DataSetExecute(vtkDataSet *input);
  void ImageDataExecute(vtkImageData *input);
  void StructuredGridExecute(vtkStructuredGrid *input);
  void RectilinearGridExecute(vtkRectilinearGrid *input);
  void UnstructuredGridExecute(vtkUnstructuredGrid *input);
  void PolyDataExecute(vtkPolyData *input);

  int OutlineFlag;
  int UseOutline;
  
private:
  vtkPVGeometryFilter(const vtkPVGeometryFilter&); // Not implemented
  void operator=(const vtkPVGeometryFilter&); // Not implemented
};

#endif


