/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVImageContinuousErode3D.h,v $
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
// .NAME vtkPVImageContinuousErode3D - 

#ifndef __vtkPVImageContinuousErode3D_h
#define __vtkPVImageContinuousErode3D_h


#include "vtkImageContinuousErode3D.h"

class VTK_EXPORT vtkPVImageContinuousErode3D : public vtkImageContinuousErode3D
{
public:
  // Description:
  // Construct an instance of vtkPVImageContinuousErode3D filter.
  // By default zero values are eroded.
  static vtkPVImageContinuousErode3D *New();
  vtkTypeRevisionMacro(vtkPVImageContinuousErode3D,vtkImageSpatialFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // If you want to erode by an arbitrary point scalar array, 
  // then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}  
  
protected:
  vtkPVImageContinuousErode3D();
  ~vtkPVImageContinuousErode3D();

private:
  vtkPVImageContinuousErode3D(const vtkPVImageContinuousErode3D&);  // Not implemented.
  void operator=(const vtkPVImageContinuousErode3D&);  // Not implemented.
};

#endif



