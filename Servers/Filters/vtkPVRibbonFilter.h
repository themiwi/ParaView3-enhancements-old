/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVRibbonFilter.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRibbonFilter - 

#ifndef __vtkPVRibbonFilter_h
#define __vtkPVRibbonFilter_h

#include "vtkRibbonFilter.h"

class VTK_EXPORT vtkPVRibbonFilter : public vtkRibbonFilter 
{
public:
  vtkTypeRevisionMacro(vtkPVRibbonFilter,vtkRibbonFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkPVRibbonFilter *New();

  // Description:
  // If you want to use an arbitrary normals array, then set its name here.
  // By default this in NULL and the filter will use the active normal array.
  vtkGetStringMacro(InputVectorsSelection);
  void SelectInputVectors(const char *fieldName) 
    {this->SetInputVectorsSelection(fieldName);}

protected:
  vtkPVRibbonFilter();
  ~vtkPVRibbonFilter();

  
private:
  vtkPVRibbonFilter(const vtkPVRibbonFilter&);  // Not implemented.
  void operator=(const vtkPVRibbonFilter&);  // Not implemented.
};

#endif


