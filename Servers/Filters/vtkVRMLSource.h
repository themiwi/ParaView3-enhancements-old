/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkVRMLSource.h,v $
  Language:  C++
  Date:      $Date: 2003-05-05 16:45:38 $
  Version:   $Revision: 1.3 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVRMLSource - Converts importer to a source.
// .SECTION Description
// Since paraview can only use vtkSources, I am wrapping the VRML importer
// as a source.  I will loose lights, texture maps and colors,

#ifndef __vtkVRMLSource_h
#define __vtkVRMLSource_h

#include "vtkSource.h"
class vtkVRMLImporter;
class vtkPolyData;

class VTK_EXPORT vtkVRMLSource : public vtkSource
{
public:
  vtkTypeRevisionMacro(vtkVRMLSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkVRMLSource *New();

  // Description:
  // VRML file name.  Set
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  int GetNumberOfOutputs();
  vtkPolyData* GetOutput(int idx);
  vtkPolyData* GetOutput() { return this->GetOutput(0);}

  // Description: 
  // Descided whether to generate color arrays or not.
  vtkSetMacro(Color,int);
  vtkGetMacro(Color,int);
  vtkBooleanMacro(Color,int);

  // Description:
  // This method allows all parts to be put into a single output.
  // By default this flag is on.
  vtkSetMacro(Append,int);
  vtkGetMacro(Append,int);
  vtkBooleanMacro(Append,int);

protected:
  vtkVRMLSource();
  ~vtkVRMLSource();

  void Execute();
  void InitializeImporter();
  void CopyImporterToOutputs();

  char* FileName;
  vtkVRMLImporter *Importer;
  int Color;
  int Append;

private:
  vtkVRMLSource(const vtkVRMLSource&);  // Not implemented.
  void operator=(const vtkVRMLSource&);  // Not implemented.
};

#endif

