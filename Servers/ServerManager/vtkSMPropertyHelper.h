/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPropertyHelper.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
// .NAME vtkSMPropertyHelper - helper class to get/set property values.
// .SECTION Description
// vtkSMPropertyHelper is a helper class to get/set property values in a type
// independent fashion.
// eg.
// \code
//    vtkSMPropertyHelper(proxy, "Visibility").Set(0);
//    vtkSMPropertyHelper(proxy, "Input").Set(inputProxy, 0);
//
//    double center[3] = {...};
//    vtkSMPropertyHelper(proxy, "Center").Set(center, 3);
// \endcode
// .SECTION Caveat
// This class is not wrapped, hence not available in any of the wrapped
// languagues such as python.

#ifndef __vtkSMPropertyHelper_h
#define __vtkSMPropertyHelper_h

#include "vtkSMObject.h"

class vtkSMProperty;
class vtkSMProxy;

class VTK_EXPORT vtkSMPropertyHelper 
{
public:
  vtkSMPropertyHelper(vtkSMProxy* proxy, const char* name);
  ~vtkSMPropertyHelper();

  // Description:
  // Updates the property value by fetching the value from the server. This only
  // works for properties with \c InformationOnly attribute set to 1. For all
  // other properties, this has no effect.
  void UpdateValueFromServer();

  // Description:
  // Set the number of elements in the property. For vtkSMProxyProperty, this is
  // equivalent to SetNumberOfProxies().
  void SetNumberOfElements(unsigned int elems);

  // Description:
  // Get the number of elements in the property.
  // For vtkSMProxyProperty, this is equivalent to GetNumberOfProxies().
  unsigned int GetNumberOfElements();

  // Description:
  // Equivalent to SetNumberOfElements(0).
  void RemoveAllValues() { this->SetNumberOfElements(0); }

  // Description:
  // Set/Get methods with \c int API. Calling these method on
  // vtkSMStringVectorProperty or vtkSMProxyProperty will raise errors.
  void Set(int value)
    { this->Set(0, value); }
  void Set(unsigned int index, int value);
  void Set(const int* values, unsigned int count);
  int GetAsInt(unsigned int index = 0);

  // Description:
  // Set/Get methods with \c double API. Calling these method on
  // vtkSMStringVectorProperty or vtkSMProxyProperty will raise errors.
  void Set(double value)
    { this->Set(0, value); }
  void Set(unsigned int index, double value);
  void Set(const double* values, unsigned int count);
  double GetAsDouble(unsigned int index = 0);

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
  // Description:
  // Set/Get methods with \c vtkIdType API. Calling these method on
  // vtkSMStringVectorProperty or vtkSMProxyProperty will raise errors.
  void Set(vtkIdType value)
    { this->Set(0, value); }
  void Set(unsigned int index, vtkIdType value);
  void Set(const vtkIdType* values, unsigned int count);
#endif
  vtkIdType GetAsIdType(unsigned int index = 0);

  // Description:
  // Set/Get methods for vtkSMStringVectorProperty. Calling these methods on any
  // other type of property will raise errors.
  void Set(const char* value)
    { this->Set(0, value); }
  void Set(unsigned int index, const char* value);
  const char* GetAsString(unsigned int index = 0);

  // Description:
  // Set/Get methods for vtkSMProxyProperty or vtkSMInputProperty. 
  // Calling these methods on any other type of property will raise errors.
  // The option \c outputport(s) argument is used only for vtkSMInputProperty.
  void Set(vtkSMProxy* value, unsigned int outputport=0)
    { this->Set(0, value, outputport); }
  void Set(unsigned int index, vtkSMProxy* value, unsigned int outputport=0);
  void Set(vtkSMProxy** value, unsigned int count, unsigned int *outputports=NULL);
  void Add(vtkSMProxy* value, unsigned int outputport=0);
  vtkSMProxy* GetAsProxy(unsigned int index=0);
  unsigned int GetOutputPort(unsigned int index=0);
  
//BTX
private:
  vtkSMPropertyHelper(const vtkSMPropertyHelper&); // Not implemented
  void operator=(const vtkSMPropertyHelper&); // Not implemented
 
  enum PType {
    INT,
    DOUBLE,
    IDTYPE,
    STRING,
    PROXY,
    INPUT,
    NONE
  };

  vtkSMProxy* Proxy;
  vtkSMProperty* Property;
  PType Type;
//ETX
};

#endif
