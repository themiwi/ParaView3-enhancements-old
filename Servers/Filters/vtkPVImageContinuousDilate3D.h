/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVImageContinuousDilate3D.h,v $
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
// .NAME vtkPVImageContinuousDilate3D -

#ifndef __vtkPVImageContinuousDilate3D_h
#define __vtkPVImageContinuousDilate3D_h


#include "vtkImageContinuousDilate3D.h"

class VTK_EXPORT vtkPVImageContinuousDilate3D : public vtkImageContinuousDilate3D
{
public:

  // Description:
  static vtkPVImageContinuousDilate3D *New();
  vtkTypeRevisionMacro(vtkPVImageContinuousDilate3D,vtkImageSpatialFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // If you want to dilate by an arbitrary point scalar array, 
  // then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}  
  
protected:
  vtkPVImageContinuousDilate3D();
  ~vtkPVImageContinuousDilate3D();

private:
  vtkPVImageContinuousDilate3D(const vtkPVImageContinuousDilate3D&);  // Not implemented.
  void operator=(const vtkPVImageContinuousDilate3D&);  // Not implemented.
};

#endif



