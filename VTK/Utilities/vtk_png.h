/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtk_png.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtk_png_h
#define __vtk_png_h

/* Use the png library configured for VTK.  */
#include "vtkToolkits.h"
#ifdef VTK_USE_SYSTEM_PNG
# include <png.h>
#else
# include <vtkpng/png.h>
#endif

#endif
