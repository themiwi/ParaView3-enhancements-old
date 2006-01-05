/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMStateLoader.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>

vtkStandardNewMacro(vtkSMStateLoader);
vtkCxxRevisionMacro(vtkSMStateLoader, "$Revision: 1.3 $");

struct vtkSMStateLoaderInternals
{
  typedef vtkstd::map<int, vtkSmartPointer<vtkSMProxy> >  ProxyMapType;
  ProxyMapType CreatedProxies;
};

//---------------------------------------------------------------------------
vtkSMStateLoader::vtkSMStateLoader()
{
  this->Internal = new vtkSMStateLoaderInternals;
  this->RootElement = 0;
}

//---------------------------------------------------------------------------
vtkSMStateLoader::~vtkSMStateLoader()
{
  delete this->Internal;
  this->RootElement = 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoader::NewProxy(int id)
{
  this->NewProxy(this->RootElement, id);
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoader::NewProxyFromElement(
  vtkPVXMLElement* proxyElement, int id)
{
  vtkSMStateLoaderInternals::ProxyMapType::iterator iter =
    this->Internal->CreatedProxies.find(id);
  if (iter != this->Internal->CreatedProxies.end())
    {
    iter->second->Register(this);
    return iter->second;
    }

  vtkSMProxyManager* pm = this->GetProxyManager();

  const char* group = proxyElement->GetAttribute("group");
  const char* type = proxyElement->GetAttribute("type");
  if (!type || !group)
    {
    vtkErrorMacro("Could not create proxy from element.");
    return 0;
    }
  vtkSMProxy* proxy = pm->NewProxy(group, type);
  if (!proxy)
    {
    vtkErrorMacro("Could not create a proxy of group: "
                  << group
                  << " type: "
                  << type);
    return 0;
    }
  this->Internal->CreatedProxies[id] = proxy;
  if (!proxy->LoadState(proxyElement, this))
    {
    vtkSMStateLoaderInternals::ProxyMapType::iterator iter2 =
      this->Internal->CreatedProxies.find(id);
    this->Internal->CreatedProxies.erase(iter2);
    proxy->Delete();
    vtkErrorMacro("Failed to load state.");
    return 0;
    }
  return proxy;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoader::NewProxy(vtkPVXMLElement* root, 
                                       int id)
{
  vtkSMStateLoaderInternals::ProxyMapType::iterator iter =
    this->Internal->CreatedProxies.find(id);
  if (iter != this->Internal->CreatedProxies.end())
    {
    iter->second->Register(this);
    return iter->second;
    }

  if (!root)
    {
    vtkErrorMacro("No root is defined. Cannot create proxy");
    return 0;
    }

  vtkSMProxyManager* pm = this->GetProxyManager();

  unsigned int numElems = root->GetNumberOfNestedElements();
  unsigned int i=0;
  for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "Proxy") == 0)
      {
      int currentId;
      if (!currentElement->GetScalarAttribute("id", &currentId))
        {
        continue;
        }
      if (id != currentId)
        {
        continue;
        }
      return this->NewProxyFromElement(currentElement, id);
      }
    }

  // If proxy was not found on root level, go into nested elements
  for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    vtkSMProxy* res = this->NewProxy(currentElement, id);
    if (res)
      {
      return res;
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::HandleProxyCollection(vtkPVXMLElement* collectionElement)
{
  vtkSMProxyManager* pm = this->GetProxyManager();

  const char* groupName = collectionElement->GetAttribute("name");
  if (!groupName)
    {
    vtkErrorMacro("Requied attribute name is missing.");
    return 0;
    }
  unsigned int numElems = collectionElement->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = collectionElement->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "Item") == 0)
      {
      int id;
      if (!currentElement->GetScalarAttribute("id", &id))
        {
        vtkErrorMacro("Could not read id for Item. Skipping.");
        continue;
        }
      vtkSMProxy* proxy = this->NewProxy(id);
      if (!proxy)
        {
        continue;
        }
      const char* name = currentElement->GetAttribute("name");
      if (!name)
        {
        vtkErrorMacro("Attribute: name is missing. Cannot register proxy "
                      "with the proxy manager.");
        proxy->Delete();
        continue;
        }
      pm->RegisterProxy(groupName, name, proxy);
      proxy->Delete();
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::ClearCreatedProxies()
{
  this->Internal->CreatedProxies.clear();
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::LoadState(vtkPVXMLElement* rootElement)
{
  if (!rootElement)
    {
    vtkErrorMacro("Cannot load state from (null) root element.");
    return 0;
    }

  vtkSMProxyManager* pm = this->GetProxyManager();
  if (!pm)
    {
    vtkErrorMacro("Cannot load state without a proxy manager.");
    return 0;
    }

  this->RootElement = rootElement;

  this->ClearCreatedProxies();

  unsigned int numElems = rootElement->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "ProxyCollection") == 0)
      {
      if (!this->HandleProxyCollection(currentElement))
        {
        return 0;
        }
      }
    }

  this->ClearCreatedProxies();

  this->RootElement = 0;

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
