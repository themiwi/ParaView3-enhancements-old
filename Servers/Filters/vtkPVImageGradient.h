/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVImageGradient.h,v $
  Language:  C++
  Date:      $Date: 2003-01-09 19:06:42 $
  Version:   $Revision: 1.1 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVImageGradient -

#ifndef __vtkPVImageGradient_h
#define __vtkPVImageGradient_h


#include "vtkImageGradient.h"

class VTK_EXPORT vtkPVImageGradient : public vtkImageGradient
{
public:
  static vtkPVImageGradient *New();
  vtkTypeRevisionMacro(vtkPVImageGradient,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // If you want to compute the gradient of an arbitrary point scalar array, 
  // then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}  

protected:
  vtkPVImageGradient();
  ~vtkPVImageGradient() {};

private:
  vtkPVImageGradient(const vtkPVImageGradient&);  // Not implemented.
  void operator=(const vtkPVImageGradient&);  // Not implemented.
};

#endif



