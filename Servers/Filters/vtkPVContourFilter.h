/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVContourFilter.h,v $
  Language:  C++
  Date:      $Date: 2003-01-09 19:06:42 $
  Version:   $Revision: 1.15 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVContourFilter -
#ifndef __vtkPVContourFilter_h
#define __vtkPVContourFilter_h

#include "vtkContourFilter.h"

class VTK_EXPORT vtkPVContourFilter : public vtkContourFilter
{
public:
  vtkTypeRevisionMacro(vtkPVContourFilter,vtkContourFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkPVContourFilter *New();

  // Description:
  // If you want to contour by an arbitrary array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}
  
protected:
  vtkPVContourFilter();
  ~vtkPVContourFilter();

private:
  vtkPVContourFilter(const vtkPVContourFilter&);  // Not implemented.
  void operator=(const vtkPVContourFilter&);  // Not implemented.
};


#endif


