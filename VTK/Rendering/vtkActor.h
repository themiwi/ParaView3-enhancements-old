/*=========================================================================

  Program:   OSCAR 
  Module:    $RCSfile: vtkActor.h,v $
  Language:  C++
  Date:      $Date: 1994-01-12 12:48:50 $
  Version:   $Revision: 1.1 $

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#ifndef __vlActor_hh
#define __vlActor_hh

#include "Property.hh"
#include "Mapper.hh"

class vlRenderer;
class vlMapper;

class vlActor : public vlObject
{
 public:
  vlActor();
  void Render(vlRenderer *ren);
  int GetVisibility();
  void GetCompositeMatrix(float mat[4][4]);
  void SetMapper(vlMapper *m);
  vlMapper *GetMapper();
  vlProperty *MyProperty; 

 protected:
  vlMapper *Mapper;
  float Origin[3];
  float Position[3];
  float Orientation[3];
  float Scale[3];
  int   Visibility;
};

#endif

