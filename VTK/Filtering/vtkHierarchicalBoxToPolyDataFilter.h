/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkHierarchicalBoxToPolyDataFilter.h,v $
  Language:  C++
  Date:      $Date: 2003-12-11 15:47:37 $
  Version:   $Revision: 1.1 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalBoxToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkHierarchicalBoxToPolyDataFilter is an abstract filter class
// whose subclasses take as input vtkHierarchicalBoxDataSet and generate
// polygonal data on output.

#ifndef __vtkHierarchicalBoxToPolyDataFilter_h
#define __vtkHierarchicalBoxToPolyDataFilter_h

#include "vtkPolyDataSource.h"
 
class vtkHierarchicalBoxDataSet;

class VTK_FILTERING_EXPORT vtkHierarchicalBoxToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeRevisionMacro(vtkHierarchicalBoxToPolyDataFilter,
                       vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkHierarchicalBoxDataSet *input);
  vtkHierarchicalBoxDataSet *GetInput();
  
protected:
  vtkHierarchicalBoxToPolyDataFilter(){this->NumberOfRequiredInputs = 1;};
  ~vtkHierarchicalBoxToPolyDataFilter() {};
  
private:
  vtkHierarchicalBoxToPolyDataFilter(const vtkHierarchicalBoxToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkHierarchicalBoxToPolyDataFilter&);  // Not implemented.
};

#endif


