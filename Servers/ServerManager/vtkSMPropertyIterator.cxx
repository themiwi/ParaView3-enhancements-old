/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPropertyIterator.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPropertyIterator.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyInternals.h"

vtkStandardNewMacro(vtkSMPropertyIterator);
vtkCxxRevisionMacro(vtkSMPropertyIterator, "$Revision: 1.6 $");

struct vtkSMPropertyIteratorInternals
{
  vtkSMProxyInternals::PropertyInfoMap::iterator PropertyIterator;
  vtkSMProxyInternals::ProxyMap::iterator ProxyIterator;
  vtkSMProxyInternals::PropertyInfoMap::iterator SubPropertyIterator;
};

//---------------------------------------------------------------------------
vtkSMPropertyIterator::vtkSMPropertyIterator()
{
  this->Proxy = 0;
  this->Internals = new vtkSMPropertyIteratorInternals;
  this->TraverseSubProxies = 1;
}

//---------------------------------------------------------------------------
vtkSMPropertyIterator::~vtkSMPropertyIterator()
{
  this->SetProxy(0);
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMPropertyIterator::SetProxy(vtkSMProxy* proxy)
{
  if (this->Proxy != proxy)
    {
    if (this->Proxy != NULL) { this->Proxy->UnRegister(this); }
    this->Proxy = proxy;
    if (this->Proxy != NULL) 
      { 
      this->Proxy->Register(this); 
      this->Begin();
      }
    this->Modified();
    }
}

//---------------------------------------------------------------------------
void vtkSMPropertyIterator::Begin()
{
  if (!this->Proxy)
    {
    vtkErrorMacro("Proxy is not set. Can not perform operation: Begin()");
    return;
    }

  this->Internals->PropertyIterator = 
    this->Proxy->Internals->Properties.begin(); 
  this->Internals->ProxyIterator = 
    this->Proxy->Internals->SubProxies.begin(); 

  if (this->TraverseSubProxies)
    {
    // Go to the first sub-proxy that is not empty.
    // And to the first exposed property.
    while (this->Internals->ProxyIterator != 
           this->Proxy->Internals->SubProxies.end())
      {
      this->Internals->SubPropertyIterator = 
        this->Internals->ProxyIterator->second->Internals->Properties.begin();
     
      int found_exposed = 0;
      while ( this->Internals->SubPropertyIterator !=
           this->Internals->ProxyIterator->second->Internals->Properties.end())
        {
        // iterate until an exposed property is found.
        if (this->Internals->ProxyIterator->second->Internals->
          ExposedPropertyNames.find(
            this->Internals->SubPropertyIterator->first.c_str()) !=
          this->Internals->ProxyIterator->second->Internals->
          ExposedPropertyNames.end())
          {
          found_exposed = 1;
          break;
          }
        this->Internals->SubPropertyIterator++;
        }
      if (found_exposed)
        {
        break;
        }
      this->Internals->ProxyIterator++;
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMPropertyIterator::IsAtEnd()
{
  if (!this->Proxy)
    {
    vtkErrorMacro("Proxy is not set. Can not perform operation: IsAtEnd()");
    return 1;
    }
  if (this->TraverseSubProxies)
    {
    if ( (this->Internals->PropertyIterator == this->Proxy->Internals->Properties.end()) && (this->Internals->ProxyIterator == this->Proxy->Internals->SubProxies.end()) )
      {
      return 1;
      }
    }
  else
    {
    if ( this->Internals->PropertyIterator == this->Proxy->Internals->Properties.end() ) 
      {
      return 1;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMPropertyIterator::Next()
{
  if (!this->Proxy)
    {
    vtkErrorMacro("Proxy is not set. Can not perform operation: Next()");
    return;
    }

  int iterating_base_properties = 0; 
    // flag indicating if before the current call to the Next(),
    // the property that was current was a property
    // on the proxy and not on the subproxies.
  
  // If we are still in the root proxy, move to the next element.
  if (this->Internals->PropertyIterator != 
    this->Proxy->Internals->Properties.end())
    {
    iterating_base_properties = 1;
    this->Internals->PropertyIterator++;
    if (this->Internals->PropertyIterator !=
      this->Proxy->Internals->Properties.end())
      {
      return;
      }
    }


  if (!this->TraverseSubProxies)
    {
    return;
    }
  // if iterating_base_properties, the subproperty iterator
  // would have been set to the first exposed property by
  // call to Begin. Hence we don't neet to adjust the subproperty 
  // iterator on this call to Next().
  if (iterating_base_properties)
    {
    return;
    }
  
  // If we moved past the elements in the root proxy, move to the 
  // sub-proxy elements.
  int property_is_exposed = 0;
  while (!property_is_exposed &&
    this->Internals->ProxyIterator != this->Proxy->Internals->SubProxies.end())
    {
    // Increment
    if (this->Internals->SubPropertyIterator != 
      this->Internals->ProxyIterator->second->Internals->Properties.end())
      {
      this->Internals->SubPropertyIterator++;
      }

    // If we reached the end of the current sub-proxy, move to the next one
    if (this->Internals->SubPropertyIterator == 
      this->Internals->ProxyIterator->second->Internals->Properties.end())
      {
      this->Internals->ProxyIterator++;
      // Move to the first sub-proxy which is not empty
      while (this->Internals->ProxyIterator != 
        this->Proxy->Internals->SubProxies.end())
        {
        this->Internals->SubPropertyIterator = 
          this->Internals->ProxyIterator->second->Internals->Properties.begin();
        if ( this->Internals->SubPropertyIterator !=
          this->Internals->ProxyIterator->second->Internals->Properties.end())
          {
          break;
          }
        this->Internals->ProxyIterator++;
        }
      }
    // now verify that the current property is an exposed property,
    // otherwise, we should find the next property.
    if ( ( this->Internals->ProxyIterator !=
      this->Proxy->Internals->SubProxies.end())  &&
      ( this->Internals->SubPropertyIterator !=
        this->Internals->ProxyIterator->second->Internals->Properties.end() ))
      {
      // both iterator are valid.

      if (this->Internals->ProxyIterator->second->Internals->
        ExposedPropertyNames.find(
          this->Internals->SubPropertyIterator->first.c_str()) !=
        this->Internals->ProxyIterator->second->Internals->
        ExposedPropertyNames.end())
        {
        property_is_exposed = 1;
        }
      }
    }
}

//---------------------------------------------------------------------------
const char* vtkSMPropertyIterator::GetKey()
{
  if (!this->Proxy)
    {
    vtkErrorMacro("Proxy is not set. Can not perform operation: GetKey()");
    return 0;
    }

  if (this->Internals->PropertyIterator != 
      this->Proxy->Internals->Properties.end())
    {
    return this->Internals->PropertyIterator->first.c_str();
    }

  if (this->TraverseSubProxies)
    {
    if (this->Internals->ProxyIterator != 
        this->Proxy->Internals->SubProxies.end())
      {
      return this->Internals->SubPropertyIterator->first.c_str();
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMPropertyIterator::GetProperty()
{
  if (!this->Proxy)
    {
    vtkErrorMacro("Proxy is not set. Can not perform operation: GetProperty()");
    return 0;
    }
  if (this->Internals->PropertyIterator != 
      this->Proxy->Internals->Properties.end())
    {
    return this->Internals->PropertyIterator->second.Property.GetPointer();
    }

  if (this->TraverseSubProxies)
    {
    if (this->Internals->ProxyIterator != 
        this->Proxy->Internals->SubProxies.end())
      {
      return this->Internals->SubPropertyIterator->second.Property.GetPointer();
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMPropertyIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TraverseSubProxies: " << this->TraverseSubProxies << endl;
  os << indent << "Proxy: " << this->Proxy << endl;
}
