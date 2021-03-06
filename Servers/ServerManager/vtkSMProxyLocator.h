/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMProxyLocator.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProxyLocator - is used to locate proxies referred to in state xmls
// while loading state files.
// .SECTION Description
// vtkSMProxyLocator is used to locate proxies referred to in state xmls (and
// otherwise) when loading state files. 

#ifndef __vtkSMProxyLocator_h
#define __vtkSMProxyLocator_h

#include "vtkSMObject.h"

class vtkSMProxy;
class vtkSMDeserializer;

class VTK_EXPORT vtkSMProxyLocator : public vtkSMObject
{
public:
  static vtkSMProxyLocator* New();
  vtkTypeRevisionMacro(vtkSMProxyLocator, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Locate a proxy with the given "name". If none can be found returns NULL.
  // If a proxy with the name was not previously located, it will ask the
  // Deserializer (if any) to create a new proxy is possible.
  vtkSMProxy* LocateProxy(int name);

  // Description:
  // Get/Set the de-serializer to used to locate XMLs for unknown proxies
  // requested to be located using LocateProxy().
  void SetDeserializer(vtkSMDeserializer*);
  vtkGetObjectMacro(Deserializer, vtkSMDeserializer);

  // Description:
  // Get/Set the connection on which to locate the proxies. A proxy locator can
  // only deal with one connection at a time.
  vtkSetMacro(ConnectionID, vtkIdType);
  vtkGetMacro(ConnectionID, vtkIdType);

  // Description:
  // Clear the locator.
  void Clear();

  // Description:
  // WARNING: Temporary API. This will be restructured soon.
  vtkSetMacro(ReviveProxies, int);
  vtkGetMacro(ReviveProxies, int);
//BTX
protected:
  vtkSMProxyLocator();
  ~vtkSMProxyLocator();

  // Description:
  // Create new proxy with the given id. Default implementation asks the
  // Deserializer, if any, to create a new proxy.
  virtual vtkSMProxy* NewProxy(int id);

  vtkIdType ConnectionID;
  vtkSMDeserializer* Deserializer;

  int ReviveProxies;
private:
  vtkSMProxyLocator(const vtkSMProxyLocator&); // Not implemented
  void operator=(const vtkSMProxyLocator&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif

