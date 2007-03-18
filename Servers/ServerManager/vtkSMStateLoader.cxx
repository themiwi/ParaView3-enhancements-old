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
#include "vtkProcessModuleConnectionManager.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLink.h"
#include "vtkSMCameraLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMStateLoader);
vtkCxxRevisionMacro(vtkSMStateLoader, "$Revision: 1.20 $");
vtkCxxSetObjectMacro(vtkSMStateLoader, RootElement, vtkPVXMLElement);
//---------------------------------------------------------------------------
struct vtkSMStateLoaderRegistrationInfo
{
  vtkstd::string GroupName;
  vtkstd::string ProxyName;
};

struct vtkSMStateLoaderInternals
{
  typedef vtkstd::map<int, vtkSmartPointer<vtkSMProxy> >  ProxyMapType;
  ProxyMapType CreatedProxies;

  typedef vtkstd::vector<vtkSMStateLoaderRegistrationInfo> VectorOfRegInfo;
  typedef vtkstd::map<int, VectorOfRegInfo> RegInfoMapType;
  RegInfoMapType RegistrationInformation;
};

//---------------------------------------------------------------------------
vtkSMStateLoader::vtkSMStateLoader()
{
  this->Internal = new vtkSMStateLoaderInternals;
  this->RootElement = 0;
  this->ConnectionID = 
    vtkProcessModuleConnectionManager::GetRootServerConnectionID();
  this->ReviveProxies = 0;
}

//---------------------------------------------------------------------------
vtkSMStateLoader::~vtkSMStateLoader()
{
  delete this->Internal;
  this->SetRootElement(0);
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoader::NewProxy(int id)
{
  return this->NewProxy(this->RootElement, id);
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

  vtkSMProxy* proxy = 0;
  if (strcmp(proxyElement->GetName(), "Proxy") == 0)
    {
    const char* group = proxyElement->GetAttribute("group");
    const char* type = proxyElement->GetAttribute("type");
    if (!type || !group)
      {
      vtkErrorMacro("Could not create proxy from element.");
      return 0;
      }
    proxy = this->NewProxyInternal(group, type);
    if (!proxy)
      {
      vtkErrorMacro("Could not create a proxy of group: "
                    << group
                    << " type: "
                    << type);
      return 0;
      }

    if (!proxy->GetObjectsCreated())
      {
      proxy->SetConnectionID(this->ConnectionID);
      }
    }
  else if (strcmp(proxyElement->GetName(), "CompoundProxy") == 0)
    {
    proxy = vtkSMCompoundProxy::New();
    proxy->SetConnectionID(this->ConnectionID);
    }
  this->Internal->CreatedProxies[id] = proxy;

  if (!this->LoadProxyState(proxyElement, proxy))
    {
    vtkSMStateLoaderInternals::ProxyMapType::iterator iter2 =
      this->Internal->CreatedProxies.find(id);
    this->Internal->CreatedProxies.erase(iter2);
    proxy->Delete();
    vtkErrorMacro("Failed to load state.");
    return 0;
    }

  // Register this new proxy before we use it.
  this->RegisterProxy(id, proxy);
  return proxy;
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::RegisterProxy(int id, vtkSMProxy* proxy)
{
  vtkSMStateLoaderInternals::RegInfoMapType::iterator iter
    = this->Internal->RegistrationInformation.find(id);
  if (iter == this->Internal->RegistrationInformation.end())
    {
    return;
    }
  vtkSMStateLoaderInternals::VectorOfRegInfo::iterator iter2;
  for (iter2 =iter->second.begin(); iter2 != iter->second.end(); iter2++)
    {
    this->RegisterProxyInternal(iter2->GroupName.c_str(), 
      iter2->ProxyName.c_str(), proxy);
    }
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::RegisterProxyInternal(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->RegisterProxy(group, name, proxy);
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoader::NewProxyInternal(const char* xmlgroup, 
  const char* xmlname)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  return pxm->NewProxy(xmlgroup, xmlname);
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
    vtkErrorMacro("No root is defined. Cannot create proxy with id " << id);
    return 0;
    }

  unsigned int numElems = root->GetNumberOfNestedElements();
  unsigned int i=0;
  for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    if (currentElement->GetName() &&
        (strcmp(currentElement->GetName(), "Proxy") == 0 ||
         strcmp(currentElement->GetName(), "CompoundProxy") == 0))
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
int vtkSMStateLoader::BuildProxyCollectionInformation(
  vtkPVXMLElement* collectionElement)
{
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
      const char* name = currentElement->GetAttribute("name");
      if (!name)
        {
        vtkErrorMacro("Attribute: name is missing. Cannot register proxy "
          "with the proxy manager.");
        continue;
        }
      vtkSMStateLoaderRegistrationInfo info;
      info.GroupName = groupName;
      info.ProxyName = name;
      this->Internal->RegistrationInformation[id].push_back(info);
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::HandleProxyCollection(vtkPVXMLElement* collectionElement)
{
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
      // No need to register
      //pm->RegisterProxy(groupName, name, proxy);
      proxy->Delete();
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::HandleCompoundProxyDefinitions(
  vtkPVXMLElement* element)
{
  vtkSMProxyManager* pm = this->GetProxyManager();
  pm->LoadCompoundProxyDefinitions(element);
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::HandleLinks(vtkPVXMLElement* element)
{
  vtkSMProxyManager* pxm = this->GetProxyManager();
  
  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* currentElement= element->GetNestedElement(cc);
    const char* name = currentElement->GetName();
    const char* linkname = currentElement->GetAttribute("name");
    if (name && linkname)
      {
      vtkSMLink* link = 0;
      if (strcmp(name, "PropertyLink") == 0)
        {
        link = pxm->GetRegisteredLink(linkname);
        if (!link)
          {
          link = vtkSMPropertyLink::New();
          pxm->RegisterLink(linkname, link);
          link->Delete();
          }
        }
      else if (strcmp(name, "ProxyLink") == 0)
        {
        link = pxm->GetRegisteredLink(linkname);
        if (!link)
          {
          link = vtkSMProxyLink::New();
          pxm->RegisterLink(linkname, link);
          link->Delete();
          }       
        }
      else if (strcmp(name, "CameraLink") == 0)
        {
        link = pxm->GetRegisteredLink(linkname);
        if (!link)
          {
          link = vtkSMCameraLink::New();
          pxm->RegisterLink(linkname, link);
          link->Delete();
          }       
        }
      if (link)
        {
        if (!link->LoadState(currentElement, this))
          {
          return 0;
          }
        }
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::LoadProxyState(vtkPVXMLElement* elem, vtkSMProxy* proxy)
{
  return proxy->LoadState(elem, this);
}
  

//---------------------------------------------------------------------------
void vtkSMStateLoader::ClearCreatedProxies()
{
  this->Internal->CreatedProxies.clear();
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::LoadState(vtkPVXMLElement* rootElement, int keep_proxies/*=0*/)
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

  this->SetRootElement(rootElement);

  this->ClearCreatedProxies();
  this->Internal->RegistrationInformation.clear();

  unsigned int numElems = rootElement->GetNumberOfNestedElements();
  unsigned int i;
  for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(i);
    const char* name = currentElement->GetName();
    if (name)
      {
      if (strcmp(name, "ProxyCollection") == 0)
        {
        if (!this->BuildProxyCollectionInformation(currentElement))
          {
          return 0;
          }
        }
      }
    }
  for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(i);
    const char* name = currentElement->GetName();
    if (name)
      {
      if (strcmp(name, "ProxyCollection") == 0)
        {
        if (!this->HandleProxyCollection(currentElement))
          {
          return 0;
          }
        }
      else if (strcmp(name, "CompoundProxyDefinitions") == 0)
        {
        this->HandleCompoundProxyDefinitions(currentElement);
        }
      else if (strcmp(name, "Links") == 0)
        {
        this->HandleLinks(currentElement);
        }
      }
    }

  if (!keep_proxies)
    {
    this->ClearCreatedProxies();
    }

  this->SetRootElement(0);

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConnectionID: " << this->ConnectionID << endl;
  os << indent << "ReviveProxies: " << this->ReviveProxies << endl;
}
