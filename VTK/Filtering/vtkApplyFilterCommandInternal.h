/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkApplyFilterCommandInternal.h,v $
  Language:  C++
  Date:      $Date: 2003-12-11 15:47:37 $
  Version:   $Revision: 1.1 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkApplyFilterCommandInternal_h
#define __vtkApplyFilterCommandInternal_h

#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>

class vtkApplyFilterCommandInternal
{
public:
  typedef vtkstd::vector<vtkstd::string> FilterTypesVector;
  typedef vtkstd::map<vtkstd::string, FilterTypesVector> FilterTypesMap;

  FilterTypesMap FilterTypes;
};


#endif
