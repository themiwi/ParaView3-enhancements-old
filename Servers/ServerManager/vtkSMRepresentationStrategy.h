/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMRepresentationStrategy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRepresentationStrategy - strategy for representation proxies that
// encapsulates lod/parallel pipelines.
// .SECTION Description
// vtkSMRepresentationStrategy is an abstract superclass for representation
// stragtegies that encapsulate the lod/parallel pipelines. 
// Representation implementations don't need to implement pipeline for handling
// parallel rendering instead, they simply ask for a strategy based on their
// data type and mode of rendering (ie. surface/volume) from the view proxy to
// which they are added.

#ifndef __vtkSMRepresentationStrategy_h
#define __vtkSMRepresentationStrategy_h

#include "vtkSMProxy.h"

class vtkSMSourceProxy;
class vtkPVDataInformation;

class VTK_EXPORT vtkSMRepresentationStrategy : public vtkSMProxy
{
public:
  vtkTypeRevisionMacro(vtkSMRepresentationStrategy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input to the strategy.
  void SetInput(vtkSMSourceProxy* input);

  // Description:
  // Get the output from the strategy.
  virtual vtkSMSourceProxy* GetOutput()=0;

  // Description:
  // Get low resolution output.
  virtual vtkSMSourceProxy* GetLODOutput()=0;

  // Description:
  // Get the data information. If EnableLOD is true and ViewHelper
  // indicates that the rendering decision was to use LOD, then
  // this method will return the data information for the 
  // LOD data displayed otherwise non-lod data information 
  // will be returned.
  // This method on the strategy does not update the pipeline before getting 
  // the data information i.e. it will returns the current data information.
  vtkPVDataInformation* GetDisplayedDataInformation();

  // Description:
  // Must be set if the representation makes use of the LOD pipeline provided 
  // by the strategy. false by default, implying that the representation only 
  // displays the high resolution data.
  vtkSetMacro(EnableLOD, bool);
  vtkGetMacro(EnableLOD, bool);

  // Description:
  // Helper is used to determine the current LOD decision.
  void SetViewHelperProxy(vtkSMProxy*);
  vtkGetObjectMacro(ViewHelperProxy, vtkSMProxy);

  // Description:
  // Returns if the strategy is not up-to-date. This happens
  // when any proxy upstream was modified since the last Update()
  // on the strategy's displayed pipeline.
  // Note that this reply may change if the decision to use
  // LOD changes.
  bool UpdateRequired();

  // Description:
  // Updates the displayed pipeline if update is required.
  void Update();

  // Description:
  // Overridden to clear data valid flags.
  virtual void MarkModified(vtkSMProxy* modifiedProxy);

//BTX
protected:
  vtkSMRepresentationStrategy();
  ~vtkSMRepresentationStrategy();

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input) =0;

  // Description:
  // Create and initialize the LOD data pipeline.
  // Note that this method is called irrespective of EnableLOD
  // flag.
  virtual void CreateLODPipeline(vtkSMSourceProxy* input) =0;

  // Description:
  // Gather the information of the displayed data
  // for the current update state of the data pipeline (non-LOD).
  virtual void GatherInformation(vtkPVDataInformation*) = 0;

  // Description:
  // Gather the information of the displayed data
  // for the current update state of the LOD pipeline.
  virtual void GatherLODInformation(vtkPVDataInformation*) = 0;

  // Description:
  // Update the LOD pipeline. Subclasses must override this method
  // to provide their own implementation and then call the superclass
  // to ensure that various flags are updated correctly.
  // This method need not worry about caching, since LODPipeline is never used
  // when caching is enabled.
  virtual void UpdateLODPipeline();

  // Description:
  // Updates the data pipeline (non-LOD only).
  // Subclasses must override this method
  // to provide their own implementation and then call the superclass
  // to ensure that various flags are updated correctly.
  // This method should respect caching, if supported. Call
  // UseCache() to check if caching is to be employed.
  virtual void UpdatePipeline();

  // Description:
  // Returns if the strategy should use LOD pipeline for its operations.
  bool UseLODPipeline();

  // Description:
  // Retruns if caching is enabled.
  bool UseCache();

  // Description:
  // Creates a connection between the producer and the consumer
  // using "Input" property. Subclasses can use this to build
  // pipelines.
  void Connect(vtkSMProxy* producer, vtkSMProxy* consumer,
    const char* propertyname="Input");

  vtkSMSourceProxy* Input;
  bool EnableLOD;
  bool LODDataValid;
  bool LODInformationValid;
  vtkPVDataInformation* LODInformation;

  bool DataValid;
  bool InformationValid;
  vtkPVDataInformation* Information;

  vtkSMProxy* ViewHelperProxy;
private:
  vtkSMRepresentationStrategy(const vtkSMRepresentationStrategy&); // Not implemented
  void operator=(const vtkSMRepresentationStrategy&); // Not implemented
//ETX
};

#endif

