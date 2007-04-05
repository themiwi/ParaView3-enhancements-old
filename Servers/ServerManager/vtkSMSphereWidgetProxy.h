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
// vtkSMSphereWidgetProxy is the proxy for vtkSphereWidget. 
// It maintains iVars for Center and Radius of the vtkSphereWidget.
// These values are pushed onto the vtkSphereWidget on
// UpdateVTKObjects(). 
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

  virtual void UpdateVTKObjects();
protected:
  vtkSMSphereWidgetProxy();
  ~vtkSMSphereWidgetProxy();

  // Description:
  // Overloaded to update the property values before saving state
  virtual vtkPVXMLElement* SaveState(vtkPVXMLElement* root);
  
  // Description:
  // Execute event of the 3D Widget.
  // When the user interacts with the 3DWidget on the client, events are fired.
  // Since this class listens to such events, it leads to a call to ExecuteEvent.
  // This method updates the iVars based on the values on the client 3DWidget and
  // calls Superclass ExecuteEvent which triggers a WidgetModifiedEvent indicating that
  // widget has been manipulated. 
  virtual void ExecuteEvent(vtkObject*, unsigned long, void*);

  virtual void CreateVTKObjects(int numObjects); 

  double Center[3];
  double Radius;

private:
  vtkSMSphereWidgetProxy(const vtkSMSphereWidgetProxy&);// Not implemented
  void operator=(const vtkSMSphereWidgetProxy&); // Not implemented
};  

#endif
