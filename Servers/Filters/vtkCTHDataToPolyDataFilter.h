/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkCTHDataToPolyDataFilter.h,v $
  Language:  C++
  Date:      $Date: 2003-09-05 20:07:52 $
  Version:   $Revision: 1.1 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHDataToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkCTHDataToPolyDataFilter is an abstract filter class whose
// subclasses take as input datasets of type vtkCTHData and 
// generate polygonal data on output.

// .SECTION See Also
// vtkContourGrid

#ifndef __vtkCTHDataToPolyDataFilter_h
#define __vtkCTHDataToPolyDataFilter_h

#include "vtkPolyDataSource.h"

class vtkCTHData;

class VTK_EXPORT vtkCTHDataToPolyDataFilter : public vtkPolyDataSource
{
public:
  static vtkCTHDataToPolyDataFilter *New();

  vtkTypeRevisionMacro(vtkCTHDataToPolyDataFilter,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkCTHData *input);
  vtkCTHData *GetInput();
  
  // Description:
  // Do not let datasets return more than requested.
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkCTHDataToPolyDataFilter() {this->NumberOfRequiredInputs = 1;}
  ~vtkCTHDataToPolyDataFilter() {}
  
  
private:
  vtkCTHDataToPolyDataFilter(const vtkCTHDataToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkCTHDataToPolyDataFilter&);  // Not implemented.
};

#endif


