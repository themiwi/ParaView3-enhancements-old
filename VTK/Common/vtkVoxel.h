/*=========================================================================

  Program:   Visualization Library
  Module:    $RCSfile: vtkVoxel.h,v $
  Language:  C++
  Date:      $Date: 1994-03-03 20:04:59 $
  Version:   $Revision: 1.1 $

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Computational class for bricks.
//
#ifndef __vlBrick_h
#define __vlBrick_h

#include "Cell.hh"

class vlBrick : public vlCell
{
public:
  vlBrick() {};
  char *GetClassName() {return "vlBrick";};

  float DistanceToPoint(float *x);

};

#endif


