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
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMCameraLink.h"
#include "vtkSMGlobalPropertiesManager.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStateVersionController.h"
#include "vtkSMViewProxy.h"

#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMStateLoader);
vtkCxxRevisionMacro(vtkSMStateLoader, "$Revision: 1.35 $");
vtkCxxSetObjectMacro(vtkSMStateLoader, ProxyLocator, vtkSMProxyLocator);
//---------------------------------------------------------------------------
struct vtkSMStateLoaderRegistrationInfo
{
  vtkstd::string GroupName;
  vtkstd::string ProxyName;
};

struct vtkSMStateLoaderInternals
{
  typedef vtkstd::vector<vtkSMStateLoaderRegistrationInfo> VectorOfRegInfo;
  typedef vtkstd::map<int, VectorOfRegInfo> RegInfoMapType;
  RegInfoMapType RegistrationInformation;
};

//---------------------------------------------------------------------------
vtkSMStateLoader::vtkSMStateLoader()
{
  this->Internal = new vtkSMStateLoaderInternals;
  this->ServerManagerStateElement = 0;
  this->ProxyLocator = vtkSMProxyLocator::New();
}

//---------------------------------------------------------------------------
vtkSMStateLoader::~vtkSMStateLoader()
{
  this->SetProxyLocator(0);
  this->ServerManagerStateElement = 0;
  this->ProxyLocator = 0;
  delete this->Internal;
}

