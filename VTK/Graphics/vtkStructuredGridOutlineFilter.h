/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkStructuredGridOutlineFilter.h,v $
  Language:  C++
  Date:      $Date: 2002-01-22 15:29:49 $
  Version:   $Revision: 1.33 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStructuredGridOutlineFilter - create wireframe outline for structured grid
// .SECTION Description
// vtkStructuredGridOutlineFilter is a filter that generates a wireframe 
// outline of a structured grid (vtkStructuredGrid). Structured data is 
// topologically a cube, so the outline will have 12 "edges".

#ifndef __vtkStructuredGridOutlineFilter_h
#define __vtkStructuredGridOutlineFilter_h

#include "vtkStructuredGridToPolyDataFilter.h"

class VTK_GRAPHICS_EXPORT vtkStructuredGridOutlineFilter : public vtkStructuredGridToPolyDataFilter
{
public:
  static vtkStructuredGridOutlineFilter *New();
  vtkTypeRevisionMacro(vtkStructuredGridOutlineFilter,vtkStructuredGridToPolyDataFilter);

protected:
  vtkStructuredGridOutlineFilter() {};
  ~vtkStructuredGridOutlineFilter() {};

  void Execute();
private:
  vtkStructuredGridOutlineFilter(const vtkStructuredGridOutlineFilter&);  // Not implemented.
  void operator=(const vtkStructuredGridOutlineFilter&);  // Not implemented.
};

#endif


