/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPart.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPart - proxy for a data object
// .SECTION Description
// This object manages one vtk data set. It is used internally
// by vtkSMSourceProxy to manage all of it's outputs.

#ifndef __vtkSMPart_h
#define __vtkSMPart_h

#include "vtkSMProxy.h"

class vtkPVDataInformation;

class VTK_EXPORT vtkSMPart : public vtkSMProxy
{
public:
  static vtkSMPart* New();
  vtkTypeRevisionMacro(vtkSMPart, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  vtkPVDataInformation* GetDataInformation();
  //ETX
  
  // Description:
  void GatherDataInformation();

  // Description:
  void InvalidateDataInformation();

protected:
  vtkSMPart();
  ~vtkSMPart();

  vtkSMPart(const vtkSMPart&); // Not implemented
  void operator=(const vtkSMPart&); // Not implemented

  vtkPVDataInformation *DataInformation;
  int DataInformationValid;
};

#endif
