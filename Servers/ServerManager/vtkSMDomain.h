/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMDomain.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDomain - represents the possible values a property can have
// .SECTION Description
// vtkSMDomain is an abstract class that describes the "domain" of a
// a widget. A domain is a collection of possible values a property
// can have.
// .SECTION See Also
// vtkSMProxyGroupDomain

#ifndef __vtkSMDomain_h
#define __vtkSMDomain_h

#include "vtkSMObject.h"

class vtkSMProperty;
class vtkPVXMLElement;

class VTK_EXPORT vtkSMDomain : public vtkSMObject
{
public:
  vtkTypeRevisionMacro(vtkSMDomain, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Is the value of the property in the domain? Overwritten by
  // sub-classes.
  virtual int IsInDomain(vtkSMProperty* property) = 0;

protected:
  vtkSMDomain();
  ~vtkSMDomain();

//BTX
  friend class vtkSMProperty;
//ETX

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkPVXMLElement*) {return 1;};

  char* XMLName;

  // Description:
  // Assigned by the XML parser. The name assigned in the XML
  // configuration. Can be used to figure out the origin of the
  // domain.
  vtkSetStringMacro(XMLName);

private:
  vtkSMDomain(const vtkSMDomain&); // Not implemented
  void operator=(const vtkSMDomain&); // Not implemented
};

#endif
