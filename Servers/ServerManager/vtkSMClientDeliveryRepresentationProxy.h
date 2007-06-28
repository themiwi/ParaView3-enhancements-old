/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMClientDeliveryRepresentationProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMClientDeliveryRepresentationProxy - representation that can be used to
// show client delivery displays, such as a LineChart in a render view.
// .SECTION Description
// vtkSMClientDeliveryRepresentationProxy is a representation that can be used
// to show displays that needs client delivery, such as a LineChart, etc.

#ifndef __vtkSMClientDeliveryRepresentationProxy_h
#define __vtkSMClientDeliveryRepresentationProxy_h

#include "vtkSMDataRepresentationProxy.h"
class vtkDataObject;

class VTK_EXPORT vtkSMClientDeliveryRepresentationProxy : 
  public vtkSMDataRepresentationProxy
{
public:
  static vtkSMClientDeliveryRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMClientDeliveryRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Method gets called to set input when using Input property.
  // Internally leads to a call to SetInput.
  virtual void AddInput(unsigned int inputPort,
                        vtkSMSourceProxy* input,
                        unsigned int outputPort,
                        const char* method);
  virtual void AddInput(vtkSMSourceProxy* input, 
                        const char* method)
  {
    this->AddInput(0, input, 0, method);
  }

  // Description:
  // Get the data that was collected to the client
  virtual vtkDataObject* GetOutput();

  // Description:
  // Called to update the Display. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  virtual void Update() { this->Update(0); };
  virtual void Update(vtkSMViewProxy* view);

  //BTX
  enum ReductionTypeEnum
    {
    ADD = 0,
    POLYDATA_APPEND = 1,
    UNSTRUCTURED_APPEND = 2,
    FIRST_NODE_ONLY = 3,
    RECTILINEAR_GRID_APPEND=4,
    COMPOSITE_DATASET_APPEND=5
    };
  //ETX

  // Description:
  // Set the reduction algorithm type. Cannot be called before
  // objects are created.
  void SetReductionType(int type);

//BTX
protected:
  vtkSMClientDeliveryRepresentationProxy();
  ~vtkSMClientDeliveryRepresentationProxy();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();

  void SetInputInternal();
  bool SetupStrategy();

  vtkSMProxy* ReduceProxy;
  vtkSMRepresentationStrategy* StrategyProxy;
  vtkSMSourceProxy* PostProcessorProxy;

  int ReductionType;

  // Proxies for the selection pipeline.
  vtkSMSourceProxy* ExtractSelection;

private:
  vtkSMClientDeliveryRepresentationProxy(const vtkSMClientDeliveryRepresentationProxy&); // Not implemented
  void operator=(const vtkSMClientDeliveryRepresentationProxy&); // Not implemented
//ETX
};

#endif

