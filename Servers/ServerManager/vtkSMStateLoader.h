/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMStateLoader.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStateLoader - Utility class to load state from XML
// .SECTION Description
// vtkSMStateLoader can load server manager state from a given 
// vtkPVXMLElement. This element is usually populated by a vtkPVXMLParser.
// .SECTION See Also
// vtkPVXMLParser vtkPVXMLElement

#ifndef __vtkSMStateLoader_h
#define __vtkSMStateLoader_h

#include "vtkSMObject.h"

class vtkPVXMLElement;
class vtkSMProxy;

//BTX
struct vtkSMStateLoaderInternals;
//ETX

class VTK_EXPORT vtkSMStateLoader : public vtkSMObject
{
public:
  static vtkSMStateLoader* New();
  vtkTypeRevisionMacro(vtkSMStateLoader, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the state from the given root element. This root
  // element must have Proxy and ProxyCollection sub-elements
  // Returns 1 on success, 0 on failure.
  // If keep_proxies is set, then the internal map
  // of proxy ids to proxies is not cleared on loading of the state.
  virtual int LoadState(vtkPVXMLElement* rootElement, int keep_proxies=0);

  // Description:
  // Either create a new proxy or returns one from the map
  // of existing properties. Newly created proxies are stored
  // in the map with the id as the key.
  virtual vtkSMProxy* NewProxy(int id);
  virtual vtkSMProxy* NewProxyFromElement(vtkPVXMLElement* proxyElement, int id);
  
  // Description:
  // Get/Set the connection ID for the connection on which the state is
  // loaded. By default it is set to RootServerConnectionID.
  vtkSetMacro(ConnectionID, vtkIdType);
  vtkGetMacro(ConnectionID, vtkIdType);

  // Description:
  // Get/Set if the proxies are to be revived, if the state has sufficient
  // information needed to revive proxies (such as server-side object IDs etc).
  // By default, this is set to 0.
  vtkSetMacro(ReviveProxies, int);
  vtkGetMacro(ReviveProxies, int);


  // Description:
  // Clears all internal references to created proxies.
  void ClearCreatedProxies();
protected:
  vtkSMStateLoader();
  ~vtkSMStateLoader();

  vtkPVXMLElement* RootElement;
  void SetRootElement(vtkPVXMLElement*);

  int ReviveProxies;


  int HandleProxyCollection(vtkPVXMLElement* collectionElement);
  void HandleCompoundProxyDefinitions(vtkPVXMLElement* element);
  int HandleLinks(vtkPVXMLElement* linksElement);
  virtual int BuildProxyCollectionInformation(vtkPVXMLElement*);

  // Description:
  // This method scans through the internal data structure built 
  // during BuildProxyCollectionInformation() and registers the proxy. 
  // The DS keeps info
  // about each proxy ID and the groups and names 
  // the proxy should be registered as (as indicated in the state file).
  void RegisterProxy(int id, vtkSMProxy* proxy);
  virtual void RegisterProxyInternal(const char* group, 
    const char* name, vtkSMProxy* proxy);

  // Either create a new proxy or returns one from the map
  // of existing properties. Newly created proxies are stored
  // in the map with the id as the key. The root is the xml
  // element under which the proxy definitions are stored.
  virtual vtkSMProxy* NewProxy(vtkPVXMLElement* root, int id);

  // Default implementation simply requests the proxy manager
  // to create a new proxy of the given type.
  virtual vtkSMProxy* NewProxyInternal(const char* xmlgroup, const char* xmlname);

  // Description:
  // This method is called to load a proxy state. The implementation
  // here merely calls proxy->LoadState() however, subclass can override to do
  // some state pre-processing.
  virtual int LoadProxyState(vtkPVXMLElement* proxyElement, vtkSMProxy* proxy);

  vtkSMStateLoaderInternals* Internal;

  vtkIdType ConnectionID;
private:
  vtkSMStateLoader(const vtkSMStateLoader&); // Not implemented
  void operator=(const vtkSMStateLoader&); // Not implemented
};

#endif
