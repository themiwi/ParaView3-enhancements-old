/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSimpleParallelStrategy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSimpleParallelStrategy
// .SECTION Description
//

#ifndef __vtkSMSimpleParallelStrategy_h
#define __vtkSMSimpleParallelStrategy_h

#include "vtkSMSimpleStrategy.h"

class VTK_EXPORT vtkSMSimpleParallelStrategy : public vtkSMSimpleStrategy
{
public:
  static vtkSMSimpleParallelStrategy* New();
  vtkTypeRevisionMacro(vtkSMSimpleParallelStrategy, vtkSMSimpleStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the data generator that goes into the distributor.
  virtual vtkSMSourceProxy* GetDistributedSource()
    { return this->PreDistributorSuppressor; }

  // Description:
  // Update the distributor to use ordered compositing.
  vtkGetMacro(UseOrderedCompositing, bool);
  virtual void SetUseOrderedCompositing(bool);

  // Description:
  // Update the data the gets distributed.
  virtual void UpdateDistributedData();
//BTX
protected:
  vtkSMSimpleParallelStrategy();
  ~vtkSMSimpleParallelStrategy();

  // Description:
  // Overridden to set the servers correctly on all subproxies.
  virtual void CreateVTKObjects(int numObjects);

  // Description:
  // Create and initialize the data pipeline.
  virtual void CreatePipeline(vtkSMSourceProxy* input);

  // Description:
  // Create and initialize the LOD data pipeline.
  // Note that this method is called irrespective of EnableLOD
  // flag.
  virtual void CreateLODPipeline(vtkSMSourceProxy* input);

  // Description:
  // Update the LOD pipeline.
  // Overridden to pass correct collection decision to the Collect filter
  // based on UseCompositing() flag.
  virtual void UpdateLODPipeline();

  // Description:
  // Updates the data pipeline (non-LOD only).
  // Overridden to pass correct collection decision to the Collect filter
  // based on UseCompositing() flag.
  virtual void UpdatePipeline();

  // Description:
  // Indicates if the current update should use compositing or not.
  bool UseCompositing();

  vtkSMSourceProxy* Collect;
  vtkSMSourceProxy* PreDistributorSuppressor;
  vtkSMSourceProxy* Distributor;

  vtkSMSourceProxy* CollectLOD;
  vtkSMSourceProxy* PreDistributorSuppressorLOD;
  vtkSMSourceProxy* DistributorLOD;
  
  bool UseOrderedCompositing;

private:
  vtkSMSimpleParallelStrategy(const vtkSMSimpleParallelStrategy&); // Not implemented
  void operator=(const vtkSMSimpleParallelStrategy&); // Not implemented

  // Since LOD and full res pipeline have exactly the same setup, we have this
  // common method.
  void CreatePipelineInternal(vtkSMSourceProxy* input, vtkSMSourceProxy* collect,
    vtkSMSourceProxy* predistributorsuppressor, vtkSMSourceProxy* distributor,
    vtkSMSourceProxy* updatesuppressor);
//ETX
};

#endif