//---------------------------------------------------------------------------
const char* vtkSMStateLoader::GetViewXMLName (int connectionID,
  const char *xml_name)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMViewProxy* prototype = vtkSMViewProxy::SafeDownCast(
    pxm->GetPrototypeProxy("views", xml_name));
  if (prototype)
    {
    // Generally each view type is different class of view eg. bar char view, line
    // plot view etc. However in some cases a different view types are indeed the
    // same class of view the only different being that each one of them works in
    // a different configuration eg. "RenderView" in builin mode, 
    // "IceTDesktopRenderView" in remote render mode etc. This method is used to
    // determine what type of view needs to be created for the given class. 
    return prototype->GetSuggestedViewType(connectionID);
    }

  return xml_name;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoader::CreateProxy(
  const char* xml_group, const char* xml_name, vtkIdType cid)
{
  // Check if the proxy requested is a view module.
  if (xml_group && xml_name && strcmp(xml_group, "views") == 0)
    {
    return this->Superclass::CreateProxy(xml_group,
      this->GetViewXMLName(cid, xml_name), cid);
    }

  // If all else fails, let the superclass handle it:
  return this->Superclass::CreateProxy(xml_group, xml_name, cid);
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::CreatedNewProxy(int id, vtkSMProxy* proxy)
{
  // Ensure that the proxy is created before it is registered, unless we are
  // reviving the server-side server manager, which needs special handling.
  proxy->UpdateVTKObjects();
  if (proxy->IsA("vtkSMSourceProxy"))
    {
    vtkSMSourceProxy::SafeDownCast(proxy)->UpdatePipelineInformation();
    }
  this->RegisterProxy(id, proxy);
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
vtkPVXMLElement* vtkSMStateLoader::LocateProxyElement(int id)
{
  return this->LocateProxyElementInternal(
    this->ServerManagerStateElement, id);
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMStateLoader::LocateProxyElementInternal(
  vtkPVXMLElement* root, int id)
{
  if (!root)
    {
    vtkErrorMacro("No root is defined. Cannot locate proxy element with id " 
      << id);
    return 0;
    }

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
      return currentElement;
      }
    }

  // If proxy was not found on root level, go into nested elements
  for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = root->GetNestedElement(i);
    vtkPVXMLElement* res = this->LocateProxyElementInternal(currentElement, id);
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
    vtkErrorMacro("Required attribute name is missing.");
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

      vtkSMProxy* proxy = this->ProxyLocator->LocateProxy(id);
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
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::HandleCustomProxyDefinitions(
  vtkPVXMLElement* element)
{
  vtkSMProxyManager* pm = this->GetProxyManager();
  pm->LoadCustomProxyDefinitions(element);
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::HandleGlobalPropertiesManagers(vtkPVXMLElement* element)
{
  vtkSMProxyManager* pxm = this->GetProxyManager();
  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* currentElement= element->GetNestedElement(cc);
    const char* name = currentElement->GetName();
    const char* mgrname = currentElement->GetAttribute("name");
    if (!name || !mgrname || strcmp(name, "GlobalPropertiesManager") != 0)
      {
      continue;
      }
    vtkstd::string group = currentElement->GetAttribute("group");
    vtkstd::string type = currentElement->GetAttribute("type");
    vtkSMGlobalPropertiesManager* mgr =
      pxm->GetGlobalPropertiesManager(mgrname);
    if (mgr && (group != mgr->GetXMLGroup() || type != mgr->GetXMLName()))
      {
      vtkErrorMacro("GlobalPropertiesManager with name " << mgrname
        << " exists, however is of different type.");
      return 0;
      }
    if (!mgr)
      {
      mgr = vtkSMGlobalPropertiesManager::New();
      mgr->InitializeProperties(group.c_str(), type.c_str());
      pxm->SetGlobalPropertiesManager(mgrname, mgr);
      mgr->Delete();
      }
    if (!mgr->LoadLinkState(currentElement, this->ProxyLocator))
      {
      return 0;
      }
    }
  return 1;
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
        if (!link->LoadState(currentElement, this->ProxyLocator))
          {
          return 0;
          }
        }
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
bool vtkSMStateLoader::VerifyXMLVersion(vtkPVXMLElement* rootElement)
{
  const char* version = rootElement->GetAttribute("version");
  if (!version)
    {
    vtkWarningMacro("ServerManagerState missing \"version\" information.");
    return true;
    }

  // Nothing to check here really.
  return true;
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::LoadState(vtkPVXMLElement* elem)
{
  if (!elem)
    {
    vtkErrorMacro("Cannot load state from (null) root element.");
    return 0;
    }

  if (!this->GetProxyManager())
    {
    vtkErrorMacro("Cannot load state without a proxy manager.");
    return 0;
    }

  if (!this->ProxyLocator)
    {
    vtkErrorMacro("Please set the locator correctly.");
    return 0;
    }

  this->ProxyLocator->SetDeserializer(this);
  int ret = this->LoadStateInternal(elem);
  this->ProxyLocator->SetDeserializer(0);
  return ret;
}

//---------------------------------------------------------------------------
int vtkSMStateLoader::LoadStateInternal(vtkPVXMLElement* rootElement)
{
  if (rootElement->GetName() && 
    strcmp(rootElement->GetName(),"ServerManagerState") != 0)
    {
    rootElement = rootElement->FindNestedElementByName("ServerManagerState");
    if (!rootElement)
      {
      vtkErrorMacro("Failed to locate <ServerManagerState /> element."
        << "Cannot load server manager state.");
      return 0;
      }
    }
  
  vtkSMStateVersionController* convertor = vtkSMStateVersionController::New();
  if (!convertor->Process(rootElement))
    {
    vtkWarningMacro("State convertor was not able to convert the state to current "
      "version successfully");
    }
  convertor->Delete();

  if (!this->VerifyXMLVersion(rootElement))
    {
    return 0;
    }

  this->ServerManagerStateElement = rootElement;

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

  // Load all compound proxy definitions.
   for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = rootElement->GetNestedElement(i);
    const char* name = currentElement->GetName();
    if (name)
      {
      if (strcmp(name, "CustomProxyDefinitions") == 0)
        {
        this->HandleCustomProxyDefinitions(currentElement);
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
      else if (strcmp(name, "Links") == 0)
        {
        this->HandleLinks(currentElement);
        }
      else if (strcmp(name, "GlobalPropertiesManagers") == 0)
        {
        this->HandleGlobalPropertiesManagers(currentElement);
        }
      }
    }

  // Clear internal data structures.
  this->Internal->RegistrationInformation.clear();
  this->ServerManagerStateElement = 0; 
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
