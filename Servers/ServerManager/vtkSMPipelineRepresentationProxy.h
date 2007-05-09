/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPipelineRepresentationProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPipelineRepresentationProxy - Superclass for representations that
// have some data input.
// .SECTION Description
// vtkSMPipelineRepresentationProxy is a superclass for representations that
// consume data i.e. require some input.
// .SECTION Caveats
// \li Generally speaking, this proxy requires that the input is set before the
// representation is added to any view. This requirement stems from the fact
// that to deterine the right strategy to use for a representation, it may be
// necessary to know the data type of the input data.

#ifndef __vtkSMPipelineRepresentationProxy_h
#define __vtkSMPipelineRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class vtkSMSourceProxy;
class vtkSMRepresentationStrategy;
class vtkSMPipelineRepresentationProxyObserver;

class VTK_EXPORT vtkSMPipelineRepresentationProxy : 
  public vtkSMRepresentationProxy
{
public:
  vtkTypeRevisionMacro(vtkSMPipelineRepresentationProxy, 
    vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // vtkSMInputProperty requires that the consumer proxy support
  // AddInput() method. Hence, this method is defined. This method
  // sets up the input connection.
  void AddInput(vtkSMSourceProxy* input, const char* method,
    int hasMultipleInputs);

  // Description:
  // Get the data information for the represented data.
  // Representations that do not have significatant data representations such as
  // 3D widgets, text annotations may return NULL.
  // Overridden to return the strategy's data information.
  virtual vtkPVDataInformation* GetDisplayedDataInformation();

  // Description:
  // Get the data information for the full resolution data irrespective of
  // whether current rendering decision was to use LOD. For representations that
  // don't have separate LOD pipelines, this simply calls
  // GetDisplayedDataInformation().
  // Overridden to return the strategy's data information.
  virtual vtkPVDataInformation* GetFullResDataInformation();

  // Description:
  // Called to update the Representation. 
  // Overridden to forward the update request to the strategy if any. 
  // If subclasses don't use any strategy, they may want to override this
  // method.
  // Fires vtkCommand:StartEvent and vtkCommand:EndEvent and start and end of
  // the update if it is indeed going to cause some pipeline execution.
  virtual void Update(vtkSMViewProxy* view);
  virtual void Update() { this->Superclass::Update(); };

  // Description:
  // Returns if the representation's input has changed since most recent
  // Update(). Overridden to forward the request to the strategy, if any. If
  // subclasses don't use any strategy, they may want to override this method.
  virtual bool UpdateRequired();
  
  // Description:
  // Set the time used during update requests.
  // Default implementation passes the time to the strategy, if any. If
  // subclasses don't use any stratgy, they may want to override this method.
  virtual void SetUpdateTime(double time);

  // Description:
  // Get the representation strategy used by this representation, if any.
  vtkGetObjectMacro(Strategy, vtkSMRepresentationStrategy);

//BTX
protected:
  vtkSMPipelineRepresentationProxy();
  ~vtkSMPipelineRepresentationProxy();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // If this method returns false, CreateVTKObjects() is aborted.
  // Overridden to abort CreateVTKObjects() only if the input has
  // been initialized correctly.
  virtual bool BeginCreateVTKObjects(int numObjects);

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Added to call InitializeStrategy() to give subclassess the opportunity to
  // set up pipelines involving compositing strategy it they support it.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Some representations may require lod/compositing strategies from the view
  // proxy. This method gives such subclasses an opportunity to as the view
  // module for the right kind of strategy and plug it in the representation
  // pipeline. Returns true on success. Default implementation suffices for
  // representation that don't use strategies.
  virtual bool InitializeStrategy(vtkSMViewProxy* vtkNotUsed(view))
    { return true; }

  // Description:
  // Set the representation strategy. Simply initializes the Strategy ivar.
  void SetStrategy(vtkSMRepresentationStrategy*);

  // Description:
  // Provide access to Input for subclasses.
  vtkGetObjectMacro(InputProxy, vtkSMSourceProxy);

  // Description:
  // Subclasses can use this method to traverse up the input connection
  // from this representation and mark them modified.
  void MarkUpstreamModified();

  double UpdateTime;
  bool UpdateTimeInitialized;
private:
  vtkSMPipelineRepresentationProxy(const vtkSMPipelineRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPipelineRepresentationProxy&); // Not implemented

  void SetInputProxy(vtkSMSourceProxy*);
  vtkSMSourceProxy* InputProxy;
  vtkSMRepresentationStrategy* Strategy;

  vtkSMPipelineRepresentationProxyObserver* Observer;
//ETX
};

#endif

