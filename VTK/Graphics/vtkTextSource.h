/*=========================================================================

  Program:   Visualization Library
  Module:    $RCSfile: vtkTextSource.h,v $
  Language:  C++
  Date:      $Date: 1995-05-05 16:42:54 $
  Version:   $Revision: 1.2 $

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlTextSource - create a Text centered at the origin
// .SECTION Description
// vlTextSource converts a text string into polygons.  This way you can 
// insert text into your renderings.

#ifndef __vlTextSource_h
#define __vlTextSource_h

#include "PolySrc.hh"

class vlTextSource : public vlPolySource 
{
public:
  vlTextSource();
  char *GetClassName() {return "vlTextSource";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Set/Get the text to be drawn.
  vlSetStringMacro(Text);
  vlGetStringMacro(Text);

  // Description:
  // Controlls whether or not a background is drawn with the text.
  vlSetMacro(Backing,int);
  vlGetMacro(Backing,int);
  vlBooleanMacro(Backing,int);

protected:
  void Execute();
  char *Text;
  int   Backing;
};

#endif


