/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSelectionConverter.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSelectionConverter - converts one selection type to another
// .SECTION Description
// vtkSelectionConverter can be used to convert from one selection type
// to another. Currently, it only supports conversion from a 'surface' 
// geometry selection to a 'volume' selection. It does this by looking for a 
// pedigree array called vtkOriginalCellIds that says what 3D cell produced 
// each selected 2D surface cell.
// .SECTION See Also
// vtkSelection

#ifndef __vtkSelectionConverter_h
#define __vtkSelectionConverter_h

#include "vtkObject.h"

class vtkSelection;

class VTK_EXPORT vtkSelectionConverter : public vtkObject
{
public:
  static vtkSelectionConverter* New();
  vtkTypeRevisionMacro(vtkSelectionConverter,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convert input selection and store it in output. Currently, the
  // input selection must be a geometry selection and the output
  // is a volume selection.
  void Convert(vtkSelection* input, vtkSelection* output);

protected:
  vtkSelectionConverter();
  ~vtkSelectionConverter();

private:
  vtkSelectionConverter(const vtkSelectionConverter&);  // Not implemented.
  void operator=(const vtkSelectionConverter&);  // Not implemented.
};

#endif
