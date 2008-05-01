/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkCompositeDataToUnstructuredGridFilter.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataToUnstructuredGridFilter -  appends all
// vtkUnstructuredGrid leaves of the input composite dataset to a single
// vtkUnstructuredGrid.
// .SECTION Description
// vtkCompositeDataToUnstructuredGridFilter appends all vtkUnstructuredGrid
// leaves of the input composite dataset to a single unstructure grid. The
// subtree to be combined can be choosen using the SubTreeCompositeIndex. If 
// the SubTreeCompositeIndex is a leaf node, then no appending is required.

#ifndef __vtkCompositeDataToUnstructuredGridFilter_h
#define __vtkCompositeDataToUnstructuredGridFilter_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkCompositeDataSet;
class VTK_EXPORT vtkCompositeDataToUnstructuredGridFilter : 
  public vtkUnstructuredGridAlgorithm
{
public:
  static vtkCompositeDataToUnstructuredGridFilter* New();
  vtkTypeRevisionMacro(vtkCompositeDataToUnstructuredGridFilter, 
    vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(SubTreeCompositeIndex, unsigned int);
  vtkGetMacro(SubTreeCompositeIndex, unsigned int);
//BTX
protected:
  vtkCompositeDataToUnstructuredGridFilter();
  ~vtkCompositeDataToUnstructuredGridFilter();

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  void ExecuteSubTree(vtkCompositeDataSet* cd, vtkUnstructuredGrid* output);
  unsigned int SubTreeCompositeIndex;
private:
  vtkCompositeDataToUnstructuredGridFilter(const vtkCompositeDataToUnstructuredGridFilter&); // Not implemented
  void operator=(const vtkCompositeDataToUnstructuredGridFilter&); // Not implemented
//ETX
};

#endif

