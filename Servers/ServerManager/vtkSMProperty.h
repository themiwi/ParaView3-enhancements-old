/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMProperty.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMProperty - superclass for all SM properties
// .SECTION Description
// Each isntance of vtkSMProperty or a sub-class represents a method 
// and associated arguments (if any) of a  a vtk object stored on one 
// or more client manager or server nodes. It may have a state and can push 
// this state to the vtk object it refers to. vtkSMPropery only supports
// methods with no arguments. Sub-classes support methods with different
// arguments types and numbers.
// .SECTION See Also
// vtkSMProxyProperty vtkSMInputProperty vtkSMVectorProperty
// vtkSMDoubleVectorPropery vtkSMIntVectorPropery vtkSMStringVectorProperty

#ifndef __vtkSMProperty_h
#define __vtkSMProperty_h

#include "vtkSMObject.h"
#include "vtkClientServerID.h" // needed for vtkClientServerID

class vtkClientServerStream;
class vtkPVXMLElement;
class vtkSMDomain;
class vtkSMDomainIterator;
class vtkSMProxy;
class vtkSMXMLParser;
//BTX
struct vtkSMPropertyInternals;
//ETX

class VTK_EXPORT vtkSMProperty : public vtkSMObject
{
public:
  static vtkSMProperty* New();
  vtkTypeRevisionMacro(vtkSMProperty, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The command name used to set the value on the server object.
  // For example: SetThetaResolution
  vtkSetStringMacro(Command);
  vtkGetStringMacro(Command);

  // Description:
  // If ImmediateUpdate is true, the value of the property will
  // be pushed to the server as soon as it is modified. Properties
  // that do not have values can be pushed by calling Modified().
  vtkSetMacro(ImmediateUpdate, int);
  vtkGetMacro(ImmediateUpdate, int);

  // Description:
  // Advanced. If UpdateSelf is true, the property will be pushed
  // by calling the method (Command) on the proxy instead of the
  // VTK object. This is commonly used to implement more complicated
  // functionality than can be obtained by calling a method on all
  // server objects.
  vtkSetMacro(UpdateSelf, int);
  vtkGetMacro(UpdateSelf, int);

  // Description:
  vtkGetMacro(IsReadOnly, int);

  // Description:
  vtkSMProperty* GetSubProperty(const char* name);

  // Description:
  // Returns true if all values are in all domains, false otherwise.
  int IsInDomains();

  // Description:
  // Overloaded to break the reference loop caused by the 
  // internal domain iterator.
  virtual void UnRegister(vtkObjectBase* obj);

  // Description:
  vtkSMDomainIterator* NewDomainIterator();

  // Description:
  // Returns a domain give a name. 
  vtkSMDomain* GetDomain(const char* name);

  // Description:
  // Calls Update() on all domains contained by the property
  // as well as all dependant domains. This is usually called
  // after SetUncheckedXXX() to tell all dependant domains to
  // update themselves according to the new value.
  void UpdateDependentDomains();

  // Description:
  static int GetCheckDomains();
  static void SetCheckDomains(int check);

  // Description:
  static int GetModifiedAtCreation();
  static void SetModifiedAtCreation(int check);

protected:
  vtkSMProperty();
  ~vtkSMProperty();

  //BTX
  friend class vtkSMProxyManager;
  friend class vtkSMProxy;
  friend class vtkSMSubPropertyIterator;
  friend class vtkSMDomainIterator;
  friend class vtkSMSourceProxy;
  friend class vtkSMDomain;

  // Description:
  // Append a command to update the vtk object with the property values(s).
  // The proxy objects create a stream by calling this method on all the
  // modified properties.
  virtual void AppendCommandToStream(
    vtkSMProxy*, vtkClientServerStream* stream, vtkClientServerID objectId );
  //ETX

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProxy* parent, 
                                vtkPVXMLElement* element);

  // Description:
  // Update all proxies referred by this property (if any). Overwritten
  // by vtkSMProxyProperty and sub-classes.
  virtual void UpdateAllInputs() {};

  // Description:
  // Properties can have one or more domains. These are assigned by
  // the proxy manager and can be used to obtain information other
  // than given by the type of the propery and its values.
  void AddDomain(const char* name, vtkSMDomain* dom);

  // Description:
  // The name assigned by the xml parser. Used to get the property
  // from a proxy.
  vtkSetStringMacro(XMLName);
  vtkGetStringMacro(XMLName);

  // Description:
  void AddSubProperty(const char* name, vtkSMProperty* proxy);

  // Description:
  void RemoveSubProperty(const char* name);

  // Description:
  vtkSetMacro(IsReadOnly, int);

  // Description:
  vtkSMProperty* NewProperty(const char* name);

  // Description:
  void AddDependent(vtkSMDomain* dom);

  // Description:
  void RemoveAllDependents();

  virtual void SaveState(const char* name, ofstream* file, vtkIndent indent);

  char* Command;

  vtkSMPropertyInternals* PInternals;

  int ImmediateUpdate;
  int UpdateSelf;
  int IsReadOnly;

  char* XMLName;

  vtkSMDomainIterator* DomainIterator;

  static int CheckDomains;
  static int ModifiedAtCreation;

  void SetProxy(vtkSMProxy* proxy);
  vtkSMProxy* Proxy;

private:
  vtkSMProperty(const vtkSMProperty&); // Not implemented
  void operator=(const vtkSMProperty&); // Not implemented
};

#endif
