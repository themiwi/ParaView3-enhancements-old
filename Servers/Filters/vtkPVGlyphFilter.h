/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVGlyphFilter.h,v $
  Language:  C++
  Date:      $Date: 2003-10-01 20:55:53 $
  Version:   $Revision: 1.4 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGlyphFilter -
#ifndef __vtkPVGlyphFilter_h
#define __vtkPVGlyphFilter_h

#include "vtkGlyph3D.h"

class vtkMaskPoints;

class VTK_EXPORT vtkPVGlyphFilter : public vtkGlyph3D
{
public:
  vtkTypeRevisionMacro(vtkPVGlyphFilter,vtkGlyph3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  static vtkPVGlyphFilter *New();

  // Description:
  // If you want to use an arbitrary scalars array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}

  // Description:
  // If you want to use an arbitrary vectors array, then set its name here.
  // By default this in NULL and the filter will use the active vector array.
  vtkGetStringMacro(InputVectorsSelection);
  void SelectInputVectors(const char *fieldName) 
    {this->SetInputVectorsSelection(fieldName);}

  // Description:
  // If you want to use an arbitrary normals array, then set its name here.
  // By default this in NULL and the filter will use the active normal array.
  vtkGetStringMacro(InputNormalsSelection);
  void SelectInputNormals(const char *fieldName) 
    {this->SetInputNormalsSelection(fieldName);}

  // Description:
  // Limit the number of points to glyph
  vtkSetMacro(MaximumNumberOfPoints, int);
  vtkGetMacro(MaximumNumberOfPoints, int);

  // Description:
  // Set the input to this filter.
  virtual void SetInput(vtkDataSet *input);
  
  // Description:
  // Get the number of processes used to run this filter.
  vtkGetMacro(NumberOfProcesses, int);
  
  // Description:
  // Set/get whether to mask points
  vtkSetMacro(UseMaskPoints, int);
  vtkGetMacro(UseMaskPoints, int);

  // Description:
  // Set/get flag to cause randomization of which points to mask.
  void SetRandomMode(int mode);
  int GetRandomMode();

protected:
  vtkPVGlyphFilter();
  ~vtkPVGlyphFilter();

  virtual void Execute();
  
  vtkMaskPoints *MaskPoints;
  int MaximumNumberOfPoints;
  int NumberOfProcesses;
  int UseMaskPoints;
  
private:
  vtkPVGlyphFilter(const vtkPVGlyphFilter&);  // Not implemented.
  void operator=(const vtkPVGlyphFilter&);  // Not implemented.
};

#endif
