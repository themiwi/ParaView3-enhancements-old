/*=========================================================================

  Program:   Visualization Library
  Module:    $RCSfile: vtkStructuredGridOutlineFilter.h,v $
  Language:  C++
  Date:      $Date: 1994-09-26 16:11:42 $
  Version:   $Revision: 1.2 $

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStructuredOutlineFilter - create wireframe outline for structured data
// .SECTION Description
// vlStructuredOutlineFilter is a filter that generates a wireframe outline of
// structured (i.e., topologically regular) data. Structured data is 
// topologically a cube, so the outline will have 12 "edges".

#ifndef __vlStructuredOutlineFilter_h
#define __vlStructuredOutlineFilter_h

#include "SD2PolyF.hh"

class vlStructuredOutlineFilter : public vlStructuredDataToPolyFilter
{
public:
  vlStructuredOutlineFilter() {};
  ~vlStructuredOutlineFilter() {};
  char *GetClassName() {return "vlStructuredOutlineFilter";};

protected:
  void Execute();
};

#endif


