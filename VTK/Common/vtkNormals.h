/*=========================================================================

  Program:   Visualization Library
  Module:    $RCSfile: vtkNormals.h,v $
  Language:  C++
  Date:      $Date: 1994-04-14 07:54:58 $
  Version:   $Revision: 1.5 $

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Abstract interface to 3D normals.
//
#ifndef __vlNormals_h
#define __vlNormals_h

#include "Object.hh"

class vlIdList;
class vlFloatNormals;

class vlNormals : public vlObject 
{
public:
  virtual ~vlNormals() {};
  virtual vlNormals *MakeObject(int sze, int ext=1000) = 0;
  virtual int GetNumberOfNormals() = 0;
  virtual float *GetNormal(int i) = 0;
  virtual void SetNormal(int i,float x[3]) = 0;     // fast insert
  virtual void InsertNormal(int i, float x[3]) = 0; // allocates memory as necessary
  virtual void Squeeze() = 0;

  void GetNormals(vlIdList& ptId, vlFloatNormals& fp);
  char *GetClassName() {return "vlNormals";};
  void PrintSelf(ostream& os, vlIndent indent);
};

#endif
