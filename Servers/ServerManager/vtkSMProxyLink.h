/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMProxyLink.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyLink -
// .SECTION Description

#ifndef __vtkSMProxyLink_h
#define __vtkSMProxyLink_h

#include "vtkSMLink.h"

//BTX
struct vtkSMProxyLinkInternals;
//ETX

class VTK_EXPORT vtkSMProxyLink : public vtkSMLink
{
public:
  static vtkSMProxyLink* New();
  vtkTypeRevisionMacro(vtkSMProxyLink, vtkSMLink);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a property to the link. updateDir determines whether a property of
  // the proxy is read or written. When a property of an input proxy
  // changes, it's value is pushed to all other output proxies in the link.
  void AddLinkedProxy(vtkSMProxy* proxy, int updateDir);

 
protected:
  vtkSMProxyLink();
  ~vtkSMProxyLink();

  virtual void UpdateVTKObjects(vtkSMProxy* caller);
  virtual void UpdateProperties(vtkSMProxy* caller, const char* pname);

  // Description:
  // Save the state of the link.
  virtual void SaveState(const char* linkname, vtkPVXMLElement* parent);

  // Description:
  // Load the link state.
  virtual int LoadState(vtkPVXMLElement* linkElement, vtkSMStateLoader* loader);
private:
  vtkSMProxyLinkInternals* Internals;

  vtkSMProxyLink(const vtkSMProxyLink&); // Not implemented
  void operator=(const vtkSMProxyLink&); // Not implemented
};

#endif
