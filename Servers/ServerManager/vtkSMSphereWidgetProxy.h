/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSphereWidgetProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSphereWidgetProxy
// .SECTION Description

#ifndef __vtkSMSphereWidgetProxy_h
#define __vtkSMSphereWidgetProxy_h


#include "vtkSM3DWidgetProxy.h"

class VTK_EXPORT vtkSMSphereWidgetProxy : public vtkSM3DWidgetProxy
{
public:
  static vtkSMSphereWidgetProxy* New();
  vtkTypeRevisionMacro(vtkSMSphereWidgetProxy, vtkSM3DWidgetProxy);
  void PrintSelf(ostream &os,vtkIndent indent);

  // Description:
  // Get/Set the Center
  vtkSetVector3Macro(Center,double);
  vtkGetVector3Macro(Center,double);

  // Description:
  // Get/Set the Radius
  vtkSetMacro(Radius,double);
  vtkGetMacro(Radius,double);

  virtual void SaveInBatchScript(ofstream *file);

  virtual void UpdateVTKObjects();
protected:
  vtkSMSphereWidgetProxy();
  ~vtkSMSphereWidgetProxy();

  // Description:
  // Overloaded to update the property values before saving state
  virtual void SaveState(const char* name, ostream* file, vtkIndent indent);
  
  // Description:
  // Execute event of the 3D Widget.
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);
  virtual void CreateVTKObjects(int numObjects); 

  double Center[3];
  double Radius;

private:
  vtkSMSphereWidgetProxy(const vtkSMSphereWidgetProxy&);// Not implemented
  void operator=(const vtkSMSphereWidgetProxy&); // Not implemented
};  

#endif
