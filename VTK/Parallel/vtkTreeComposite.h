/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTreeComposite.h,v $
  Language:  C++
  Date:      $Date: 2002-04-24 11:30:14 $
  Version:   $Revision: 1.21 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTreeComposite - Implements tree based compositing.
// .SECTION Description
// vtkTreeComposite is a legacy class that is an emty subclass of
// vtkCompositeManager.  You should use vtkCompositeManager in the future.

#ifndef __vtkTreeComposite_h
#define __vtkTreeComposite_h

#include "vtkCompositeManager.h"


class VTK_PARALLEL_EXPORT vtkTreeComposite : public vtkCompositeManager
{
public:
  static vtkTreeComposite *New();
  vtkTypeRevisionMacro(vtkTreeComposite,vtkCompositeManager);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTreeComposite();
  ~vtkTreeComposite();
  vtkTreeComposite(const vtkTreeComposite&);
  void operator=(const vtkTreeComposite&);
  
};

#endif
