/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVWarpScalar.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVWarpScalar -
// .SECTION Description

#ifndef __vtkPVWarpScalar_h
#define __vtkPVWarpScalar_h

#include "vtkWarpScalar.h"

class vtkDataArray;

class VTK_EXPORT vtkPVWarpScalar : public vtkWarpScalar
{
public:
  static vtkPVWarpScalar *New();
  vtkTypeRevisionMacro(vtkPVWarpScalar,vtkWarpScalar);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If you want to warp by an arbitrary scalar array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}
  
protected:
  vtkPVWarpScalar();
  ~vtkPVWarpScalar();

private:
  vtkPVWarpScalar(const vtkPVWarpScalar&);  // Not implemented.
  void operator=(const vtkPVWarpScalar&);  // Not implemented.
};

#endif
