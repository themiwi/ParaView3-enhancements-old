/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMScatterPlotArraysInformationHelper.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMScatterPlotArraysInformationHelper
// .SECTION Description
//

#ifndef __vtkSMScatterPlotArraysInformationHelper_h
#define __vtkSMScatterPlotArraysInformationHelper_h

#include "vtkSMInformationHelper.h"

class VTK_EXPORT vtkSMScatterPlotArraysInformationHelper : public vtkSMInformationHelper
{
public:
  static vtkSMScatterPlotArraysInformationHelper* New();
  vtkTypeRevisionMacro(vtkSMScatterPlotArraysInformationHelper, vtkSMInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Updates the property using value obtained for server. It creates
  // an instance of the server helper class vtkPVServerArraySelection
  // and passes the objectId (which the helper class gets as a pointer)
  // and populates the property using the values returned.
  // Each array is represented by two components:
  // name, state (on/off)  
  virtual void UpdateProperty(
    vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop);
  //ETX

//BTX
protected:
  vtkSMScatterPlotArraysInformationHelper();
  virtual ~vtkSMScatterPlotArraysInformationHelper();

private:
  vtkSMScatterPlotArraysInformationHelper(const vtkSMScatterPlotArraysInformationHelper&); // Not implemented
  void operator=(const vtkSMScatterPlotArraysInformationHelper&); // Not implemented
//ETX
};

#endif

