/*=========================================================================

  Program:   Visualization Library
  Module:    $RCSfile: vtkCylinderSource.h,v $
  Language:  C++
  Date:      $Date: 1994-03-12 19:01:38 $
  Version:   $Revision: 1.8 $

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Created cylinder centered at origin
//
#ifndef __vlCylinderSource_h
#define __vlCylinderSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_CELL_SIZE

class vlCylinderSource : public vlPolySource 
{
public:
  vlCylinderSource(int res=6);
  char *GetClassName() {return "vlCylinderSource";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlSetClampMacro(Height,float,0.0,LARGE_FLOAT)
  vlGetMacro(Height,float);

  vlSetClampMacro(Radius,float,0.0,LARGE_FLOAT)
  vlGetMacro(Radius,float);

  vlSetClampMacro(Resolution,int,0,MAX_RESOLUTION)
  vlGetMacro(Resolution,int);

  vlSetMacro(Capping,int);
  vlGetMacro(Capping,int);
  vlBooleanMacro(Capping,int);

protected:
  void Execute();
  float Height;
  float Radius;
  int Resolution;
  int Capping;

};

#endif


