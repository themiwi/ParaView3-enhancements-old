/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkCTHSource.h,v $
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
// .NAME vtkCTHSource - A source super class.
// .SECTION Description
// Supper class for CTH readers and filters.

#ifndef __vtkCTHSource_h
#define __vtkCTHSource_h

#include "vtkSource.h"

class vtkCTHData;

class VTK_EXPORT vtkCTHSource : public vtkSource
{
public:
  static vtkCTHSource *New();

  vtkTypeRevisionMacro(vtkCTHSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this source.
  vtkCTHData *GetOutput();
  vtkCTHData *GetOutput(int idx);
  void SetOutput(vtkCTHData *output);  

protected:
  vtkCTHSource();
  ~vtkCTHSource();

private:
  void InternalImageDataCopy(vtkCTHSource *src);
private:
  vtkCTHSource(const vtkCTHSource&);  // Not implemented.
  void operator=(const vtkCTHSource&);  // Not implemented.
};


#endif



