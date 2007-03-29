/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSourceProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSourceProxy - proxy for a VTK source on a server
// .SECTION Description
// vtkSMSourceProxy manages VTK source(s) that are created on a server
// using the proxy pattern. In addition to functionality provided
// by vtkSMProxy, vtkSMSourceProxy provides method to connect and
// update sources. Each source proxy has one or more parts (vtkSMPart).
// Each part represents one output of one filter. These are created
// automatically (when CreateParts() is called) by the source.
// Each vtkSMSourceProxy creates a property called DataInformation.
// This property is a composite property that provides information
// about the output(s) of the VTK sources (obtained from the server)
// .SECTION See Also
// vtkSMProxy vtkSMPart vtkSMInputProperty

#ifndef __vtkSMSourceProxy_h
#define __vtkSMSourceProxy_h

#include "vtkSMProxy.h"
#include "vtkClientServerID.h" // Needed for ClientServerID

class vtkPVArrayInformation;
class vtkPVDataInformation;
class vtkPVDataSetAttributesInformation;
//BTX
struct vtkSMSourceProxyInternals;
//ETX
class vtkSMPart;
class vtkSMProperty;

class VTK_EXPORT vtkSMSourceProxy : public vtkSMProxy
{
public:
  static vtkSMSourceProxy* New();
  vtkTypeRevisionMacro(vtkSMSourceProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Calls UpdateInformation() on all sources.
  virtual void UpdatePipelineInformation();

  // Description:
  // Calls Update() on all sources. It also creates parts if
  // they are not already created.
  virtual void UpdatePipeline();

  // Description:
  // Calls Update() on all sources with the given time request. 
  // It also creates parts if they are not already created.
  virtual void UpdatePipeline(double time);

  // Description:
  // Connects filters/sinks to an input. If the filter(s) is not
  // created, this will create it. If hasMultipleInputs is
  // true,  only one filter is created, even if the input has
  // multiple parts. All the inputs are added using the method
  // name provided. If hasMultipleInputs is not true, one filter
  // is created for each input. NOTE: The filter(s) is created
  // when SetInput is called the first and if the it wasn't already
  // created. If the filter has two inputs and one is multi-block
  // whereas the other one is not, SetInput() should be called with
  // the multi-block input first. Otherwise, it will create only
  // one filter and can not apply to the multi-block input.
  virtual void AddInput(vtkSMSourceProxy* input, 
                const char* method,
                int hasMultipleInputs);

  // Description:
  // Calls method on all VTK sources. Used by the input property 
  // to remove inputs. Made public to allow access by wrappers. Do
  // not use.
  void CleanInputs(const char* method);

  // Description:
  // Chains to superclass and calls UpdateInformation()
  virtual void UpdateSelfAndAllInputs();

  // Description:
  // Return the number of parts (output proxies).
  unsigned int GetNumberOfParts();

  // Description:
  // Return a part (output proxy).
  vtkSMPart* GetPart(unsigned int idx);

  // Description:
  // Create n parts where n is the number of filters. Each part
  // correspond to one output of one filter.
  virtual void CreateParts();

  // Description:
  // DataInformation is used by the source proxy to obtain information
  // on the output(s) from the server. The information contained in
  // this object is also copied to the DataInformation automatically.
  // The direct use of the data information object is low level and
  // should be avoided if possible.
  vtkPVDataInformation* GetDataInformation();

  // Description:
  // Returns if the data information is currently valid.
  vtkGetMacro(DataInformationValid, int);

  // Description:
  // Chains to superclass as well as mark the data information as
  // invalid (next time data information is requested, it will be
  // re-created).
  virtual void MarkModified(vtkSMProxy* modifiedProxy);

  // Description:
  // Mark the data information as invalid. If invalidateConsumers
  // is true, all consumers' data information is also marked as
  // invalid.
  void InvalidateDataInformation(int invalidateConsumers);

  // Description:
  // This method saves state information about the proxy
  // which can be used to revive the proxy using server side objects
  // already present. This includes the entire state saved by calling 
  // SaveState() as well additional information such as server side
  // object IDs.
  // Overridden to save information pertinant to reviving the parts.
  virtual vtkPVXMLElement* SaveRevivalState(vtkPVXMLElement* root);
  virtual int LoadRevivalState(vtkPVXMLElement* revivalElement, 
    vtkSMStateLoader* loader);
protected:
  vtkSMSourceProxy();
  ~vtkSMSourceProxy();

//BTX
  friend class vtkSMInputProperty;
//ETX

  int PartsCreated;


  // Description:
  // Obtain data information from server (does not check if the
  // data information is valid)
  void GatherDataInformation();

  // Description:
  // Mark the data information as invalid.
  void InvalidateDataInformation();

  vtkPVDataInformation *DataInformation;
  int DataInformationValid;

  // Description:
  // Call superclass' and then assigns a new executive 
  // (vtkCompositeDataPipeline)
  virtual void CreateVTKObjects(int numObjects);

  char *ExecutiveName;
  vtkSetStringMacro(ExecutiveName);

  // Description:
  // Read attributes from an XML element.
  virtual int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

  // Description:
  // Internal method which creates the parts using the proxy specified.
  void CreatePartsInternal(vtkSMProxy* op);

  int DoInsertExtractPieces;

private:
  vtkSMSourceProxyInternals* PInternals;

  vtkSMSourceProxy(const vtkSMSourceProxy&); // Not implemented
  void operator=(const vtkSMSourceProxy&); // Not implemented
};

#endif
