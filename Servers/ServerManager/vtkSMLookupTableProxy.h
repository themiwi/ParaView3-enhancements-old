/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMLookupTableProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLookupTableProxy - proxy for a VTK lookup table
// .SECTION Description
// This proxy class is an example of how vtkSMProxy can be subclassed
// to add functionality. It adds one simple method : Build().

#ifndef __vtkSMLookupTableProxy_h
#define __vtkSMLookupTableProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMLookupTableProxy : public vtkSMProxy
{
public:
  static vtkSMLookupTableProxy* New();
  vtkTypeRevisionMacro(vtkSMLookupTableProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the lookup table values.
  void Build();

protected:
  vtkSMLookupTableProxy();
  ~vtkSMLookupTableProxy();

private:
  vtkSMLookupTableProxy(const vtkSMLookupTableProxy&); // Not implemented
  void operator=(const vtkSMLookupTableProxy&); // Not implemented
};

#endif
